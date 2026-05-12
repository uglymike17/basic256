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

#include <iostream>

#include <QPainter>
#include <QTextCursor>
#include <QMutex>
#include <QClipboard>
#include <QMimeData>

#include <QtWidgets/QAction>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include "Settings.h"
#include "BasicOutput.h"
#include "BasicKeyboard.h"

extern QMutex *mymutex;
extern BasicKeyboard *basicKeyboard;

BasicOutput::BasicOutput( ) : QTextEdit () {
    inputText.clear();
    setReadOnly(true);
	setInputMethodHints(Qt::ImhNoPredictiveText);
	setFocusPolicy(Qt::StrongFocus);
	setAcceptRichText(false);
	setUndoRedoEnabled(false);
	gettingInput = false;
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorChanged()));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(cursorChanged()));
}

BasicOutput::~BasicOutput( ) {
    // destructor for basic output
}

void BasicOutput::getInput() {
    // move cursor to the end of the existing text and start input
    inputText.clear();
    gettingInput = true;
	emit(mainWindowsVisible(2,true));
	QTextCursor t(textCursor());
	t.movePosition(QTextCursor::End);
	setTextCursor(t);
	startPos = t.position();
	setReadOnly(false);
	setFocus();
    updatePasteButton();
}

void BasicOutput::stopInput() {
	gettingInput = false;
	setReadOnly(true);
    updatePasteButton();
}


void BasicOutput::keyPressEvent(QKeyEvent *e) {
    e->accept();
    if (!gettingInput) {
        mymutex->lock();
        basicKeyboard->keyPressed(e);
        QTextEdit::keyPressEvent(e);
		mymutex->unlock();
    } else {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            QTextCursor t(textCursor());
            t.setPosition(startPos);
            t.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            inputText=t.selectedText();
            t.movePosition(QTextCursor::End);
            setTextCursor(t);
            insertPlainText("\n");
            stopInput();
            emit(inputEntered(inputText)); // send the string back to the interperter and run controller

       } else if (e->key() == Qt::Key_Backspace) {
            QTextCursor t(textCursor());
            t.movePosition(QTextCursor::PreviousCharacter);
            if (t.position() >= startPos)
                QTextEdit::keyPressEvent(e);
        } else {
            QTextEdit::keyPressEvent(e);
        }
    }
}


void BasicOutput::keyReleaseEvent(QKeyEvent *e) {
	e->accept();
	if (!gettingInput) {
        mymutex->lock();
        basicKeyboard->keyReleased(e);
        QTextEdit::keyReleaseEvent(e);
		mymutex->unlock();
	}
}

void BasicOutput::focusOutEvent(QFocusEvent* ){
    //clear pressed keys list when lose focus to avoid detecting still pressed keys
    basicKeyboard->reset();
}

bool BasicOutput::initActions(QMenu * vMenu, QToolBar * vToolBar) {
	if ((NULL == vMenu) || (NULL == vToolBar)) {
		return false;
	}

	vToolBar->setObjectName("outtoolbar");


    copyAct = vMenu->addAction(QObject::tr("Copy"));
    copyAct->setShortcutContext(Qt::WidgetShortcut);
    copyAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Copy));
    copyAct->setEnabled(false);
    pasteAct = vMenu->addAction(QObject::tr("Paste"));
    pasteAct->setShortcutContext(Qt::WidgetShortcut);
    pasteAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Paste));
    pasteAct->setEnabled(false);
    printAct = vMenu->addAction(QObject::tr("Print"));
    printAct->setShortcutContext(Qt::WidgetShortcut);
    printAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));
    clearAct = vMenu->addAction(QObject::tr("Clear"));
    clearAct->setEnabled(false);

	vToolBar->addAction(copyAct);
	vToolBar->addAction(pasteAct);
    vToolBar->addAction(printAct);
    vToolBar->addAction(clearAct);

	QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));
	QObject::connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
	QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
    QObject::connect(this, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updatePasteButton()));
    QObject::connect(clearAct, SIGNAL(triggered()), this, SLOT(slotClear()));

	m_usesToolBar = true;
	m_usesMenu = true;

	return true;
}

void BasicOutput::slotPrint() {
#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else
    QTextDocument *document = this->document();
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Text Output"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted))	{
            document->print(&printer);
        } else {
            QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }
    }
#endif
}


//User can navigate or select text only in permitted area when BASIC-256 wait for input
void BasicOutput::cursorChanged() {
	if (gettingInput) {
		QTextCursor t(textCursor());
		int position=t.position();
		int anchor=t.anchor();
		if (position < startPos || anchor < startPos) {
			if (position < startPos) position = startPos;
			if (anchor < startPos) anchor = startPos;
			t.setPosition(anchor, QTextCursor::MoveAnchor);
			t.setPosition(position, QTextCursor::KeepAnchor);
			setTextCursor(t);
		}
	}
}


// Ensure that drag and drop is allowed only in permitted area when BASIC-256 wait for input
void BasicOutput::dragEnterEvent(QDragEnterEvent *e){
	if (e->mimeData()->hasFormat("text/plain") && gettingInput && !isReadOnly()  )
		e->acceptProposedAction();
}

void BasicOutput::dragMoveEvent (QDragMoveEvent *event){
	QTextCursor t = cursorForPosition(event->pos());
	if (t.position() >= startPos){
		event->acceptProposedAction();
		QDragMoveEvent move(event->pos(),event->dropAction(), event->mimeData(), event->mouseButtons(),
			event->keyboardModifiers(), event->type());
		QTextEdit::dragMoveEvent(&move); // Call the parent function (show cursor and keep selection)
	} else {
		event->ignore();
	}
}


//Ensure that drang and drop operation or paste operation will add only first line of the copied text.
void BasicOutput::insertFromMimeData(const QMimeData* source)
{
	if (source->hasText()) {
		QString s = source->text();
		QStringList l = s.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
		textCursor().insertText(l.at(0));
		setFocus();
	}
}

void BasicOutput::updatePasteButton(){
     pasteAct->setEnabled(this->canPaste());
}

void BasicOutput::slotClear(){
     clearAct->setEnabled(false);
     clear();
}

