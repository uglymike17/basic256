/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 **/

// Graphics opcodes.
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execGraphicsOp().  The cases are exactly as they were
// there: a break still ends the opcode, and the checks execByteCode()
// makes after every opcode still run when this returns.

#include "InterpreterPrivate.h"

void Interpreter::execGraphicsOp(int opcode) {
	switch(opcode) {

		case OP_SETCOLOR: {
			QColor brushcolor = stack->popQColor();
			QColor pencolor = stack->popQColor();
			if(pencolor!=painter_pen_color) {
				drawingpen.setColor(pencolor);
				painter_pen_need_update=true;
				painter_pen_color=pencolor;
				//set PenColorIsClear and CompositionModeClear flags here for speed
				if (pencolor == Qt::transparent){
					PenColorIsClear = true;
					if (brushcolor == Qt::transparent){
						CompositionModeClear = true;
					} else {
						CompositionModeClear = false;
					}
				} else {
					PenColorIsClear = false;
					CompositionModeClear = false;
				}
			}
			if(brushcolor!=painter_brush_color) {
				drawingbrush.setColor(brushcolor);
				painter_brush_need_update=true;
				painter_brush_color=brushcolor;
			}
		}
		break;

		case OP_RGB: {
			int aval = stack->popInt();
			int bval = stack->popInt();
			int gval = stack->popInt();
			int rval = stack->popInt();
			if (((rval | gval | bval | aval)&(~0xff))!=0) {
			//if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255 || aval < 0 || aval > 255) {
				error->q(ERROR_RGB);
				stack->pushLong(0);
			} else {
				stack->pushInt( (int) QColor(rval,gval,bval,aval).rgba());
			}
		}
		break;

		case OP_HSV: {
			// hue in degrees 0-360 (360 is red again, the same as 0),
			// saturation, value and alpha in percent 0-100; fractions
			// are kept so a slow hue sweep does not step
			double aval = stack->popDouble();
			double vval = stack->popDouble();
			double sval = stack->popDouble();
			double hval = stack->popDouble();
			if (!(hval >= 0 && hval <= 360 && sval >= 0 && sval <= 100
				  && vval >= 0 && vval <= 100 && aval >= 0 && aval <= 100)) {
				error->q(ERROR_HSV);
				stack->pushLong(0);
			} else {
				if (hval >= 360) hval = 0;
				stack->pushInt( (int) QColor::fromHsvF(hval / 360.0, sval / 100.0, vval / 100.0, aval / 100.0).rgba());
			}
		}
		break;

		case OP_PIXEL: {
			double yd = stack->popDouble();
			double xd = stack->popDouble();
			int x, y;
			if (windowActive) {
				// PIXEL is the read-side inverse of PLOT, so it speaks
				// window units and lands on the nearest whole pixel
				QPointF p = windowTransform.map(QPointF(xd, yd));
				x = qRound(p.x());
				y = qRound(p.y());
			} else {
				x = (int) xd;
				y = (int) yd;
			}
			if(drawingOnScreen || drawto.isEmpty()){
				QRgb rgb = graphics->image->pixel(x,y);
				stack->pushInt((int) rgb);
			}else{
				QRgb rgb = images[drawto]->pixel(x,y);
				stack->pushInt((int) rgb);
			}
		}
		break;

		case OP_GETCOLOR: {
			stack->pushInt((int) drawingpen.color().rgba());
		}
		break;

		case OP_GETSLICE: {
			// slice format was a hex stringnow it is a 2d array list on the stack
			int layer = stack->popInt();
			int h = stack->popInt();
			int w = stack->popInt();
			int x, y;
			if (windowActive) {
				// as PIXEL, the corner is a window coordinate; the width and height
				// stay a count of pixels, so the slice keeps the size it was asked
				// for and PUTSLICE can put it back exactly where it came from
				double yd = stack->popDouble();
				double xd = stack->popDouble();
				QPointF c = windowTransform.map(QPointF(xd, yd));
				x = qRound(c.x());
				y = qRound(c.y());
			} else {
				y = stack->popInt();
				x = stack->popInt();
			}
			QImage *layerimage;
			DataElement *d = new DataElement();			// RELEASE
			switch(layer) {
				case SLICE_PAINT:
					if(drawingOnScreen || drawto.isEmpty()){
						layerimage = graphics->image;
					}else{
						layerimage = images[drawto];
					}
					break;
				case SLICE_SPRITE:
					layerimage = graphics->spritesimage;
					break;
				default:
					layerimage = graphics->displayedimage;
					break;
			}
			if (w<=0 || h<=0) {
				error->q(ERROR_SLICESIZE);
			} else {
				d->arrayDim(w, h, false);
				QImage tmp = QImage(layerimage->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32));
				const uchar* p = tmp.constBits();
				QRgb *r = (QRgb *) p;
				int counter = 0;
				int tw, th;
				for(th=0; th<h; th++) {
					for(tw=0; tw<w; tw++) {
						DataElement* temp = new DataElement((int) r[counter]);
						d->arraySetData(tw,th,temp);
						delete temp;
						counter++;
					}
				}
			}
			stack->pushDE(d);
			delete d;
		}
		break;

		case OP_PUTSLICE: {
			// get image from array
			int tw,th;
			DataElement *d = stack->popDE();			// RELEASE
			int x, y;
			if (windowActive) {
				// as PIXEL, the corner is a window coordinate; the width and height
				// stay a count of pixels, so the slice keeps the size it was asked
				// for and PUTSLICE can put it back exactly where it came from
				double yd = stack->popDouble();
				double xd = stack->popDouble();
				QPointF c = windowTransform.map(QPointF(xd, yd));
				x = qRound(c.x());
				y = qRound(c.y());
			} else {
				y = stack->popInt();
				x = stack->popInt();
			}
			
			if (DataElement::getType(d)==T_ARRAY) {
				int w = d->arrayRows();
				int h = d->arrayCols();

				// get image data from array
				QImage tmp = QImage(w, h, QImage::Format_ARGB32);
				const uchar* p = tmp.constBits();
				QRgb *r = (QRgb *) p;
				int counter = 0;
				for (th=0;th<h;th++) {
					for (tw=0; tw<w; tw++) {
					r[counter++] = (QRgb) convert->getInt(d->arrayGetData(tw,th));			// DONT RELEASE
					}
				}
				//update painter only if needed (faster)
				if(CompositionModeClear){
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
					painter_last_compositionModeClear=false;
				}
				// actually display the image -- x,y is already in surface pixels, and the
				// slice is drawn pixel for pixel, so putslice x,y,getslice(x,y,w,h,l)
				// puts the pixels back exactly where they were taken from
				if (windowActive && painter->isActive()) {
					painter->save();
					painter->resetTransform();
					painter->drawImage(x, y, tmp);
					painter->restore();
				} else {
					painter->drawImage(x, y, tmp);
				}
				if (!fastgraphics && drawingOnScreen) autoRefresh();
			} else {
				error->q(ERROR_ARRAYEXPR);
			}
			delete d;
		}
		break;

		case OP_LINE: {
			double y1val = stack->popDouble();
			double x1val = stack->popDouble();
			double y0val = stack->popDouble();
			double x0val = stack->popDouble();

			//update painter's attributes only if needed (only pen)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_last_compositionModeClear!=PenColorIsClear){
				if(PenColorIsClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=PenColorIsClear;
			}
			//end painter update

			painter->drawLine(QLineF(x0val, y0val, x1val, y1val));

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_ROUNDEDRECT:{
			double y_rad = stack->popDouble();
			double x_rad = stack->popDouble();
			double y1val = stack->popDouble();
			double x1val = stack->popDouble();
			double y0val = stack->popDouble();
			double x0val = stack->popDouble();

			// the +1 makes a negative size include its starting pixel --
			// a pixel convention, so it only applies without a window
			double edge = windowActive ? 0.0 : 1.0;
			if(x1val<0) {
				x0val+=x1val+edge;
				x1val*=-1;
			}
			if(y1val<0) {
				y0val+=y1val+edge;
				y1val*=-1;
			}

			//update painter's attributes only if needed (pen and brush)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_brush_need_update){
				painter->setBrush(drawingbrush);
				painter_brush_need_update=false;
			}
			if(painter_last_compositionModeClear!=CompositionModeClear){
				if(CompositionModeClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=CompositionModeClear;
			}
			//end painter update

			if (windowActive) {
				// as OP_RECT: no pixel fudging in window units
				painter->drawRoundedRect(QRectF(x0val, y0val, x1val, y1val), x_rad, y_rad);
			} else if (x1val > 1 && y1val > 1) {
				painter->drawRoundedRect(QRectF(x0val, y0val, x1val-1, y1val-1), x_rad, y_rad);
			} else if (x1val==1 && y1val==1) {
				// rect 1x1 is actually a point
				painter->drawPoint(QPointF(x0val, y0val));
			} else if (x1val==1 && y1val!=0) {
				// rect 1xn is actually a line
				painter->drawLine(QLineF(x0val, y0val, x0val, y0val+y1val));
			} else if (x1val!=0 && y1val==1) {
				// rect nx1 is actually a line
				painter->drawLine(QLineF(x0val, y0val, x0val + x1val, y0val));
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_RECT: {
			double y1val = stack->popDouble();
			double x1val = stack->popDouble();
			double y0val = stack->popDouble();
			double x0val = stack->popDouble();

			// the +1 makes a negative size include its starting pixel --
			// a pixel convention, so it only applies without a window
			double edge = windowActive ? 0.0 : 1.0;
			if(x1val<0) {
				x0val+=x1val+edge;
				x1val*=-1;
			}
			if(y1val<0) {
				y0val+=y1val+edge;
				y1val*=-1;
			}

			//update painter's attributes only if needed (pen and brush)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_brush_need_update){
				painter->setBrush(drawingbrush);
				painter_brush_need_update=false;
			}
			if(painter_last_compositionModeClear!=CompositionModeClear){
				if(CompositionModeClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=CompositionModeClear;
			}
			//end painter update

			if (windowActive) {
				// in window units a size of 1 is not "one pixel wide", so
				// neither the -1 nor the degenerate 1xN cases below apply:
				// the rectangle is simply the rectangle asked for
				painter->drawRect(QRectF(x0val, y0val, x1val, y1val));
			} else if (x1val > 1 && y1val > 1) {
				painter->drawRect(QRectF(x0val, y0val, x1val-1, y1val-1));
			} else if (x1val==1 && y1val==1) {
				// rect 1x1 is actually a point
				painter->drawPoint(QPointF(x0val, y0val));
			} else if (x1val==1 && y1val!=0) {
				// rect 1xn is actually a line
				painter->drawLine(QLineF(x0val, y0val, x0val, y0val+y1val));
			} else if (x1val!=0 && y1val==1) {
				// rect nx1 is actually a line
				painter->drawLine(QLineF(x0val, y0val, x0val + x1val, y0val));
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_POLY: {
			// doing a polygon from an array's data
			// either as an even numbered 1d array or as rows of points
			DataElement *e = stack->popDE();			// RELEASE
			QPolygonF *poly = convert->getPolygonF(e);
			if (poly) {
				//update painter's attributes only if needed (pen and brush)
				if(painter_pen_need_update){
					painter->setPen(drawingpen);
					painter_pen_need_update=false;
				}
				if(painter_brush_need_update){
					painter->setBrush(drawingbrush);
					painter_brush_need_update=false;
				}
				if(painter_last_compositionModeClear!=CompositionModeClear){
					if(CompositionModeClear)
						painter->setCompositionMode(QPainter::CompositionMode_Clear);
					else
						painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
					painter_last_compositionModeClear=CompositionModeClear;
				}
				//end painter update

				painter->drawPolygon(*poly);

				if (!fastgraphics && drawingOnScreen) autoRefresh();

			}
			delete e;
		}
		break;

		case OP_STAMP: {
			// special type of poly where x,y,scale, are given first and
			// the ploy is sized and loacted - so we can move them easy
			double tx, ty, savetx;		// used in scaling and rotating
			QPointF point;
			
			DataElement *e = stack->popDE();			// RELEASE
			QPolygonF *poly = convert->getPolygonF(e);
			double rotate = stack->popDouble();
			double scale = stack->popDouble();
			double y = stack->popDouble();
			double x = stack->popDouble();
			
			if (poly) {
				// scale, rotate, and position the points
				for (int j = 0; j < poly->size(); j++) {
					point = poly->at(j);
					tx = scale * point.x();
					ty = scale * point.y();
					if (rotate!=0) {
						savetx = tx;
						tx = cos(rotate) * tx - sin(rotate) * ty;
						ty = cos(rotate) * ty + sin(rotate) * savetx;
					}
					point.setX(tx + x);
					point.setY(ty + y);
					poly->replace(j, point);
				}

				//update painter's attributes only if needed (pen and brush)
				if(painter_pen_need_update){
					painter->setPen(drawingpen);
					painter_pen_need_update=false;
				}
				if(painter_brush_need_update){
					painter->setBrush(drawingbrush);
					painter_brush_need_update=false;
				}
				if(painter_last_compositionModeClear!=CompositionModeClear){
					if(CompositionModeClear)
						painter->setCompositionMode(QPainter::CompositionMode_Clear);
					else
						painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
					painter_last_compositionModeClear=CompositionModeClear;
				}
				//end painter update

				painter->drawPolygon(*poly);
				if (!fastgraphics && drawingOnScreen) autoRefresh();
			}
			delete e;
		}
		break;

		case OP_CIRCLE: {
			double rval = stack->popDouble();
			double yval = stack->popDouble();
			double xval = stack->popDouble();

			//update painter's attributes only if needed (pen and brush)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_brush_need_update){
				painter->setBrush(drawingbrush);
				painter_brush_need_update=false;
			}
			if(painter_last_compositionModeClear!=CompositionModeClear){
				if(CompositionModeClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=CompositionModeClear;
			}
			//end painter update

			painter->drawEllipse(QRectF(xval - rval, yval - rval, 2 * rval, 2 * rval));

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_ELLIPSE: {
			double hval = stack->popDouble();
			double wval = stack->popDouble();
			double yval = stack->popDouble();
			double xval = stack->popDouble();

			//update painter's attributes only if needed (pen and brush)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_brush_need_update){
				painter->setBrush(drawingbrush);
				painter_brush_need_update=false;
			}
			if(painter_last_compositionModeClear!=CompositionModeClear){
				if(CompositionModeClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=CompositionModeClear;
			}
			//end painter update

			painter->drawEllipse(QRectF(xval, yval, wval, hval));

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_IMGLOAD: {
			// Image Load - with scale and rotate

			// pop the filename to uncover the location and scale
			QString file = stack->popQString();

			double rotate = stack->popDouble();
			double scale = stack->popDouble();
			double y = stack->popDouble();
			double x = stack->popDouble();

			QImage i;
			if(QFileInfo(file).exists()){
				i = QImage(file);
			}else{
				// wasm: no local file exists, so a relative path is fetched from
				// beside the page (MediaPath). Unchanged on the desktop.
				i.loadFromData(media.fetch(file));
			}

			if(i.isNull()) {
				error->q(ERROR_IMAGEFILE);
			} else {


				if (rotate != 0 || scale != 1) {
					QTransform transform = QTransform().translate(0,0).rotateRadians(rotate).scale(scale, scale);
					i = i.transformed(transform);
				}
				if (i.width() != 0 && i.height() != 0) {

					//update painter only if needed (faster)
					if(CompositionModeClear){
						painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
						painter_last_compositionModeClear=false;
					}
					//end update painter

					if (windowActive && painter->isActive()) {
						// as OP_TEXT: an image is a block of pixels and IMGLOAD has its own
						// scale argument, so the window places the centre and the image is
						// then drawn at its own size rather than magnified by the window
						QPointF c = windowTransform.map(QPointF(x, y));
						painter->save();
						painter->resetTransform();
						painter->drawImage(QPointF(c.x() - .5 * i.width(), c.y() - .5 * i.height()), i);
						painter->restore();
					} else {
						painter->drawImage(QPointF(x - .5 * i.width(), y - .5 * i.height()), i);
					}
				}
				if (!fastgraphics && drawingOnScreen) autoRefresh();
			}
		}
		break;

		case OP_TEXT: {
			QString txt = stack->popQString();
			double y0val = stack->popDouble();
			double x0val = stack->popDouble();

			//update painter's attributes only if needed (only pen)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_last_compositionModeClear!=PenColorIsClear){
				if(PenColorIsClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=PenColorIsClear;
			}
			//end painter update

			if(painter_font_need_update){
				painter->setFont(font);
				painter_font_need_update=false;
			}
			if (windowActive && painter->isActive()) {
				// FONT is in surface pixels, so place the anchor through
				// the window and then draw with the transform off --
				// otherwise the glyphs stretch with the window instead of
				// staying the size FONT asked for
				QPointF p = windowTransform.map(QPointF(x0val, y0val));
				painter->save();
				painter->resetTransform();
				painter->drawText(QPointF(p.x(), p.y()+(QFontMetrics(painter->font()).ascent())), txt);
				painter->restore();
			} else {
				painter->drawText(QPointF(x0val, y0val+(QFontMetrics(painter->font()).ascent())), txt);
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_TEXTBOX: {
			int flags = stack->popInt();
			QString txt = stack->popQString();
			double h = stack->popDouble();
			double w = stack->popDouble();
			double y = stack->popDouble();
			double x = stack->popDouble();

			if(h<0){
				y+=h;
				h=-h;
			}
			if(w<0){
				x+=w;
				w=-w;
			}

			//update painter's attributes only if needed (only pen)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_last_compositionModeClear!=PenColorIsClear){
				if(PenColorIsClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=PenColorIsClear;
			}
			//end painter update

			if(painter_font_need_update){
				painter->setFont(font);
				painter_font_need_update=false;
			}
			if (windowActive && painter->isActive()) {
				// as OP_TEXT: the box is placed by the window, the text
				// inside it is laid out at its FONT size in pixels
				QRectF r = windowTransform.mapRect(QRectF(x, y, w, h));
				painter->save();
				painter->resetTransform();
				painter->drawText(r, flags|Qt::TextWordWrap|Qt::TextExpandTabs, txt);
				painter->restore();
			} else {
				painter->drawText(QRectF(x, y, w, h), flags|Qt::TextWordWrap|Qt::TextExpandTabs, txt);
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_TEXTBOXHEIGHT:
		case OP_TEXTBOXWIDTH: {
			double w = stack->popDouble();
			QString txt = stack->popQString();

			if(w<0) w=-w;

			//update painter's attributes only if needed (only pen)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			//end painter update

			if(painter_font_need_update){
				painter->setFont(font);
				painter_font_need_update=false;
			}
			QRectF boundingRect;
			painter->drawText(QRectF(-1, -1, w, 0), Qt::TextWordWrap, txt, &boundingRect);
			if(opcode==OP_TEXTBOXHEIGHT){
				stack->pushInt(boundingRect.height());
			}else{
				stack->pushInt(boundingRect.width());
			}
		}
		break;

		case OP_FONT: {
			bool italic = stack->popBool();
			int weight = stack->popInt();
			int size = stack->popInt();
			QString family = stack->popQString().trimmed();
			if(family.isEmpty())
				font = QFont(defaultfontfamily, size, weight, italic);
			else
				font = QFont(family, size, weight, italic);

			if(defaultfontpointsize == font.pointSize() && defaultfontweight == font.weight() && defaultfontfamily == painter->font().family() && defaultfontitalic == font.italic())
				painter_custom_font_flag = false;
			else
				painter_custom_font_flag = true;

			painter_font_need_update=true;
		}
		break;

		case OP_CLG: {
			int clearcolor = stack->popInt();
			QColor c = QColor::fromRgba((QRgb) clearcolor);

			if (drawingOnScreen){
				// --silent: skip the direct fill too (it bypasses the
				// painter no-op in setPainterTo since it writes straight
				// to the image buffer).
				if (guiState != GUISTATESILENT) {
					graphics->image->fill(c);
					if (!fastgraphics) autoRefresh();
				}
			}
#ifdef BASIC256_ENABLE_PRINTER
			else if(printing){
				if(printdocument->pageLayout().paintRectPixels(printdocument->resolution())==printdocument->pageLayout().fullRectPixels(printdocument->resolution())){
					//printer is in full page mode already
					painter->fillRect(printdocument->pageLayout().fullRectPixels(printdocument->resolution()),c);
				}else{
					//a good solution is to end painter and begin after setFullPage(true)
					//this will reset origins for painter to top-left of the page
					//but swiching back setFullPage(false) and starting again painter (begin) to page
					//clear the page entirely.
					QRect r = printdocument->pageLayout().fullRectPixels(printdocument->resolution());
					printdocument->setFullPage(true);
					painter->translate(-r.left(),-r.top());
					painter->fillRect(printdocument->pageLayout().fullRectPixels(printdocument->resolution()),c);
					printdocument->setFullPage(false);
					painter->translate(r.topLeft());
				}
			}
#endif
			else{
				images[drawto]->fill(c);
			}
		}
		break;

		case OP_PLOT: {
			double oneval = stack->popDouble();
			double twoval = stack->popDouble();

			//update painter's attributes only if needed (only pen)
			if(painter_pen_need_update){
				painter->setPen(drawingpen);
				painter_pen_need_update=false;
			}
			if(painter_last_compositionModeClear!=PenColorIsClear){
				if(PenColorIsClear)
					painter->setCompositionMode(QPainter::CompositionMode_Clear);
				else
					painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
				painter_last_compositionModeClear=PenColorIsClear;
			}
			//end painter update

			painter->drawPoint(QPointF(twoval, oneval));

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_WINDOW: {
			// WINDOW x1,y1,x2,y2 gives the drawing surface a logical
			// coordinate space: (x1,y1) is its top-left corner and
			// (x2,y2) its bottom-right, so the argument order picks
			// which way each axis runs. WINDOW on its own goes back to
			// surface pixels.
			int arg = stack->popInt();
			bool ok = true;
			if (arg == 0) {
				windowActive = false;
			} else {
				double y2 = stack->popDouble();
				double x2 = stack->popDouble();
				double y1 = stack->popDouble();
				double x1 = stack->popDouble();
				if (x1 == x2 || y1 == y2) {
					// a zero-width or zero-height window would divide by
					// zero in updateWindowTransform()
					error->q(ERROR_WINDOWSIZE);
					ok = false;
				} else {
					winX1 = x1; winY1 = y1; winX2 = x2; winY2 = y2;
					windowActive = true;
				}
			}
			if (ok && painter->isActive()) {
				// rebuild the painter on the same surface so the new
				// transform (or its removal) takes effect; this covers
				// the screen canvas, an image resource and the printer
				// without having to know which one we are on
				setPainterTo(painter->device());
			}
		}
		break;

		case OP_FASTGRAPHICS: {
			fastgraphics = true;
			emit(fastGraphics());
		}
		break;

		case OP_GRAPHSIZE: {
			qreal scale = stack->popDouble();
			int height = stack->popInt();
			int width = stack->popInt();

			if (height<=0 || width<=0){
				height = GSIZE_INITIAL_HEIGHT;
				width = GSIZE_INITIAL_WIDTH;
			}
			if(drawingOnScreen || drawto.isEmpty()){
				//change graph size if current graph is on screen (it may be printing also)
				if(drawingOnScreen) painter->end();
				mymutex->lock();
				emit(resizeGraphWindow(width, height, scale));
				waitCond->wait(mymutex);
				mymutex->unlock();
				if(drawingOnScreen) setPainterTo(graphics->image);
				sprites.forceRedrawAll();
			}else{
				QImage tmp = images[drawto]->copy(0,0,width,height);
				if(!printing){
					painter->end();
					images[drawto]->swap(tmp);
					setPainterTo(images[drawto]);
				}else{
					images[drawto]->swap(tmp);
				}
			}
		}
		break;

		case OP_GRAPHWIDTH: {
			int w = 0;
			if (drawingOnScreen){
				w = graphics->image->width();
			}
#ifdef BASIC256_ENABLE_PRINTER
			else if(printing){
				w = printdocument->width();
			}
#endif
			else{
				w = images[drawto]->width();
			}
			stack->pushInt(w);
		}
		break;

		case OP_GRAPHHEIGHT: {
			int h = 0;
			if (drawingOnScreen) {
				h = graphics->image->height();
			}
#ifdef BASIC256_ENABLE_PRINTER
			else if(printing){
				h = printdocument->height();
			}
#endif
			else{
				h = images[drawto]->height();
			}
			stack->pushInt(h);
		}
		break;

		case OP_REFRESH: {
			waitForGraphics();
		}
		break;

		case OP_MOUSEX: {
			// -1 is the "pointer is not on the canvas" marker and is
			// passed through unmapped so existing tests for it still work
			if (windowActive && graphics->mouseX >= 0) {
				stack->pushDouble(windowInverse.map(QPointF(graphics->mouseX, graphics->mouseY)).x());
			} else {
				stack->pushInt((int) graphics->mouseX);
			}
		}
		break;

		case OP_MOUSEY: {
			if (windowActive && graphics->mouseY >= 0) {
				stack->pushDouble(windowInverse.map(QPointF(graphics->mouseX, graphics->mouseY)).y());
			} else {
				stack->pushInt((int) graphics->mouseY);
			}
		}
		break;

		case OP_MOUSEB: {
			stack->pushInt((int) graphics->mouseB);
		}
		break;

		case OP_CLICKCLEAR: {
			graphics->clickX = 0;
			graphics->clickY = 0;
			graphics->clickB = 0;
		}
		break;

		case OP_CLICKX: {
			if (windowActive) {
				stack->pushDouble(windowInverse.map(QPointF(graphics->clickX, graphics->clickY)).x());
			} else {
				stack->pushInt((int) graphics->clickX);
			}
		}
		break;

		case OP_CLICKY: {
			if (windowActive) {
				stack->pushDouble(windowInverse.map(QPointF(graphics->clickX, graphics->clickY)).y());
			} else {
				stack->pushInt((int) graphics->clickY);
			}
		}
		break;

		case OP_CLICKB: {
			stack->pushInt((int) graphics->clickB);
		}
		break;

		case OP_IMGSAVE: {
			// Image Save - Save image
			QString type = stack->popQString();
			QString file = stack->popQString();
			if (!fileSecurity.allowPath(file, tr("save an image to"))) break;
			
			QStringList validtypes;
			validtypes << IMAGETYPE_BMP << IMAGETYPE_JPG << IMAGETYPE_JPEG << IMAGETYPE_PNG ;
			if (validtypes.contains(type, Qt::CaseInsensitive)) {
				if(drawingOnScreen || drawto.isEmpty()){
					graphics->image->save(file, type.toUpper().toUtf8().data());
				}else{
					images[drawto]->save(file, type.toUpper().toUtf8().data());
				}
			} else {
				error->q(ERROR_IMAGESAVETYPE);
			}
		}
		break;

		case OP_TEXTHEIGHT: {
			// returns the height of the font.
			if(painter_font_need_update){
				painter->setFont(font);
				painter_font_need_update=false;
			}
			stack->pushInt((int) (QFontMetrics(painter->font()).height()));
		}
		break;

		case OP_TEXTWIDTH: {
			// return the number of pixels the font requires for diaplay
			// a string is required for width but not for height
			QString txt = stack->popQString();
			int width=0;
			if(painter_font_need_update){
				painter->setFont(font);
				painter_font_need_update=false;
			}
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			width = (int) (QFontMetrics(painter->font()).horizontalAdvance(txt));
#else
			width = (int) (QFontMetrics(painter->font()).width(txt));
#endif
			stack->pushInt(width);
		}
		break;

		case OP_ARC:
		case OP_CHORD:
		case OP_PIE: {
			double yval, xval, hval, wval;
			int arg = stack->popInt(); // number of arguments
			double angwval = stack->popDouble();
			double startval = stack->popDouble();

			if(arg==5){
				double rval = stack->popDouble();
				yval = stack->popDouble() - rval;
				xval = stack->popDouble() - rval;
				hval = rval * 2;
				wval = rval * 2;
			}else{
				hval = stack->popDouble();
				wval = stack->popDouble();
				yval = stack->popDouble();
				xval = stack->popDouble();
			}

			// degrees * 16
			int s = (int) (startval * 360 * 16 / 2 / M_PI);
			int aw = (int) (angwval * 360 * 16 / 2 / M_PI);
			// transform to clockwise from 12'oclock
			s = 1440-s-aw;


			if(opcode==OP_ARC) {
				//update painter's attributes only if needed (only pen)
				if(painter_pen_need_update){
					painter->setPen(drawingpen);
					painter_pen_need_update=false;
				}
				if(painter_last_compositionModeClear!=PenColorIsClear){
					if(PenColorIsClear)
						painter->setCompositionMode(QPainter::CompositionMode_Clear);
					else
						painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
					painter_last_compositionModeClear=PenColorIsClear;
				}
				//end painter update
			}else{
				//update painter's attributes only if needed (pen and brush)
				if(painter_pen_need_update){
					painter->setPen(drawingpen);
					painter_pen_need_update=false;
				}
				if(painter_brush_need_update){
					painter->setBrush(drawingbrush);
					painter_brush_need_update=false;
				}
						if(painter_last_compositionModeClear!=CompositionModeClear){
							if(CompositionModeClear)
								painter->setCompositionMode(QPainter::CompositionMode_Clear);
							else
								painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
							painter_last_compositionModeClear=CompositionModeClear;
						}
				//end painter update
			}

			if(opcode==OP_ARC) {
				painter->drawArc(QRectF(xval, yval, wval, hval), s, aw);
			}
			if(opcode==OP_CHORD) {
				painter->drawChord(QRectF(xval, yval, wval, hval), s, aw);
			}
			if(opcode==OP_PIE) {
				painter->drawPie(QRectF(xval, yval, wval, hval), s, aw);
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_PENWIDTH: {
			double a = stack->popInt();
			if (a<0) {
				error->q(ERROR_PENWIDTH);
			} else {
				drawingpen.setWidth(a);
				if (a==0) {
					drawingpen.setStyle(Qt::NoPen);
				} else {
					drawingpen.setStyle(Qt::SolidLine);
				}
				painter_pen_need_update=true;
			}
		}
		break;

		case OP_GETPENWIDTH: {
			stack->pushInt((double) (drawingpen.width()));
		}
		break;

		case OP_GETBRUSHCOLOR: {
			stack->pushInt((int) drawingbrush.color().rgba());
		}
		break;

#ifdef BASIC256_ENABLE_PRINTER
		case OP_PRINTEROFF: {
			if (printing) {
				printing = false;
				setGraph(drawto);
				delete printdocument;
			} else {
				error->q(ERROR_PRINTERNOTON);
			}
		}
		break;

		case OP_PRINTERON: {
			if (printing) {
				error->q(ERROR_PRINTERNOTOFF);
			} else {
				int printer = settingsPrinterPrinter;
				if (printer==-1) {
					// pdf printer
					printdocument = new QPrinter((QPrinter::PrinterMode) settingsPrinterResolution);
					printdocument->setOutputFormat(QPrinter::PdfFormat);
					printdocument->setOutputFileName(settingsPrinterPdfFile);

				} else {
					// system printer
					QList<QPrinterInfo> printerList=QPrinterInfo::availablePrinters();
					if (printer>=printerList.count()) printer = 0;
					printdocument = new QPrinter(printerList[printer], (QPrinter::PrinterMode) settingsPrinterResolution);
				}
				if (printdocument) {
					if(printdocument->isValid()){
						printdocument->setCreator(QString(SETTINGSAPP));
						printdocument->setDocName(programTitle);
						printdocument->setPageSize(QPageSize(static_cast<QPageSize::PageSizeId>(settingsPrinterPaper)));
						printdocument->setPageOrientation(static_cast<QPageLayout::Orientation>(settingsPrinterOrient));
						if (!setPainterTo(printdocument)) {
							error->q(ERROR_PRINTEROPEN);
							setGraph(drawto); //if drawing on printer fails, then fall back to graph area
						} else {
							printing = true;
						}
					}else{
						delete printdocument;
						error->q(ERROR_PRINTEROPEN);
					}
				} else {
					error->q(ERROR_PRINTEROPEN);
				}
			}
		}
		break;

		case OP_PRINTERPAGE: {
			if (printing) {
				printdocument->newPage();
			} else {
				error->q(ERROR_PRINTERNOTON);
			}
		}
		break;

		case OP_PRINTERCANCEL: {
			if (printing) {
				printing = false;
				setGraph(drawto);
				printdocument->abort();
				delete printdocument;
			} else {
				error->q(ERROR_PRINTERNOTON);
			}
		}
		break;
#else
		case OP_PRINTEROFF:
		case OP_PRINTERON:
		case OP_PRINTERPAGE:
		case OP_PRINTERCANCEL: {
			error->q(ERROR_NOTAVAILABLE);
		}
		break;
#endif

		case OP_IMAGELOAD: {
			QString s = stack->popQString();
			lastImageId++;
			QString id = QString("image:") + QString::number(lastImageId) + QStringLiteral(":") + s;
			if(QFileInfo(s).exists()){
				images[id] = new QImage(QImage(s).convertToFormat(QImage::Format_ARGB32));
			}else{
				// wasm: relative paths are fetched from beside the page.
				QImage *temp = new QImage();
				temp->loadFromData(media.fetch(s));
				images[id] = new QImage(temp->convertToFormat(QImage::Format_ARGB32));
				delete temp;
			}
			stack->pushQString(id);
			if(images[id]->isNull())
				error->q(ERROR_IMAGEFILE);
		}
		break;

		case OP_IMAGENEW: {
			int c = stack->popInt();
			int h = stack->popInt();
			int w = stack->popInt();
			lastImageId++;
			QString id = QString("image:") + QString::number(lastImageId);
			images[id] = new QImage(w, h, QImage::Format_ARGB32);
			images[id]->fill(QColor::fromRgba((QRgb) c));
			stack->pushQString(id);
		}
		break;

		case OP_IMAGECOPY: {
			int nr = stack->popInt();
			lastImageId++;
			QString id = QString("image:") + QString::number(lastImageId);
			switch (nr){
			case 0:{
				if(drawingOnScreen || drawto.isEmpty()){
					images[id] = new QImage(*graphics->image);
				}else{
					images[id] = new QImage(*images[drawto]);
				}
				break;
			}
			case 4:{
				int h = stack->popInt();
				int w = stack->popInt();
				int y = stack->popInt();
				int x = stack->popInt();
				if(drawingOnScreen || drawto.isEmpty()){
					images[id] = new QImage(graphics->image->copy(x, y, w, h));
				}else{
					images[id] = new QImage(images[drawto]->copy(x, y, w, h));
				}
				break;
			}
			case 1:{
				QString id2 = stack->popQString();
				if (images.contains(id2)){
					images[id] = new QImage(*images[id2]);
				}else{
					error->q(ERROR_IMAGERESOURCE);
				}
				break;
			}
			case 5:{
				int h = stack->popInt();
				int w = stack->popInt();
				int y = stack->popInt();
				int x = stack->popInt();
				QString id2 = stack->popQString();
				if (images.contains(id2)){
					images[id] = new QImage(images[id2]->copy(x, y, w, h));
				}else{
					error->q(ERROR_IMAGERESOURCE);
				}
				break;
			}
			}
			stack->pushQString(id);
		}
		break;

		case OP_IMAGECROP: {
			int h = stack->popInt();
			int w = stack->popInt();
			int y = stack->popInt();
			int x = stack->popInt();
			QString id = stack->popQString();
			if (images.contains(id)){
				QImage tmp = QImage(images[id]->copy(x, y, w, h));
				if(id==drawto){
					painter->end();
					images[id]->swap(tmp);
					setPainterTo(images[id]);
				}else{
					images[id]->swap(tmp);
				}
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGEAUTOCROP: {
			int x,y,w,h,y1,y2,x1=0,x2=0;
			int c=0;
			int nr = stack->popInt();
			if(nr==2) c = stack->popInt();
			QString id = stack->popQString();
			if (images.contains(id)){
				w=images[id]->width();
				h=images[id]->height();
				if(w>0 && h>0){
					if(nr==2){
						for(y=0;y<h;y++){
							for(x=0;x<w;x++){
								if(images[id]->pixel(x,y)!=c) break;
							}
						if(x<w) break;
						}
						y1=y;
						for(y=h-1;y>y1;y--){
							for(x=0;x<w;x++){
								if(images[id]->pixel(x,y)!=c) break;
							}
						if(x<w) break;
						}
						y2=y;
						if(y1!=y2){
							for(x=0;x<w;x++){
								for(y=y1;y<y2;y++){
									if(images[id]->pixel(x,y)!=c) break;
								}
							if(y<y2) break;
							}
							x1=x;
							for(x=w-1;x>x1;x--){
								for(y=y1;y<y2;y++){
									if(images[id]->pixel(x,y)!=c) break;
								}
							if(y<y2) break;
							}
							x2=x;
						}
					}else{
						for(y=0;y<h;y++){
							for(x=0;x<w;x++){
								if(qAlpha(images[id]->pixel(x,y))!=0) break;
							}
						if(x<w) break;
						}
						y1=y;
						for(y=h-1;y>y1;y--){
							for(x=0;x<w;x++){
								if(qAlpha(images[id]->pixel(x,y))!=0) break;
							}
						if(x<w) break;
						}
						y2=y;
						if(y1!=y2){
							for(x=0;x<w;x++){
								for(y=y1;y<y2;y++){
									if(qAlpha(images[id]->pixel(x,y))!=0) break;
								}
							if(y<y2) break;
							}
							x1=x;
							for(x=w-1;x>x1;x--){
								for(y=y1;y<y2;y++){
									if(qAlpha(images[id]->pixel(x,y))!=0) break;
								}
							if(y<y2) break;
							}
							x2=x;
						}
					}
					QImage tmp;
					if(y2>y1){
						 tmp = QImage(images[id]->copy(x1, y1, x2-x1+1, y2-y1+1));
					}else{
						tmp = QImage();
					}

					if(id==drawto){
						painter->end();
						images[id]->swap(tmp);
						setPainterTo(images[id]);
					}else{
						images[id]->swap(tmp);
					}
				}
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGERESIZE: {
			int h = 0, w = 0, nr;
			double s = 0.0;
			nr = stack->popInt();
			if(nr==3){
				h = stack->popInt();
				w = stack->popInt();
			}else{
				s = stack->popDouble();
			}
			QString id = stack->popQString();
			if (images.contains(id)){
				QImage tmp;
				if(nr==3){
					tmp = QImage(images[id]->scaled(w,h,Qt::IgnoreAspectRatio,imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
				}else{
					QTransform transform = QTransform().scale(s,s);
					tmp = QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
				}
				if(id==drawto){
					painter->end();
					images[id]->swap(tmp);
					setPainterTo(images[id]);
				}else{
					images[id]->swap(tmp);
				}
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGESETPIXEL: {
			int c = stack->popInt();
			int y = stack->popInt();
			int x = stack->popInt();
			QString id = stack->popQString();
			if (images.contains(id)){
				images[id]->setPixel(x,y,c);
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGEWIDTH: {
			QString id = stack->popQString();
			if (images.contains(id)){
				stack->pushInt(images[id]->width());
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGEHEIGHT: {
			QString id = stack->popQString();
			if (images.contains(id)){
				stack->pushInt(images[id]->height());
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGEPIXEL: {
			int y = stack->popInt();
			int x = stack->popInt();
			QString id = stack->popQString();
			if (images.contains(id)){
				int c = images[id]->pixel(x,y);
				stack->pushInt(c);
			}else{
				error->q(ERROR_IMAGERESOURCE);
				stack->pushInt(0);
			}
		}
		break;

		case OP_IMAGEDRAW: {
			int nr = stack->popInt();
			double o = 1;
			int x, y, h, w;
			QString id;



			switch (nr){
			case 4:
				o = stack->popDouble();
			case 3:
				y = stack->popInt();
				x = stack->popInt();
				id = stack->popQString();
				if (images.contains(id)){
					if(o!=1.0) painter->setOpacity(o);
					painter->drawImage(x, y, *images[id]);
					if(o!=1.0) painter->setOpacity(1.0);
				}else{
					error->q(ERROR_IMAGERESOURCE);
				}
				break;
			case 6:
				o = stack->popDouble();
			case 5:
				h = stack->popInt();
				w = stack->popInt();
				y = stack->popInt();
				x = stack->popInt();
				id = stack->popQString();
				if (images.contains(id)){
					bool r=false;
					r = painter->testRenderHint(QPainter::SmoothPixmapTransform);
					painter->setRenderHint(QPainter::SmoothPixmapTransform,imageSmooth);
					if(o!=1.0) painter->setOpacity(o);
					if(w<0 || h<0){
						painter->drawImage(QRectF(x,y,w<0?w*-1:w,h<0?h*-1:h), images[id]->mirrored(w<0, h<0), QRectF(0,0,images[id]->width(),images[id]->height()));
					}else{
						painter->drawImage(QRectF(x,y,w,h), *images[id], QRectF(0,0,images[id]->width(),images[id]->height()));
					}
					painter->setRenderHints(QPainter::SmoothPixmapTransform, r);
					if (o!=1.0) painter->setOpacity(1.0);
				}else{
					error->q(ERROR_IMAGERESOURCE);
				}
				break;
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_IMAGEFLIP: {
			bool h = stack->popBool();
			bool w = stack->popBool();
			QString id = stack->popQString();
			if (images.contains(id)){
				QImage tmp = QImage(images[id]->mirrored(w, h));
				if(id==drawto){
					painter->end();
					images[id]->swap(tmp);
					setPainterTo(images[id]);
				}else{
					images[id]->swap(tmp);
				}
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGEROTATE: {
			double d = stack->popDouble();
			QString id = stack->popQString();
			if (images.contains(id)){
				QTransform rot;
				rot.rotateRadians(d);
				QImage tmp = QImage(images[id]->transformed(rot, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
				if(id==drawto){
					painter->end();
					images[id]->swap(tmp);
					setPainterTo(images[id]);
				}else{
					images[id]->swap(tmp);
				}
			}else{
				error->q(ERROR_IMAGERESOURCE);
			}
		}
		break;

		case OP_IMAGESMOOTH: {
			bool v = stack->popBool();
			imageSmooth = v;
		}
		break;

		case OP_IMAGECENTERED: {
			double o=1, r=0, s=1, y=0, x=0;
			int nr = stack->popInt(); // number of arguments (3-6)
			switch(nr){
				case 6  :
				   o = stack->popDouble();
				case 5  :
				   r = stack->popDouble();
				case 4  :
				   s = stack->popDouble();
				case 3 :
				   y = stack->popDouble();
				   x = stack->popDouble();
			}
			QString id = stack->popQString();
			if (images.contains(id)){
				QTransform transform = QTransform().translate(images[id]->width()/2, images[id]->height()/2).rotateRadians(r).scale(s,s);
				QImage *tmp = new QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
				if(o!=1.0) painter->setOpacity(o);
				painter->drawImage(x-(tmp->width()/2),y-(tmp->height()/2), *tmp);
				delete tmp;
				if (o!=1.0) painter->setOpacity(1.0);

			}else{
				error->q(ERROR_IMAGERESOURCE);
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_UNLOAD: {
			QString id = stack->popQString();
			if(id.startsWith("image:")){
				if (images.contains(id)){
					if(drawto==id) setGraph("");
					delete(images[id]);
					images.remove(id);
				}else{
					error->q(ERROR_IMAGERESOURCE);
				}
			}else if(id.startsWith("sound:") || id.startsWith("beep:")){
#ifdef Q_OS_WASM
				media.forgetSound(id);
#endif
				if(!sound->unloadSound(id)){
					error->q(ERROR_SOUNDRESOURCE);
				}
			}else{
				error->q(ERROR_INVALIDRESOURCE);
			}
		}
		break;

		case OP_IMAGETRANSFORMED: {
			double o = stack->popDouble();
			int y4 = stack->popInt();
			int x4 = stack->popInt();
			int y3 = stack->popInt();
			int x3 = stack->popInt();
			int y2 = stack->popInt();
			int x2 = stack->popInt();
			int y1 = stack->popInt();
			int x1 = stack->popInt();
			QString id = stack->popQString();

			if (images.contains(id)){
				double w = images[id]->width();
				double h = images[id]->height();
				QPolygonF polygon1, polygon2;
				polygon2 << QPointF(x1, y1) << QPointF(x2, y2) << QPointF(x3, y3) << QPointF(x4, y4);
				polygon1 << QPointF(0.0, 0.0) << QPointF(w-1.0, 0.0) << QPointF(w-1.0, h-1.0) << QPointF(0.0, h-1.0);
				QTransform transform;
				QTransform::quadToQuad(polygon1,polygon2,transform);
				QImage *tmp = new QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));

				if(x1>x2) x1=x2;
				if(x1>x3) x1=x3;
				if(x1>x4) x1=x4;
				if(y1>y2) y1=y2;
				if(y1>y3) y1=y3;
				if(y1>y4) y1=y4;

				if(o!=1.0) painter->setOpacity(o);
				painter->drawImage((int) x1, (int) y1, *tmp);
				delete tmp;
				if (o!=1.0) painter->setOpacity(1.0);

			}else{
				error->q(ERROR_IMAGERESOURCE);
			}

			if (!fastgraphics && drawingOnScreen) autoRefresh();
		}
		break;

		case OP_SETGRAPH: {
			QString id = stack->popQString();
			setGraph(id);
		}
		break;

	}
}
