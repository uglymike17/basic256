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
	saveLastPosition();

}

BasicOutput::~BasicOutput( ) {
    // destructor for basic output
}

void BasicOutput::getInput() {
	// move cursor to the end of the existing text and start input
	inputText.clear();
	gettingInput = true;
	setFocus();
	emit(mainWindowsVisible(2,true));
	restoreLastPosition();
	inputPosition = lastPosition;
	setReadOnly(false);
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
            saveLastPosition();
            QTextCursor t(textCursor());
            t.setPosition(inputPosition);
            t.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            t.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, lastPosition);
            inputText=t.selectedText();
            restoreLastPosition();
            insertPlainText("\n");
            saveLastPosition();
            stopInput();
            emit(inputEntered(inputText)); // send the string back to the interperter and run controller

       } else if (e->key() == Qt::Key_Backspace) {
            QTextCursor t(textCursor());
            t.movePosition(QTextCursor::PreviousCharacter);
            if (t.position() >= inputPosition)
                QTextEdit::keyPressEvent(e);
                saveLastPosition();
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

void BasicOutput::paintEvent(QPaintEvent* event) {
	// paint a visible cursor at the text cursor
	QTextEdit::paintEvent(event);
	QRect cursor = cursorRect();
	cursor.setWidth(2);
	QPainter p(viewport());
	p.fillRect(cursor, Qt::SolidPattern);
}

// Ensure that drag and drop is allowed only in permitted area when BASIC-256 wait for input
void BasicOutput::dragEnterEvent(QDragEnterEvent *e){
	if (e->mimeData()->hasFormat("text/plain") && gettingInput && !isReadOnly()  )
		e->acceptProposedAction();
}

void BasicOutput::dragMoveEvent (QDragMoveEvent *event){
	QTextCursor t = cursorForPosition(event->pos());
	if (t.position() >= inputPosition){
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
     lastPosition = 0;
}

void BasicOutput::slotWrap(bool checked) {
	if (checked) {
		setLineWrapMode(QTextEdit::WidgetWidth);
	} else {
		setLineWrapMode(QTextEdit::NoWrap);
	}
}

void BasicOutput::outputText(QString text) {
	outputText(text, Qt::black);
}

void BasicOutput::outputText(QString text, QColor color) {
	this->setTextColor(color); //back to black color
	restoreLastPosition();
	this->insertPlainText(text);
	this->ensureCursorVisible();
	saveLastPosition();
}

int BasicOutput::getCurrentPosition() {
		QTextCursor t(textCursor());
		return t.position();
}

void BasicOutput::saveLastPosition() {
	lastPosition = getCurrentPosition();
}

void BasicOutput::restoreLastPosition() {
	moveToPosition(lastPosition);
}

void BasicOutput::moveToPosition(int pos) {
	// move to an absolute character number (position) in the document
	QTextCursor t(textCursor());
	t.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
	t.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, pos);
	setTextCursor(t);
}

void BasicOutput::outputTextAt(int col, int row, QString s) {
	//fprintf(stderr, "moveToPosition = col %i row %i\n", col, row);

	QTextCursor t(textCursor());
	t.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

	//fprintf(stderr, "moveToPosition start=%i\n", t.position());

	// move to the begining of the sprecified line or append lines
	int lines = toPlainText().count("\n");
	//fprintf(stderr, "moveToPosition lines=%i\n", lines);
	if (row>lines) {
		// go to end and append
		for (; lines < row; lines++) {
			t.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
			this->setTextCursor(t);
			insertPlainText("\n");
			//fprintf(stderr, "moveToPosition add line\n", lines);
		}
	} else {
		// go down to the row
		for (int i=0; i < row; i++) {
			t.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
			//fprintf(stderr, "moveToPosition down line\n", lines);
		}
		this->setTextCursor(t);
	}
	//fprintf(stderr, "moveToPosition after position=%i\n", t.position());

	// move to the specified character on the current line or append
	t.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
	int lineStart = t.position();
	t.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
	int lineEnd = t.position();
	//fprintf(stderr, "moveToPosition = ls %i le %i\n", lineStart, lineEnd);


	if (col <= lineEnd-lineStart) {
		// line is long enough to start - replace mode
		t.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		t.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);
		this->setTextCursor(t);
		
		// replace text at cursor
		int startText = t.position();
		t.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, s.length());
		int endLength = t.position();
		t.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
		t.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startText);
		t.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
		int endLine = t.position();

		int replaceLen= (endLine<endLength?endLine:endLength) - startText;
		//fprintf(stderr, "moveToPosition = replace s %i len %i line %i replaceLen %i\n", startText, endLength, endLine, replaceLen);
		
		t.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
		t.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startText);
		t.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, replaceLen);
		this->setTextCursor(t);
		this->insertPlainText(s);
		
	} else {
		// line is not long enough - insert spaces and insert
		t.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
		this->setTextCursor(t);
		for (int i=lineEnd-lineStart; i<col; i++) {
			insertPlainText(" ");
		}
		this->insertPlainText(s);
	}

	saveLastPosition();	
	//fprintf(stderr, "moveToPosition = -----------------------------------\n");

}





