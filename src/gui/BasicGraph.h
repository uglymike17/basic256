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


#ifndef __BASICGRAPH_H
#define __BASICGRAPH_H

#include <stdio.h>

#include <QtWidgets/QWidget>
#include <QtWidgets/QToolBar>
#include <QPainter>
#include <QKeyEvent>
#include "ViewWidgetIFace.h"
#include "GraphicsBuffer.h"

class BasicGraph : public QWidget, public ViewWidgetIFace
{
  Q_OBJECT
 public:
  BasicGraph();
  ~BasicGraph();
  // Owns the pixel buffers, sprite state, and mouse/click state -- this
  // widget is just the view onto it (paints displayedimage, feeds mouse
  // events in). The interpreter talks to this directly, not to us.
  GraphicsBuffer *graphics;
  QImage *gridlinesimage;
  bool initActions(QMenu *, QToolBar *);
  bool isVisibleGridLines();
  void updateScreenImage();
  QAction *copyAct;
  QAction *printAct;
  QAction *clearAct;


 public slots:
  void resize(int, int, qreal);
  void slotGridLines(bool);
  void slotCopy();
  void slotPrint();
  void slotClear();
  void slotSetZoom(double);
  double getZoom();

 protected:
  void paintEvent(QPaintEvent *);
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);
  void leaveEvent(QEvent *);
  void mousePressEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent * );
  void focusOutEvent(QFocusEvent* );

 private:
  int gwidth;
  int gheight;
  qreal gscale;
  qreal gzoom;
  qreal oldzoom;
  bool gridlines;		// show the grid lines or not
  void drawGridLines();
  QTransform gtransform;
  QTransform gtransforminverted;
  void setTrasformationMaps();
  void resizeWindowToFitContent();
};


#endif
