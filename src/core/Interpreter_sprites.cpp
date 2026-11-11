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

// Sprite opcodes.
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execSpriteOp().  The cases pop their arguments, check
// the sprite number, and make a sprite's image from the interpreter's pen,
// brush, font and images; the sprites themselves and the layer they are
// drawn on are SpriteLayer - see SpriteLayer.h.  A break still ends the
// opcode, and the checks execByteCode() makes after every opcode still run
// when this returns.

#include "InterpreterPrivate.h"

void Interpreter::execSpriteOp(int opcode) {
	switch(opcode) {

		case OP_SPRITEPOLY: {
			// create a sprite from a polygon
			
			DataElement *e = stack->popDE();			// RELEASE
			QPolygonF *poly = convert->getPolygonF(e);
			if (poly) {
				// Move the polygon to the top left corner of the sprite and
				// leave a margin for the pen.  drawPolygon centres the stroke
				// on the path, so half of it falls outside the polygon's own
				// bounds - without the margin a wide pen is clipped on every
				// edge.  The caller cannot make room instead, because any
				// margin it adds is taken back out by the move to the corner.
				QRectF bound = poly->boundingRect();
				qreal margin = drawingpen.width() / 2.0;
				qreal dx = margin - bound.left();
				qreal dy = margin - bound.top();
				if (dx != 0 || dy != 0) {
					for(int j=0;j<poly->size();j++) {
						QPointF pt = poly->at(j);
						pt.setX(pt.x()+dx);
						pt.setY(pt.y()+dy);
						poly->replace(j, pt);
					}
					bound = poly->boundingRect();
				}
				// the image is the polygon plus the margin on both sides
				int spritewidth = (int) ceil(bound.width() + drawingpen.width());
				int spriteheight = (int) ceil(bound.height() + drawingpen.width());
				//
				// now build sprite
				int n = stack->popInt(); // sprite number
				if(sprites.exists(n)) {
					// free old, draw, and capture sprite
					sprites.prepareForNewContent(n);
					sprites[n].image = new QImage(spritewidth,spriteheight,QImage::Format_ARGB32_Premultiplied);
					if(!sprites[n].image->isNull()){
						sprites[n].image->fill(Qt::transparent);
						if (!CompositionModeClear) {
							QPainter *p = new QPainter(sprites[n].image);
							p->setPen(drawingpen);
							p->setBrush(drawingbrush);
							p->drawPolygon(*poly);
							p->end();
							delete p;
							sprites[n].position.setRect(-(spritewidth/2),-(spriteheight/2),spritewidth,spriteheight);
						}
					}
				} else {
					error->q(ERROR_SPRITENUMBER);
				}
			}
			delete e;
		}
		break;
		
		case OP_SPRITETEXT: {
			int background = stack->popInt();
			QString txt = stack->popQString();
			int n = stack->popInt(); // sprite number
			if(sprites.exists(n)) {
				// calculate size
				int h, w;
				if(painter_font_need_update){
					painter->setFont(font);
					painter_font_need_update=false;
				}
				h = QFontMetrics(painter->font()).height();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				w = (int) (QFontMetrics(painter->font()).horizontalAdvance(txt));
#else
				w = (int) (QFontMetrics(painter->font()).width(txt));
#endif
				//build sprite
				sprites.prepareForNewContent(n);
				sprites[n].image = new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
				if (background) 
					sprites[n].image->fill(background);
				else
					sprites[n].image->fill(Qt::transparent);
				if(!sprites[n].image->isNull()){
					QPainter *p = new QPainter(sprites[n].image);
					p->setFont(font);
					p->setPen(drawingpen);
					p->drawText(0, QFontMetrics(p->font()).ascent(), txt);
					p->end();
					delete p;
					sprites[n].position.setRect(-(w/2),-(h/2),w,h);
				}
			} else {
				error->q(ERROR_SPRITENUMBER);
			}
		}
		break;

		case OP_SPRITEDIM: {
			int n = stack->popInt();
			sprites.dim(n);
		}
		break;

		case OP_SPRITELOAD: {

			QString file = stack->popQString();
			int n = stack->popInt();

			if(!sprites.exists(n)) {
				error->q(ERROR_SPRITENUMBER);
			} else {
				sprites.prepareForNewContent(n);


				QImage *tmp;
				if(QFileInfo(file).exists()){
					tmp = new QImage(file);
				}else{
					// wasm: relative paths are fetched from beside the page.
					tmp = new QImage();
					tmp->loadFromData(media.fetch(file));
				}


				if(tmp->isNull()) {
					delete tmp;
					error->q(ERROR_IMAGEFILE);
				}else{
					sprites[n].image = new QImage(tmp->convertToFormat(QImage::Format_ARGB32_Premultiplied));
					delete tmp;
					double img_w=sprites[n].image->width();
					double img_h=sprites[n].image->height();
					sprites[n].position.setRect(-(img_w/2),-(img_h/2),img_w,img_h);
				}
			}
		}
		break;

		case OP_SPRITESLICE: {

			int h = stack->popInt();
			int w = stack->popInt();
			int y = stack->popInt();
			int x = stack->popInt();
			int n = stack->popInt();

			if(!sprites.exists(n)) {
				error->q(ERROR_SPRITENUMBER);
			} else {
				sprites.prepareForNewContent(n);
				if(drawingOnScreen || drawto.isEmpty()){
					sprites[n].image = new QImage(graphics->image->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32_Premultiplied));
				}else{
					sprites[n].image = new QImage(images[drawto]->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32_Premultiplied));
				}
				if(sprites[n].image->isNull()) {
					error->q(ERROR_SPRITESLICE);
				}else{
					double img_w=sprites[n].image->width();
					double img_h=sprites[n].image->height();
					sprites[n].position.setRect(-(img_w/2),-(img_h/2),img_w,img_h);
				}
			}
		}
		break;

		case OP_SPRITEMOVE:
		case OP_SPRITEPLACE: {
			double o=0, r=0, s=0, y=0, x=0;
			int nr = stack->popInt(); // number of arguments (3-6)
			switch(nr){
				case 6  :
					o = stack->popDouble();
					[[fallthrough]];
				case 5  :
					r = stack->popDouble();
					[[fallthrough]];
				case 4  :
					s = stack->popDouble();
					[[fallthrough]];
				default :
					y = stack->popDouble();
					x = stack->popDouble();
			}
			int n = stack->popInt();

			if(!sprites.exists(n)) {
				error->q(ERROR_SPRITENUMBER);
			} else {
				if(!sprites[n].image) {
					error->q(ERROR_SPRITENA);
				} else {
					if (opcode==OP_SPRITEMOVE) {
						x += sprites[n].x;
						y += sprites[n].y;
						s += sprites[n].s;
						r += sprites[n].r;
						o += sprites[n].o;
					}else{
						//OP_SPRITEPLACE - populate missing arguments
						if(nr<6) o = sprites[n].o;
						if(nr<5) r = sprites[n].r;
						if(nr<4) s = sprites[n].s;
					}
					sprites.place(n, x, y, s, r, o);
					if (!fastgraphics) autoRefresh();
				}
			}
		}
		break;

		case OP_SPRITEHIDE:
		case OP_SPRITESHOW: {

			int n = stack->popInt();
			bool vis = opcode==OP_SPRITESHOW;

			if(!sprites.exists(n)) {
				error->q(ERROR_SPRITENUMBER);
			} else {
				if(!sprites[n].image && vis) {
					error->q(ERROR_SPRITENA);
				} else if (sprites[n].visible != vis){
					sprites[n].visible = vis;
					if (!fastgraphics) autoRefresh();
				}
			}
		}
		break;

		case OP_SPRITECOLLIDE: {
			int val = stack->popBool();
			int n1 = stack->popInt();
			int n2 = stack->popInt();

			if(!sprites.exists(n1) || !sprites.exists(n2)) {
				error->q(ERROR_SPRITENUMBER);
			} else {
				if(!sprites[n1].image || !sprites[n2].image) {
					error->q(ERROR_SPRITENA);
				} else {
					stack->pushInt(sprites.collide(n1, n2, val!=0));
				}
			}
		}
		break;

		case OP_SPRITEX:
		case OP_SPRITEY:
		case OP_SPRITEH:
		case OP_SPRITEW:
		case OP_SPRITEV:
		case OP_SPRITER:
		case OP_SPRITES:
		case OP_SPRITEO: {

			int n = stack->popInt();

			if(!sprites.exists(n)) {
				error->q(ERROR_SPRITENUMBER);
				stack->pushInt(0);
			} else {
				// SPRITEW/SPRITEH report the size the sprite covers on screen, so they
				// follow the scale and rotation given to SPRITEPLACE/SPRITEMOVE -- the
				// same transformed image that SPRITECOLLIDE and the redraw region use
				QImage *shown = sprites[n].transformed_image ? sprites[n].transformed_image : sprites[n].image;
				if (opcode==OP_SPRITEX) stack->pushDouble(sprites[n].x);
				if (opcode==OP_SPRITEY) stack->pushDouble(sprites[n].y);
				if (opcode==OP_SPRITEH) stack->pushInt(shown?shown->height():0);
				if (opcode==OP_SPRITEW) stack->pushInt(shown?shown->width():0);
				if (opcode==OP_SPRITEV) stack->pushInt(sprites[n].visible?1:0);
				if (opcode==OP_SPRITER) stack->pushDouble(sprites[n].r);
				if (opcode==OP_SPRITES) stack->pushDouble(sprites[n].s);
				if (opcode==OP_SPRITEO) stack->pushDouble(sprites[n].o);
			}
		}
		break;
	}
}
