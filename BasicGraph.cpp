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

#include <QClipboard>
#include <QMutex>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QDockWidget>
#include <QDesktopWidget>

#include "BasicWidget.h"
#include "BasicGraph.h"
#include "Constants.h"
#include "BasicKeyboard.h"
#include "Settings.h"

extern QMutex *mymutex;
extern BasicKeyboard *basicKeyboard;


BasicGraph::BasicGraph() {
    SETTINGS;
    double z = settings.value(SETTINGSZOOM, SETTINGSZOOMDEFAULT).toDouble();
    if(z==0.25 ||z==0.5 || z==1.0 || z==2.0 || z==3.0 || z==4.0){
        gzoom=z;
    }else{
        gzoom=1.0;
    }
    image = NULL;
    gridlinesimage = NULL;
    displayedimage = NULL;
    spritesimage = NULL;
    sprites_clip_region = QRegion(0,0,0,0);
    gridlines = false;
    draw_sprites_flag = false;
    gwidth = 0;
    gheight = 0;
    gscale = 1.0;
    oldzoom = 0.0;
    gtransform.scale(gscale,gscale);
    gtransforminverted=gtransform.inverted();
    setFocusPolicy(Qt::StrongFocus);
    resize(GSIZE_INITIAL_WIDTH, GSIZE_INITIAL_HEIGHT, 1.0);
    setMouseTracking(true);
}

BasicGraph::~BasicGraph() {
    if (image) {
        delete image;
        image = NULL;
    }
    if (gridlinesimage) {
        delete gridlinesimage;
        gridlinesimage = NULL;
    }
    if (displayedimage) {
        delete displayedimage;
        displayedimage = NULL;
    }
    if (spritesimage) {
        delete spritesimage;
        spritesimage = NULL;
    }
}

void BasicGraph::resize(int width, int height, qreal scale) {
    if ((width == gwidth && height == gheight && gscale == scale && gzoom == oldzoom)|| width<=0 || height<=0){
        resizeWindowToFitContent();
        return;
    }
    gwidth = width;
    gheight = height;
    gscale = scale;
    oldzoom = gzoom;

    // set transformation maps
    setTrasformationMaps();

    // delete the old image and then create a new one the right size
    if(image){
        QImage old_image = image->copy(0,0,width,height);
        image->swap(old_image);
    }else{
        image = new QImage(width, height, QImage::Format_ARGB32);
        image->fill(Qt::transparent);
    }

    // delete displayed image and then create a new one the right size
    if(displayedimage){
        QImage old_displayedimage = displayedimage->copy(0,0,width,height);
        displayedimage->swap(old_displayedimage);
    }else{
        displayedimage = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
        displayedimage->fill(Qt::transparent);
    }


    // delete sprites image and then create a new one the right size
    if(spritesimage){
        delete spritesimage;
        spritesimage = NULL;
    }
    spritesimage = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    spritesimage->fill(Qt::transparent);


    // delete the old gridlinesimage if exist and draw new one if required
    if(gridlinesimage){
        delete gridlinesimage;
        gridlinesimage = NULL;
    }
    if (gridlines)
        drawGridLines();


    mouseX = 0;
    mouseY = 0;
    mouseB = 0;
    clickX = 0;
    clickY = 0;
    clickB = 0;

    resizeWindowToFitContent();
}

void BasicGraph::setTrasformationMaps() {
    if(gscale<0.0){
        gtransform = QTransform().translate(-(gwidth-1)*gscale*gzoom, -(gheight-1)*gscale*gzoom);
        gtransform.scale(gscale*gzoom,gscale*gzoom);
    }else{
        gtransform = QTransform::fromScale(gscale*gzoom,gscale*gzoom);
    }
    gtransforminverted=gtransform.inverted();
}

void BasicGraph::resizeWindowToFitContent() {
// NOT android - Android - FULL screen
#ifndef ANDROID
    int winw = labs((qreal)gwidth * gscale * gzoom);
    int winh = labs((qreal)gheight * gscale * gzoom);
    setFixedSize(winw, winh);

    //force resize
    //if graph window is floating, then also resize window
    //graph-scroll-basicwidget-dock
    if(parentWidget()){
        QDockWidget * dock = (QDockWidget *) parentWidget()->parentWidget()->parentWidget()->parentWidget();
        if(dock->isFloating()){
            QScrollArea * scroll = (QScrollArea *)parentWidget()->parentWidget();
            if (scroll) {
                QWidget *basicwidget = parentWidget()->parentWidget()->parentWidget();
                scroll->setFixedSize(winw,winh);
                basicwidget->setFixedSize(winw,winh);
                dock->setFixedSize(winw,winh);

                scroll->setMinimumSize(0,0);
                basicwidget->setMinimumSize(0,0);
                dock->setMinimumSize(0,0);

                scroll->setMaximumSize(QWIDGETSIZE_MAX ,QWIDGETSIZE_MAX );
                basicwidget->setMaximumSize(QWIDGETSIZE_MAX ,QWIDGETSIZE_MAX );
                dock->setMaximumSize(QWIDGETSIZE_MAX ,QWIDGETSIZE_MAX );

                // make graph window visible in screen range (ignoring taskbar area)
                QRect screen (QApplication::desktop()->availableGeometry(this));
                QPoint win_position = dock->pos();
                QSize win_size = dock->size();
                int w = win_size.width()+win_position.x();
                int h = win_size.height()+win_position.y();
                int sw = screen.width() + screen.x();
                int sh = screen.height() + screen.y();
                if (w > sw) win_position.setX(win_position.x() - (w - sw));
                if (h > sh) win_position.setY(win_position.y() - (h - sh));
                if (win_position.x() < screen.x() ) win_position.setX(screen.x());
                if (win_position.y() < screen.y() ) win_position.setY(screen.y());
                dock->move(win_position);
            }
        }
    }
#endif
}

bool BasicGraph::isVisibleGridLines() {
    return gridlines;
}

void BasicGraph::paintEvent(QPaintEvent *e) {
    QPainter painter(this);
    painter.setTransform(gtransform);
    QRect r = gtransforminverted.mapRect(e->rect());
    painter.drawImage(r, *displayedimage, r);
    if (gridlines) {
        if(!gridlinesimage) drawGridLines();
        painter.drawImage(r, *gridlinesimage, r);
    }
}


void BasicGraph::keyPressEvent(QKeyEvent *e) {
    e->accept();
    mymutex->lock();
    basicKeyboard->keyPressed(e);
    mymutex->unlock();
}

void BasicGraph::keyReleaseEvent(QKeyEvent *e) {
    e->accept();
    mymutex->lock();
    basicKeyboard->keyReleased(e);
    mymutex->unlock();
}

void BasicGraph::focusOutEvent(QFocusEvent* ){
    //clear pressed keys list when lose focus to avoid detecting still pressed keys
    basicKeyboard->reset();
}

void BasicGraph::leaveEvent(QEvent *) {
	// reset mouse to off widget when we leave
	mouseX = -1;
	mouseY = -1;
	mouseB = 0;
}

void BasicGraph::mouseMoveEvent(QMouseEvent *e) {
	static int c = Qt::ArrowCursor;
    QPoint p = gtransforminverted.map(e->pos());
    int x = p.x();
    int y = p.y();

    if (x >= 0 && x < gwidth && y >= 0 && y < gheight) {
        mouseX = x;
        mouseY = y;
		mouseB = e->buttons();

		if(gridlines){
			this->setToolTip(QString::number( mouseX ) + ", " + QString::number( mouseY ));

			if(c!=Qt::CrossCursor){
				this->setCursor(Qt::CrossCursor);
				c=Qt::CrossCursor;
			}
		}else{
			if(c!=Qt::ArrowCursor){
				this->setCursor(Qt::ArrowCursor);
				this->setToolTip("");
				c=Qt::ArrowCursor;
			}
		}
	}else{
		if(c!=Qt::ArrowCursor){ //if leave the image area, then do that only once
			this->setToolTip("");
			this->setCursor(Qt::ArrowCursor);
			c=Qt::ArrowCursor;
		}
	}
}

void BasicGraph::mouseReleaseEvent(QMouseEvent *e) {
    // cascade call to mouse move so we record clicks real time like moves
    mouseMoveEvent(e);
}

void BasicGraph::mousePressEvent(QMouseEvent *e) {
    if (e->x() >= 0 && e->x() < gwidth && e->y() >= 0 && e->y() < gheight) {
        QPoint p = gtransforminverted.map(e->pos());
        clickX = mouseX = p.x();
        clickY = mouseY = p.y();
		clickB = e->button();
		mouseB = e->buttons();
	}
}

bool BasicGraph::initActions(QMenu * vMenu, QToolBar * vToolBar) {
	if ((NULL == vMenu) || (NULL == vToolBar)) {
		return false;
	}

	vToolBar->setObjectName("graphtoolbar");

    copyAct = vMenu->addAction(QObject::tr("Copy"));
    printAct = vMenu->addAction(QObject::tr("Print"));
    clearAct = vMenu->addAction(QObject::tr("Clear"));

	vToolBar->addAction(copyAct);
	vToolBar->addAction(printAct);
    vToolBar->addAction(clearAct);

	QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(slotCopy()));
	QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
    QObject::connect(clearAct, SIGNAL(triggered()), this, SLOT(slotClear()));

	m_usesToolBar = true;
	m_usesMenu = true;

	return true;
}

void BasicGraph::slotGridLines(bool visible) {
	gridlines = visible;
	update();
}

void BasicGraph::slotCopy() {
	QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(*displayedimage);
	QApplication::processEvents();
}

void BasicGraph::slotPrint() {

#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Graphics Output"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted)) {
            QPainter painter(&printer);
            QRect rect = painter.viewport();
            QSize size = displayedimage->size();
            size.scale(rect.size(), Qt::KeepAspectRatio);
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(displayedimage->rect());
            painter.drawImage(0, 0, *displayedimage);
        } else {
            QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }
    }
#endif

}


void BasicGraph::drawGridLines(){
    gridlinesimage = new QImage(gwidth, gheight, QImage::Format_ARGB32_Premultiplied);
    gridlinesimage->fill(Qt::transparent);
    int tx, ty;

    QPainter painter(gridlinesimage);

    const QColor darkColor = QColor(64,64,64,255);
    const QColor lightColor = QColor(128,128,128,255);
    painter.setFont(QFont("Sans", 6, 100));

    painter.setPen(lightColor);
    for(tx=10; tx<gwidth; tx=tx+10) if (tx%100!=0) painter.drawLine(tx, 0, tx, gheight);
    for(ty=10; ty<gheight; ty=ty+10) if (ty%100!=0) painter.drawLine(0, ty, gwidth, ty);
    painter.setPen(darkColor);
    for(tx=0; tx<gwidth; tx=tx+100) painter.drawLine(tx, 0, tx, gheight);
    for(ty=0; ty<gheight; ty=ty+100) painter.drawLine(0, ty, gwidth, ty);

    const int fontAscent = (QFontMetrics(painter.font()).ascent())+1;
    char buffer[64];
    for(tx=0; tx<gwidth; tx=tx+100) {
        for(ty=0; ty<gheight; ty=ty+100) {
            sprintf(buffer, "%u,%u", tx, ty);
            painter.drawText(tx+2, ty+fontAscent, buffer);
        }
    }
}

void BasicGraph::updateScreenImage(){
    if(draw_sprites_flag){
        QImage tmp = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QRectF target(0.0, 0.0, tmp.width(), tmp.height() );
        QPainter painter;
        painter.begin(&tmp);
        painter.setClipRegion(sprites_clip_region);
        painter.drawImage(target, *spritesimage);
        painter.end();
        displayedimage->swap(tmp);
    }else{
        *displayedimage = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
}

void BasicGraph::mouseDoubleClickEvent(QMouseEvent * e){
    if (e->x() >= 0 && e->x() < gwidth && e->y() >= 0 && e->y() < gheight) {
        clickX = mouseX = e->x();
        clickY = mouseY = e->y();
        clickB = e->button() | MOUSEBUTTON_DOUBLECLICK; //set doubleclick flag
        mouseB = e->buttons();
    }
}

void BasicGraph::slotClear(){
    displayedimage->fill(Qt::transparent);
    repaint();
}

void BasicGraph::slotSetZoom(double z){
    if(z==0.25 ||z==0.5 || z==1.0 || z==2.0 || z==3.0 || z==4.0){
        gzoom=z;
    }else{
        gzoom=1.0;
    }
    setTrasformationMaps();
    resizeWindowToFitContent();
}

double BasicGraph::getZoom(){
    return gzoom;
}
