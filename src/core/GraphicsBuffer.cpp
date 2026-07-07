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


#include "GraphicsBuffer.h"

#include <QPainter>

GraphicsBuffer::GraphicsBuffer() {
    image = NULL;
    displayedimage = NULL;
    spritesimage = NULL;
    sprites_clip_region = QRegion(0,0,0,0);
    draw_sprites_flag = false;
    mouseX = 0;
    mouseY = 0;
    mouseB = 0;
    clickX = 0;
    clickY = 0;
    clickB = 0;
}

GraphicsBuffer::~GraphicsBuffer() {
    if (image) {
        delete image;
        image = NULL;
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

void GraphicsBuffer::resizeBuffers(int width, int height) {
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

    mouseX = 0;
    mouseY = 0;
    mouseB = 0;
    clickX = 0;
    clickY = 0;
    clickB = 0;
}

void GraphicsBuffer::updateScreenImage(){
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
