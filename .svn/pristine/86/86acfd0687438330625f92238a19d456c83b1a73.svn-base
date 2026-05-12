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


#ifndef __BASICOUTPUT_H
#define __BASICOUTPUT_H

#include <QKeyEvent>
#include <QPaintEvent>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>

#include <qglobal.h>

#include "ViewWidgetIFace.h"

class BasicOutput : public QTextEdit, public ViewWidgetIFace
{
  Q_OBJECT
 public:
  BasicOutput();
  ~BasicOutput();
  
  char *inputString;
  int currentKey;			// store the last key pressed for key function
  void inputStart();
  QAction *copyAct;
  QAction *pasteAct;
  QAction *printAct;
  QAction *clearAct;

  virtual bool initActions(QMenu *, QToolBar *);

 public slots:
  void getInput();
  void stopInput();
  void slotPrint();
  void cursorChanged();
  void updatePasteButton();
  void slotClear();

 signals:
  void inputEntered(QString);
  void mainWindowsVisible(int, bool);
  
 protected:
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);
  void dragEnterEvent(QDragEnterEvent *e);
  void dragMoveEvent(QDragMoveEvent *e);
  void insertFromMimeData(const QMimeData *source);
  void focusOutEvent(QFocusEvent* );

 private:
  int startPos;
  bool gettingInput;
  void changeFontSize(unsigned int);
  QString inputText;
};


#endif
