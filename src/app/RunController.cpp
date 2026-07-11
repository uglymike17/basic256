/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include <math.h>
#include <iostream>

#include <QProcess>
#include <QDesktopServices>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>
#include <QDir>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QApplication>
#include <QFileDialog>
#include <QClipboard>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>


#include "RunController.h"
#include "MainWindow.h"
#include "Settings.h"
#include "md5.h"
#include "Sleeper.h"

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <servprov.h>
#include <string>
#include <cstdlib>
//	#include <mmsystem.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#endif

#ifdef LINUX
// sys/soundcard.h instead of linux/soundcard.h in Debian so that we don't
// FTBFS on kfreebsd (See Debian bug #594164).
#include <sys/soundcard.h>
#endif


extern QMutex* mymutex;
extern QMutex* mydebugmutex;
extern QWaitCondition* waitCond;
extern QWaitCondition* waitDebugCond;

extern MainWindow * mainwin;
extern int guiState;
extern BasicEdit * editwin;
extern BasicOutput * outwin;
extern BasicGraph * graphwin;
extern VariableWin * varwin;
extern BasicKeyboard * basicKeyboard;
extern Error *error;

SoundSystem *sound;


#ifdef Q_OS_WASM
// --- SAY via the browser's Web Speech API ---------------------------------
// Qt for WebAssembly ships no TextToSpeech backend at all (so
// BASIC256_ENABLE_TTS is OFF for wasm and QTextToSpeech is never
// linked/constructed); drive window.speechSynthesis directly instead. SAY is
// blocking on desktop, so speakWords() below waits until the utterance's
// onend/onerror fires and calls the basic256SayFinished() export.
//
// NB (same trap as WasmAudioSink::onended): makeDynCall does not expand
// inside EM_JS on this Qt 6.11.1/emsdk 4.0.7 toolchain, so the JS callbacks
// call the extern "C" EMSCRIPTEN_KEEPALIVE export directly, typeof-guarded.
#include <emscripten.h>
#include <emscripten/em_js.h>

static bool s_wasmSayFinished = false;

EM_JS(void, wasmSay, (const char* utf8), {
    var text = UTF8ToString(utf8);
    var synth = window.speechSynthesis;
    if (!synth) {
        // no Web Speech support -- report "finished" immediately so SAY doesn't hang
        if (typeof _basic256SayFinished !== "undefined") { _basic256SayFinished(); }
        else if (typeof Module !== "undefined" && Module._basic256SayFinished) { Module._basic256SayFinished(); }
        return;
    }
    var u = new SpeechSynthesisUtterance(text);
    var done = function() {
        // getVoices() is async (voiceschanged); we deliberately use the default
        // voice rather than block for a specific one.
        if (typeof _basic256SayFinished !== "undefined") { _basic256SayFinished(); }
        else if (typeof Module !== "undefined" && Module._basic256SayFinished) { Module._basic256SayFinished(); }
    };
    u.onend = done;
    u.onerror = done;
    synth.cancel();      // drop anything still queued/speaking
    synth.speak(u);      // first call needs a prior user gesture; Run click satisfies it
});

EM_JS(void, wasmSayCancel, (), {
    if (window.speechSynthesis) { window.speechSynthesis.cancel(); }
});

extern "C" EMSCRIPTEN_KEEPALIVE void basic256SayFinished()
{
    // Runs on the main thread (DOM callbacks always do), the same thread
    // speakWords() spins on -- no cross-thread race on the flag.
    s_wasmSayFinished = true;
}
#endif // Q_OS_WASM


RunController::RunController() {
	sound = NULL;
	speech = NULL;
	i = new Interpreter(mainwin->locale, graphwin->graphics, basicKeyboard);

	replacewin = NULL;

	//signals for the Interperter (i)
	QObject::connect(i, SIGNAL(debugNextStep()), this, SLOT(debugNextStep()));
	QObject::connect(i, SIGNAL(dialogAlert(QString)), this, SLOT(dialogAlert(QString)));
	QObject::connect(i, SIGNAL(dialogConfirm(QString, int)), this, SLOT(dialogConfirm(QString, int)));
	QObject::connect(i, SIGNAL(dialogPrompt(QString, QString)), this, SLOT(dialogPrompt(QString, QString)));
	QObject::connect(i, SIGNAL(dialogAllowPortInOut(QString)), this, SLOT(dialogAllowPortInOut(QString)));
	QObject::connect(i, SIGNAL(dialogAllowSystem(QString)), this, SLOT(dialogAllowSystem(QString)));
	QObject::connect(i, SIGNAL(dialogOpenFileDialog(QString,QString,QString)), this, SLOT(dialogOpenFileDialog(QString,QString,QString)));
	QObject::connect(i, SIGNAL(dialogSaveFileDialog(QString,QString,QString)), this, SLOT(dialogSaveFileDialog(QString,QString,QString)));

	//QObject::connect(i, SIGNAL(executeSystem(QString)), this, SLOT(executeSystem(QString)));
	QObject::connect(i, SIGNAL(goutputReady()), this, SLOT(goutputReady()));
	QObject::connect(i, SIGNAL(resizeGraphWindow(int, int, qreal)), this, SLOT(resizeGraphWindow(int, int, qreal)));
	QObject::connect(i, SIGNAL(mainWindowsVisible(int, bool)), this, SLOT(mainWindowsVisible(int, bool)));
	QObject::connect(i, SIGNAL(outputReady(QString)), this, SLOT(outputReady(QString)));
	QObject::connect(i, SIGNAL(outputError(QString)), this, SLOT(outputError(QString)));
	//QObject::connect(i, SIGNAL(stopRun()), this, SLOT(stopRun()));
	QObject::connect(i, SIGNAL(stopRunFinalized(bool)), this, SLOT(stopRunFinalized(bool)));
	QObject::connect(i, SIGNAL(speakWords(QString)), this, SLOT(speakWords(QString)));
	QObject::connect(i, SIGNAL(outputTextAt(int, int, QString)), this, SLOT(outputTextAt(int, int, QString)));

	QObject::connect(i, SIGNAL(playSound(QString, bool)), this, SLOT(playSound(QString, bool)));
	QObject::connect(i, SIGNAL(playSound(std::vector<std::vector<double>>, bool)), this, SLOT(playSound(std::vector<std::vector<double>>, bool)));
	QObject::connect(i, SIGNAL(loadSoundFromArray(QString, QByteArray*)), this, SLOT(loadSoundFromArray(QString, QByteArray*)));
	QObject::connect(i, SIGNAL(soundStop(int)), this, SLOT(soundStop(int)));
	QObject::connect(i, SIGNAL(soundPlay(int)), this, SLOT(soundPlay(int)));
	QObject::connect(i, SIGNAL(soundFade(int, double, int, int)), this, SLOT(soundFade(int, double, int, int)));
	QObject::connect(i, SIGNAL(soundVolume(int, double)), this, SLOT(soundVolume(int, double)));
	//QObject::connect(i, SIGNAL(soundExit()), this, SLOT(soundExit()));
	QObject::connect(i, SIGNAL(soundPlayerOff(int)), this, SLOT(soundPlayerOff(int)));
	QObject::connect(i, SIGNAL(soundSystem(int)), this, SLOT(soundSystem(int)));

	QObject::connect(i, SIGNAL(getClipboardImage()), this, SLOT(getClipboardImage()));
	QObject::connect(i, SIGNAL(getClipboardString()), this, SLOT(getClipboardString()));
	QObject::connect(i, SIGNAL(setClipboardImage(QImage)), this, SLOT(setClipboardImage(QImage)));
	QObject::connect(i, SIGNAL(setClipboardString(QString)), this, SLOT(setClipboardString(QString)));

	QObject::connect(i, SIGNAL(outputClear()), this, SLOT(outputClear()));
	QObject::connect(i, SIGNAL(getInput()), outwin, SLOT(getInput()));

	// for debugging and controlling the variable watch window
	QObject::connect(i, SIGNAL(varWinAssign(Variables**, int, int)), varwin,
		SLOT(varWinAssign(Variables**, int, int)), Qt::BlockingQueuedConnection);
	QObject::connect(i, SIGNAL(varWinAssign(Variables**, int, int, int, int)), varwin,
		SLOT(varWinAssign(Variables**, int, int, int, int)), Qt::BlockingQueuedConnection);
	QObject::connect(i, SIGNAL(varWinDropLevel(int)), varwin,
		SLOT(varWinDropLevel(int)), Qt::BlockingQueuedConnection);

	QObject::connect(this, SIGNAL(runHalted()), i, SLOT(runHalted()));

	QObject::connect(outwin, SIGNAL(inputEntered(QString)), this, SLOT(inputEntered(QString)));


}

RunController::~RunController() {
	if(replacewin!=NULL) replacewin->close();
	stopRun();
	i->wait();
	delete i;
}


void
RunController::speakWords(QString text) {
	// USE QtTestToSpeech API for ALL PLATFORMS
	
	// List the available engines.
	//QStringList engines = QTextToSpeech::availableEngines();
	//qDebug() << "Available engines:";
	//for (auto engine : engines) {
	//	qDebug() << "  " << engine;
	//}

	// List the available locales.
	//qDebug() << "Available locales:";
	//for (auto locale : speech->availableLocales()) {
	//	qDebug() << "  " << locale;
	//}
	// Set locale.
	//speech->setLocale(speech->availableLocales()[0]);
	// List the available voices.
	//qDebug() << "Available voices:";
	//for (auto voice : speech->availableVoices()) {
	//qDebug() << "  " << voice.name();
	//}
	// Display properties.
	//qDebug() << "Locale:" << speech->locale();
	//qDebug() << "Pitch:" << speech->pitch();
	//qDebug() << "Rate:" << speech->rate();
	//qDebug() << "Voice:" << speech->voice().name();
	//qDebug() << "Volume:" << speech->volume();
	
#ifdef Q_OS_WASM
	if (text.length() != 0) {
		// Speak via window.speechSynthesis and block (SAY is synchronous on
		// desktop) until the utterance's onend/onerror fires basic256SayFinished(),
		// honoring Stop the same way the desktop QTextToSpeech loop below does.
		s_wasmSayFinished = false;
		wasmSay(text.toUtf8().constData());
		QEventLoop loop;
		while (!s_wasmSayFinished && i && !i->isStopping() && !i->isStopped()) {
			loop.processEvents(QEventLoop::AllEvents, 50);
		}
		// Stopped mid-utterance: silence anything still speaking.
		if (!s_wasmSayFinished) {
			wasmSayCancel();
		}
	}
#elif defined(BASIC256_ENABLE_TTS)
	if (text.length() != 0) {
		// Say something.
		speech->say(text);

		// wait for speech to start or error
		if(i && !i->isStopping() && !i->isStopped() && speech && speech->state()==QTextToSpeech::Ready){
			while (speech->state()==QTextToSpeech::Ready) {
				QEventLoop *loop = new QEventLoop();
				QObject::connect(speech, SIGNAL(stateChanged(QTextToSpeech::State)), loop, SLOT(quit()));
				while(i && !i->isStopping() && !i->isStopped() && speech && speech->state() == QTextToSpeech::Ready){
					loop->processEvents(QEventLoop::AllEvents, 500);
				}
				delete (loop);
			}
		}
		// wait for started speech to finish
		if(i && !i->isStopping() && !i->isStopped() && speech && speech->state()==QTextToSpeech::Speaking){
			QEventLoop *loop = new QEventLoop();
			QObject::connect(speech, SIGNAL(stateChanged(QTextToSpeech::State)), loop, SLOT(quit()));
			while(i && !i->isStopping() && !i->isStopped() && speech && speech->state() == QTextToSpeech::Speaking){
				loop->processEvents(QEventLoop::AllEvents, 500);
			}
			delete (loop);
		}
	}
	if (speech && speech->state() == QTextToSpeech::Error) {
		qCritical() << "TTS state is Error after say() (reason" << int(speech->errorReason()) << "):" << speech->errorString();
	}
#else
	(void)text;
	error->q(ERROR_NOTAVAILABLE);
#endif
	//tell the interpreter we are finally done
	mymutex->lock();
	waitCond->wakeAll();
	mymutex->unlock();
}
 
void
RunController::executeSystem(QString text) {
	// need to implement system as a function to return process output
	// and to handle input
	// (dead code today -- its only wiring, above, is commented out -- but
	// gated the same as OP_SYSTEM/Interpreter::cleanup()'s sys->kill() since
	// Qt for WebAssembly's QProcess has a deleted default constructor: it
	// cannot be instantiated at all there, not even as a local variable.)
#ifdef BASIC256_ENABLE_PROCESS
	QProcess sy;

	QStringList parts = QProcess::splitCommand(text);
	if (!parts.isEmpty()) {
		sy.start(parts.takeFirst(), parts);
	}
	if (sy.waitForStarted()) {
		if (!sy.waitForFinished()) {
			//QByteArray result = sy.readAll();
		}
	}
#else
	(void)text;
#endif

	mymutex->lock();
	waitCond->wakeAll();
	mymutex->unlock();
}


void
RunController::startDebug() {
	if (i->isStopped()) {
		mainwin->setRunState(RUNSTATEDEBUG);
		// ensure that there is a valid editor selected
		// and use currentEditor to avoid accidental change of current editor by a lazy signal/slot mechanism
		currentEditor = editwin;
		if(!currentEditor){
			stopRunFinalized(false);
			return;
		}
		i->programTitle = currentEditor->title;
		QObject::connect(i, SIGNAL(goToLine(int)), currentEditor, SLOT(goToLine(int)));
		QObject::connect(i, SIGNAL(seekLine(int)), currentEditor, SLOT(seekLine(int)), Qt::BlockingQueuedConnection);

		i->debugMode = 1;
		outputClear();
		QDir::setCurrent(currentEditor->path);
		int result = i->compileProgram((currentEditor->toPlainText() + "\n").toUtf8().data());
		if (result < 0) {
			i->debugMode = 0;
			stopRunFinalized(false);
			return;
		}
		sound = new SoundSystem();
#ifdef BASIC256_ENABLE_TTS
		speech = new QTextToSpeech();
		QObject::connect(speech, &QTextToSpeech::errorOccurred, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
			qCritical() << "TTS error (reason" << int(reason) << "):" << errorString;
		});
#endif
		i->initialize();
		currentEditor->updateBreakPointsList();
		i->debugBreakPoints = currentEditor->breakPoints;
		//set focus to graphiscs window
		graphwin->setFocus();
		//if graphiscs window is floating
		graphwin->parentWidget()->parentWidget()->parentWidget()->parentWidget()->activateWindow();
		//if graphiscs window is hidden, then the main window will have the focus, which is ok
		varwin->clear();
		if (replacewin) replacewin->close();
		i->start();
	}
}

void
RunController::debugNextStep() {
	// show step buttons for next debug step
	mainwin->setRunState(RUNSTATEDEBUG);
}


void
RunController::startRun() {
	if (i->isStopped()) {
		mainwin->setRunState(RUNSTATERUN);
		// ensure that there is a valid editor selected
		// and use currentEditor to avoid accidental change of current editor by a lazy signal/slot mechanism
		currentEditor = editwin;
		if(!currentEditor){
			stopRunFinalized(false);
			return;
		}
		i->programTitle = currentEditor->title;
		QObject::connect(i, SIGNAL(goToLine(int)), currentEditor, SLOT(goToLine(int)));
		QObject::connect(i, SIGNAL(seekLine(int)), currentEditor, SLOT(seekLine(int)), Qt::BlockingQueuedConnection);

		i->debugMode = 0;
		outputClear();
		if (!currentEditor->path.isEmpty())
    		QDir::setCurrent(currentEditor->path);
		// Start Compile
		int result = i->compileProgram((currentEditor->toPlainText() + "\n").toUtf8().data());
		if (result < 0) {
			stopRunFinalized(false);
			return;
		}
		// if successful compile see if we need to save it
		SETTINGS;
		if(settings.value(SETTINGSIDESAVEONRUN, SETTINGSIDESAVEONRUNDEFAULT).toBool()) {
			currentEditor->saveFile(true);
		}
		//
		// now setup and start the run
		sound = new SoundSystem();
#ifdef BASIC256_ENABLE_TTS
		speech = new QTextToSpeech();
		QObject::connect(speech, &QTextToSpeech::errorOccurred, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
			qCritical() << "TTS error (reason" << int(reason) << "):" << errorString;
		});
#endif
		i->initialize();
		//set focus to graphiscs window
		graphwin->setFocus();
		//if graphiscs window is floating
		graphwin->parentWidget()->parentWidget()->parentWidget()->parentWidget()->activateWindow();
		//if graphiscs window is hidden, then the main window will have the focus, which is ok
		varwin->clear();
		if (replacewin) replacewin->close();
		i->start();
	 }
}


void
RunController::inputEntered(QString text) {
	i->setInputString(text); //set the input from user
	graphwin->setFocus();
	mymutex->lock();
	waitCond->wakeAll(); // continue OP_INPUT from interpreter
	mymutex->unlock();
}

void
RunController::outputClear() {
	mymutex->lock();
	outwin->setTextColor(Qt::black);
	outwin->clear();
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::outputReady(QString text) {
	mymutex->lock();
	if (guiState == GUISTATESILENT) {
		std::cout << text.toStdString();
		std::cout.flush();
	} else {
		mainWindowsVisible(2,true);
		outwin->outputText(text);
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::outputError(QString text) {
	mymutex->lock();
	if (guiState == GUISTATESILENT) {
		std::cerr << text.toStdString();
		std::cerr.flush();
	} else {
		mainWindowsVisible(2,true);
		outwin->outputText(text, Qt::red);
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::goutputReady() {
	mymutex->lock();
	graphwin->updateScreenImage();
	waitCond->wakeAll();
	mymutex->unlock();
	graphwin->update(); // faster than repaint()
}

void
RunController::stepThrough() {
	i->debugMode = 1; // step through debugging
	mainwin->setRunState(RUNSTATEDEBUG);
	mydebugmutex->lock();
	waitDebugCond->wakeAll();
	mydebugmutex->unlock();
}
void

RunController::stepBreakPoint() {
	i->debugMode = 2; // run to break point debugging
	mainwin->setRunState(RUNSTATERUNDEBUG);
	mydebugmutex->lock();
	waitDebugCond->wakeAll();
	mydebugmutex->unlock();
}

void RunController::stopRun() {
	//qDebug() << "in RunController::stopRun()";
	if(!i->isStopping()){
		// event when the user clicks on the stop button
		//mainwin->setRunState(RUNSTATESTOPING);

		i->setStatus(R_STOPING);//no more ops
		
		// wait for speech to stop
#ifdef Q_OS_WASM
		wasmSayCancel();
#elif defined(BASIC256_ENABLE_TTS)
		if(speech && speech->state()==QTextToSpeech::Speaking) {
			speech->stop();
		}
#endif

		// Stop being in input
		outwin->stopInput(); //make output window readonly
			
		// wake up interpreter that may be in a wait
		mymutex->lock();
		waitCond->wakeAll();
		mymutex->unlock();

		mydebugmutex->lock();
		i->debugMode = 0;
		waitDebugCond->wakeAll();
		mydebugmutex->unlock();

		// Stop any sound the interpreter is currently blocked on. A plain
		// SOUND statement blocks the interpreter thread inside
		// Sound::wait()'s own QEventLoop (waiting for that specific sound's
		// exitWaitingLoop() signal), which mymutex/waitCond above does not
		// reach at all -- previously the only thing that could stop it was
		// Interpreter::cleanup()'s sound->exit() call, which can't run until
		// Sound::wait() already returns, so Stop had no effect until that
		// wait's own 60-second safety timer expired. Calling exit() here
		// (safe: SoundSystem lives on this same thread) sets isStopping on
		// every active Sound and emits exitWaitingLoop() immediately,
		// breaking that wait right away instead of on a delay.
		if(sound) sound->exit();

		//emit(runHalted());
	}
}

void RunController::stopRunFinalized(bool ok) {
	// event when the interperter actually finishes the run
	//qDebug() << "in RunController::stopRunFinalized(" << ok << ")";
	if(sound){
		delete sound;
		sound = NULL;
	}
	QObject::disconnect(i, SIGNAL(goToLine(int)), 0, 0);
	QObject::disconnect(i, SIGNAL(seekLine(int)), 0, 0);

	mainwin->setRunState(RUNSTATESTOP);
	mainwin->ifGuiStateClose(ok);
	i->setStatus(R_STOPPED);
}

void RunController::showOnlineDocumentation() {
	QDesktopServices::openUrl(QUrl("http://doc.basic256.org"));
}

void RunController::showOnlineContextDocumentation() {
	if(editwin){
		QString w = editwin->getCurrentWord();
		QDesktopServices::openUrl(QUrl("http://doc.basic256.org/doku.php?id=en:" + w));
	}
}

void
RunController::showPreferences() {
	SETTINGS;
	QString prefpass = settings.value(SETTINGSPREFPASSWORD,"").toString();
	if (prefpass.length()!=0) {
		// Async (RULE 2): QInputDialog::getText()'s exec() never returns on
		// the WASM main thread without Asyncify -- prompt non-modally and
		// continue in the completion slot(s) instead of blocking here.
		QInputDialog *dialog = new QInputDialog(mainwin);
		dialog->setAttribute(Qt::WA_DeleteOnClose);
		dialog->setWindowTitle(tr("BASIC-256 Advanced Preferences and Settings"));
		dialog->setLabelText(tr("Password:"));
		dialog->setTextEchoMode(QLineEdit::Password);
		QObject::connect(dialog, &QInputDialog::textValueSelected, this, [this, prefpass](const QString &text){
			char *digest = MD5(text.toUtf8().data()).hexdigest();
			bool advanced = (QString::compare(digest, prefpass)==0);
			free(digest);
			showPreferencesWindow(advanced);
		});
		// Cancelling getText() used to still open Preferences non-advanced
		// (an empty string almost never matches the password digest) --
		// match that on the reject path too, rather than opening nothing.
		QObject::connect(dialog, &QDialog::rejected, this, [this](){
			showPreferencesWindow(false);
		});
		// open() forces window-modal instead of exec()'s old application-modal
		// default, which on Windows leaves the main window's title bar close
		// button live -- force it back to application-modal.
		dialog->setWindowModality(Qt::ApplicationModal);
		dialog->show();
	} else {
		showPreferencesWindow(true);
	}
}

void RunController::showPreferencesWindow(bool advanced) {
	// Async (RULE 2): QDialog::exec() never returns on the WASM main
	// thread without Asyncify -- PreferencesWin already manages its own
	// close via clickSaveButton()/clickCancelButton() calling close(), so
	// open() + WA_DeleteOnClose is a drop-in replacement, no completion
	// callback needed.
	PreferencesWin *w = new PreferencesWin(mainwin, advanced);
	w->setAttribute(Qt::WA_DeleteOnClose);
	// See the password dialog above: force application-modal, matching
	// exec()'s old default, instead of open()'s window-modal.
	w->setWindowModality(Qt::ApplicationModal);
	w->show();
}


void RunController::showReplace() {
	if (!replacewin) replacewin = new ReplaceWin();
	replacewin->setReplaceMode(true);
	if(editwin){
		QTextCursor cursor = editwin->textCursor();
		if(cursor.hasSelection()){
			replacewin->findText->setText(cursor.selectedText());
		}
		replacewin->findText->selectAll();
		replacewin->show();
		replacewin->raise();
		replacewin->activateWindow();
	}else{
		replacewin->close();
	}
}

void RunController::showFind() {
	if (!replacewin) replacewin = new ReplaceWin();
	replacewin->setReplaceMode(false);
	if(editwin){
		QTextCursor cursor = editwin->textCursor();
		if(cursor.hasSelection()){
			replacewin->findText->setText(cursor.selectedText());
		}
		replacewin->findText->selectAll();
		replacewin->show();
		replacewin->raise();
		replacewin->activateWindow();
	}else{
		replacewin->close();
	}
}

void RunController::findAgain() {
	if (!replacewin) replacewin = new ReplaceWin();
	replacewin->setReplaceMode(false);
	replacewin->show();
	replacewin->raise();
	replacewin->activateWindow();
	replacewin->findAgain();
}

void
RunController::mainWindowsVisible(int w, bool v) {
	if (w==0) {
		mainwin->editwin_visible_act->setChecked(v);
		mainwin->editwin_visible_act->triggered(v);
	}
	if (w==1) {
		mainwin->graphwin_visible_act->setChecked(v);
		mainwin->graphwin_visible_act->triggered(v);
	}
	if (w==2) {
		mainwin->outwin_visible_act->setChecked(v);
		mainwin->outwin_visible_act->triggered(v);
	}
	if (w==3) {
		mainwin->main_toolbar_visible_act->setChecked(v);
		mainwin->main_toolbar_visible_act->triggered(v);
	}
	if (w==4) {
		mainwin->graphwin_toolbar_visible_act->setChecked(v);
		mainwin->graphwin_toolbar_visible_act->triggered(v);
	}
	if (w==5) {
		mainwin->outwin_toolbar_visible_act->setChecked(v);
		mainwin->outwin_toolbar_visible_act->triggered(v);
	}
    
}

/*
void RunController::mainWindowsResize(int w, int width, int height) {
	// only resize graphics window now - may add other windows later
	mymutex->lock();
	if (w==1) graphwin->resize(width, height);
	if (w==2) outwin->resize(width, height);
	waitCond->wakeAll();
	mymutex->unlock();
}
*/

void RunController::resizeGraphWindow(int width, int height, qreal scale) {
	mymutex->lock();
	graphwin->resize(width, height, scale);
	if (guiState == GUISTATEGRAPH) {
		int canvasW = (int)(qAbs((qreal)width  * scale * graphwin->getZoom()));
		int canvasH = (int)(qAbs((qreal)height * scale * graphwin->getZoom()));
		mainwin->resizeToFitGraph(canvasW, canvasH);
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::dialogAlert(QString prompt) {
	// Async (RULE 2): QDialog::exec() never returns on the WASM main
	// thread without Asyncify. The interpreter thread is blocked in
	// waitCond->wait(mymutex) (see the emit() call site in Interpreter.cpp)
	// until wakeAll()/unlock() run -- deferred into the dialog's finished
	// completion slot instead of running immediately after exec() returns.
	mymutex->lock();
	QMessageBox *msgBox = new QMessageBox(mainwin);
	msgBox->setAttribute(Qt::WA_DeleteOnClose);
	msgBox->setText(prompt);
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	QObject::connect(msgBox, &QMessageBox::finished, this, [this](int){
		waitCond->wakeAll();
		mymutex->unlock();
	});
	// QDialog::open() forces window-modal, which on Windows does not
	// disable the main window's native title bar -- closing the main
	// window while this dialog is up would re-enter closeEvent()->
	// stopRun()->mymutex->lock() on this same thread while mymutex is
	// still held here, deadlocking the whole app. exec() used to avoid
	// this by defaulting to application-modal; set it explicitly and
	// show() instead of open() to get the same blocking without exec().
	msgBox->setWindowModality(Qt::ApplicationModal);
	msgBox->show();
}

void
RunController::dialogConfirm(QString prompt, int dflt) {
	mymutex->lock();
	QMessageBox *msgBox = new QMessageBox(mainwin);
	msgBox->setAttribute(Qt::WA_DeleteOnClose);
	msgBox->setText(prompt);
	msgBox->setStandardButtons(QMessageBox::Yes|QMessageBox::No);
	if (dflt!=-1) {
		if (dflt!=0) {
			msgBox->setDefaultButton(QMessageBox::Yes);
		} else {
			msgBox->setDefaultButton(QMessageBox::No);
		}
	}
	QObject::connect(msgBox, &QMessageBox::finished, this, [this](int result){
		i->returnInt = (result==QMessageBox::Yes) ? 1 : 0;
		waitCond->wakeAll();
		mymutex->unlock();
	});
	// See dialogAlert() above: force application-modal (exec()'s old
	// default) instead of open()'s window-modal, or the main window's
	// close button stays live and deadlocks the app on mymutex.
	msgBox->setWindowModality(Qt::ApplicationModal);
	msgBox->show();
}

void
RunController::dialogOpenFileDialog(QString prompt, QString path, QString filter) {
	mymutex->lock();
#ifdef Q_OS_WASM
	// No real filesystem path to return on WASM (see MainWindow/BasicEdit's
	// getOpenFileContent()-based file open/save) -- getOpenFileName() would
	// also hit the same exec()-not-supported abort, and its content-based
	// WASM replacement doesn't fit this opcode's "return a path string"
	// contract at all (BASIC's OPEN/READ/WRITE need a real path
	// afterwards). Report "cancelled" (empty string) rather than crash;
	// this BASIC-language file-picker opcode remains unavailable on WASM.
	(void)prompt; (void)path; (void)filter;
	i->setInputString("");
#else
	QString filename = QFileDialog::getOpenFileName(mainwin, prompt, path, filter);
	i->setInputString(filename);
#endif
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::dialogSaveFileDialog(QString prompt, QString path, QString filter) {
	mymutex->lock();
#ifdef Q_OS_WASM
	// Same reasoning as dialogOpenFileDialog() above.
	(void)prompt; (void)path; (void)filter;
	i->setInputString("");
#else
	QString filename = QFileDialog::getSaveFileName(mainwin, prompt, path, filter);
	i->setInputString(filename);
#endif
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::dialogPrompt(QString prompt, QString dflt) {
	mymutex->lock();
	QInputDialog *in = new QInputDialog(mainwin);
	in->setAttribute(Qt::WA_DeleteOnClose);
	in->setLabelText(prompt);
	in->setTextValue(dflt);
	QObject::connect(in, &QDialog::finished, this, [this, in, dflt](int result){
		if (result==QDialog::Accepted) {
			i->setInputString(in->textValue());
		} else {
			i->setInputString(dflt);
		}
		waitCond->wakeAll();
		mymutex->unlock();
	});
	// See dialogAlert() above: force application-modal (exec()'s old
	// default) instead of open()'s window-modal, or the main window's
	// close button stays live and deadlocks the app on mymutex.
	in->setWindowModality(Qt::ApplicationModal);
	in->show();
}

void RunController::playSound(std::vector<std::vector<double>> sounddata, bool player){
	mymutex->lock();
	sound->playSound(sounddata, player);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::playSound(QString s, bool player){
	mymutex->lock();
	sound->playSound(s, player);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::loadSoundFromArray(QString s, QByteArray* arr){
	mymutex->lock();
	sound->loadSoundFromArray(s, arr);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundStop(int n){
	mymutex->lock();
	sound->stop(n);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundPlayerOff(int n){
	mymutex->lock();
	sound->playerOff(n);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundPlay(int n){
	mymutex->lock();
	sound->play(n);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundFade(int n, double v, int ms, int delay){
	mymutex->lock();
	sound->fade(n, v, ms, delay);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundVolume(int n, double v){
	mymutex->lock();
	sound->volume(n, v);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::soundSystem(int n){
	mymutex->lock();
	sound->system(n);
	waitCond->wakeAll();
	mymutex->unlock();
}


//void RunController::soundExit(){
//    mymutex->lock();
//    qDebug() << "soundExit ";
//    sound->exit();
//    waitCond->wakeAll();
//    mymutex->unlock();
//}

void RunController::dialogAllowPortInOut(QString msg) {
	mymutex->lock();
	QMessageBox message(mainwin);
	message.setWindowTitle(tr("Confirmation"));
	message.setText(tr("Do you want to allow a PORTIN/PORTOUT command?"));
	message.setInformativeText(msg);
	message.setIcon(QMessageBox::Warning);
//  message.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore);
	message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	message.setDefaultButton(QMessageBox::No);
	QCheckBox *check=new QCheckBox (tr("Do not ask me again"));
	message.setCheckBox(check);
	int ret = message.exec();
	if (ret==QMessageBox::Yes) {
		i->returnInt = SETTINGSALLOWYES;
		if(message.checkBox()->isChecked()) i->settingsAllowPort = SETTINGSALLOWYES; // no further conf needed
	} else if (ret==QMessageBox::No){
		i->returnInt = SETTINGSALLOWNO;
		if(message.checkBox()->isChecked()) i->settingsAllowPort = SETTINGSALLOWNO;
//  } else if (ret==QMessageBox::Ignore){
//      i->returnInt = -1;
//      if(message.checkBox()->isChecked()) i->settingsAllowPort = -1;
	} else {
		i->returnInt = SETTINGSALLOWNO;
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::dialogAllowSystem(QString msg) {
	mymutex->lock();
	QMessageBox message(mainwin);
	message.setWindowTitle(tr("Confirmation"));
	message.setText(tr("Do you want to allow a SYSTEM command?"));
	if(msg.length()>50){
		message.setDetailedText(msg);
		msg.truncate(45);
		msg.append("...");
		message.setInformativeText(msg);
	}else{
		message.setInformativeText(msg);
	}
	message.setIcon(QMessageBox::Warning);
//  message.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore);
	message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	message.setDefaultButton(QMessageBox::No);
	QCheckBox *check=new QCheckBox (tr("Do not ask me again"));
	message.setCheckBox(check);
	int ret = message.exec();
	if (ret==QMessageBox::Yes) {
		i->returnInt = SETTINGSALLOWYES;
		if(message.checkBox()->isChecked()) i->settingsAllowSystem = SETTINGSALLOWYES; // no further conf needed
	} else if (ret==QMessageBox::No){
		i->returnInt = SETTINGSALLOWNO;
		if(message.checkBox()->isChecked()) i->settingsAllowSystem = SETTINGSALLOWNO;
//  } else if (ret==QMessageBox::Ignore){
//      i->returnInt = -1;
//      if(message.checkBox()->isChecked()) i->settingsAllowSystem = -1;
	} else {
		i->returnInt = SETTINGSALLOWNO;
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::getClipboardImage(){
	mymutex->lock();
	QClipboard *clipboard = QGuiApplication::clipboard();
	i->returnImage = clipboard->image();
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::getClipboardString(){
	mymutex->lock();
	QClipboard *clipboard = QGuiApplication::clipboard();
	i->setInputString(clipboard->text());
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::setClipboardImage(QImage img){
	mymutex->lock();
	QClipboard *clipboard = QGuiApplication::clipboard();
	clipboard->setImage(img);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::setClipboardString(QString s){
	mymutex->lock();
	QClipboard *clipboard = QGuiApplication::clipboard();
	clipboard->setText(s);
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::outputTextAt(int c, int r, QString s){
	mymutex->lock();
	outwin->outputTextAt(c, r, s);
	waitCond->wakeAll();
	mymutex->unlock();
}

