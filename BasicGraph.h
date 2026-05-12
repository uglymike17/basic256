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

#define GSIZE_INITIAL_WIDTH   300
#define GSIZE_INITIAL_HEIGHT   300

class BasicGraph : public QWidget, public ViewWidgetIFace
{
  Q_OBJECT
 public:
  BasicGraph();
  ~BasicGraph();
  QImage *image;
  QImage *gridlinesimage;
  QImage *displayedimage;
  QImage *spritesimage;
  QRegion sprites_clip_region;
  bool draw_sprites_flag;
  bool initActions(QMenu *, QToolBar *);
  // used to store current location of mouse
  // default value of -1 when no mouse recorded over graphic output
  int mouseX;
  int mouseY;
  int mouseB;
  // used to store location of last mouse click
  // default value of -1 when no click recorded
  int clickX;
  int clickY;
  int clickB;
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
