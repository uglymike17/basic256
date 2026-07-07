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


#ifndef __GRAPHICSBUFFER_H
#define __GRAPHICSBUFFER_H

#include <QImage>
#include <QRegion>

// The drawing state the interpreter touches, extracted out of the
// BasicGraph widget so the interpreter (core) never has to reach into a
// QWidget. Owns the pixel buffers, sprite compositing state, and the
// mouse/click state; does no painting-to-screen of its own -- BasicGraph
// is the view that paints displayedimage and feeds mouse/click events in.
class GraphicsBuffer
{
 public:
  GraphicsBuffer();
  ~GraphicsBuffer();

  QImage *image;
  QImage *displayedimage;
  QImage *spritesimage;
  QRegion sprites_clip_region;
  bool draw_sprites_flag;

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

  // (Re)creates image/displayedimage/spritesimage at the given size and
  // resets the mouse/click state -- the buffer-management half of what
  // used to be BasicGraph::resize().
  void resizeBuffers(int width, int height);

  // Composites spritesimage onto displayedimage (or just converts image)
  // -- moved verbatim out of BasicGraph::updateScreenImage().
  void updateScreenImage();
};

#endif
