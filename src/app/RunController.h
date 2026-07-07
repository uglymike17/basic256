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


#ifndef __RUNCONTROLLER_H
#define __RUNCONTROLLER_H

#include <qglobal.h>

#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtTextToSpeech/QTextToSpeech>
#include <QtTextToSpeech/QVoice>
#include <QThread>
#include <QLocale>

#include "BasicEdit.h"
#include "BasicOutput.h"
#include "BasicGraph.h"
#include "Interpreter.h"
#include "ReplaceWin.h"

class RunController : public QObject
{
  Q_OBJECT
 public:
  RunController();
  ~RunController();
  ReplaceWin *replacewin;
  
 signals:
  void runHalted();
 
 public slots:
  void speakWords(QString);
  void executeSystem(QString);
  void inputEntered(QString text);
  void outputReady(QString text);
  void outputError(QString text);
  void outputClear();
  void outputTextAt(int, int, QString);
  void goutputReady();
  void resizeGraphWindow(int, int, qreal);
  void startDebug();
  void debugNextStep();
  void startRun();
  void stopRun();				// user pressed the stop button
  void stopRunFinalized(bool ok);		// called when interperter finally finished stoprun and pass if there was an error or not
  void stepThrough();
  void stepBreakPoint();
  void showOnlineDocumentation();
  void showOnlineContextDocumentation();
  void showPreferences();
  void showReplace();
  void showFind();
  void findAgain();
  void mainWindowsVisible(int, bool);
  void dialogAlert(QString);
  void dialogConfirm(QString, int);
  void dialogPrompt(QString, QString);
  void dialogAllowPortInOut(QString);
  void dialogAllowSystem(QString);
  void dialogOpenFileDialog(QString, QString, QString);
  void dialogSaveFileDialog(QString, QString, QString);
  void playSound(QString, bool);
  void playSound(std::vector<std::vector<double>>, bool);
  void loadSoundFromArray(QString, QByteArray*);
  void soundStop(int);
  void soundPlay(int);
  void soundFade(int, double, int, int);
  void soundVolume(int, double);
  //void soundExit();
  void soundPlayerOff(int);
  void soundSystem(int);
  void getClipboardImage();
  void getClipboardString();
  void setClipboardImage(QImage);
  void setClipboardString(QString);
  

 private:
  Interpreter *i;
  bool paused;
  run_status oldStatus;
  QString bytefilename;
  QLocale *locale;
  BasicEdit *currentEditor;
  QTextToSpeech *speech;
};



#endif
