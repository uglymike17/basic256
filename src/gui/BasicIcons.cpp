/** Copyright (C) 2016, Florin Oprea <florinoprea.contact@gmail.com>
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

#include "BasicIcons.h"
#include <QApplication>
#include <QStyle>

BasicIcons::BasicIcons()
{
    QStyle *style = qApp->style();

    //create icons
    basic256Icon.addFile(":icons/basic256.png",QSize(64, 64));

    saveIcon.addFile(":icons/16x16/save.png",QSize(16, 16));
    saveIcon.addFile(":icons/22x22/save.png",QSize(22, 22));

    saveAsIcon.addFile(":icons/16x16/save-as.png",QSize(16, 16));
    saveAsIcon.addFile(":icons/22x22/save-as.png",QSize(22, 22));

    saveAllIcon.addFile(":icons/16x16/save-all.png",QSize(16, 16));
    saveAllIcon.addFile(":icons/22x22/save-all.png",QSize(22, 22));

    newIcon.addFile(":icons/16x16/new.png",QSize(16, 16));
    newIcon.addFile(":icons/22x22/new.png",QSize(22, 22));

    printIcon.addFile(":icons/16x16/print.png",QSize(16, 16));
    printIcon.addFile(":icons/22x22/print.png",QSize(22, 22));

    openIcon.addFile(":icons/16x16/open.png",QSize(16, 16));
    openIcon.addFile(":icons/22x22/open.png",QSize(22, 22));

    cutIcon.addFile(":icons/16x16/cut.png",QSize(16, 16));
    cutIcon.addFile(":icons/22x22/cut.png",QSize(22, 22));

    copyIcon.addFile(":icons/16x16/copy.png",QSize(16, 16));
    copyIcon.addFile(":icons/22x22/copy.png",QSize(22, 22));

    pasteIcon.addFile(":icons/16x16/paste.png",QSize(16, 16));
    pasteIcon.addFile(":icons/22x22/paste.png",QSize(22, 22));

    undoIcon.addFile(":icons/16x16/undo.png",QSize(16, 16));
    undoIcon.addFile(":icons/22x22/undo.png",QSize(22, 22));

    redoIcon.addFile(":icons/16x16/redo.png",QSize(16, 16));
    redoIcon.addFile(":icons/22x22/redo.png",QSize(22, 22));

    exitIcon.addFile(":icons/16x16/exit.png",QSize(16, 16));
    exitIcon.addFile(":icons/24x24/exit.png",QSize(24, 24));

    preferencesIcon.addFile(":icons/16x16/preferences.png",QSize(16, 16));
    preferencesIcon.addFile(":icons/22x22/preferences.png",QSize(22, 22));

    runIcon.addFile(":icons/16x16/run.png",QSize(16, 16));
    runIcon.addFile(":icons/24x24/run.png",QSize(24, 24));

    debugIcon.addFile(":icons/16x16/debug.png",QSize(16, 16));
    debugIcon.addFile(":icons/24x24/debug.png",QSize(24, 24));

    stepIcon.addFile(":icons/16x16/step.png",QSize(16, 16));
    stepIcon.addFile(":icons/24x24/step.png",QSize(24, 24));

    breakIcon.addFile(":icons/16x16/break.png",QSize(16, 16));
    breakIcon.addFile(":icons/24x24/break.png",QSize(24, 24));

    stopIcon.addFile(":icons/16x16/stop.png",QSize(16, 16));
    stopIcon.addFile(":icons/24x24/stop.png",QSize(24, 24));

    fontIcon.addFile(":icons/16x16/font.png",QSize(16, 16));
    fontIcon.addFile(":icons/22x22/font.png",QSize(22, 22));

    helpIcon.addFile(":icons/16x16/help.png",QSize(16, 16));
    helpIcon.addFile(":icons/24x24/help.png",QSize(24, 24));

    webIcon.addFile(":icons/16x16/web.png",QSize(16, 16));
    webIcon.addFile(":icons/24x24/web.png",QSize(24, 24));

    documentIcon.addFile(":icons/16x16/document.png",QSize(16, 16));
    documentIcon.addFile(":icons/22x22/document.png",QSize(22, 22));

    clearIcon.addFile(":icons/16x16/clear.png",QSize(16, 16));
    clearIcon.addFile(":icons/24x24/clear.png",QSize(24, 24));

    gridIcon.addFile(":icons/16x16/grid.png",QSize(16, 16));
    gridIcon.addFile(":icons/22x22/grid.png",QSize(22, 22));

    infoIcon.addFile(":icons/16x16/info.png",QSize(16, 16));
    infoIcon.addFile(":icons/24x24/info.png",QSize(24, 24));

    goNextIcon.addFile(":icons/16x16/go-next.png",QSize(16, 16));
    goNextIcon.addFile(":icons/22x22/go-next.png",QSize(22, 22));

    goPreviousIcon.addFile(":icons/16x16/go-previous.png",QSize(16, 16));
    goPreviousIcon.addFile(":icons/22x22/go-previous.png",QSize(22, 22));

    goUpIcon.addFile(":icons/16x16/go-up.png",QSize(16, 16));
    goUpIcon.addFile(":icons/22x22/go-up.png",QSize(22, 22));

    goDownIcon.addFile(":icons/16x16/go-down.png",QSize(16, 16));
    goDownIcon.addFile(":icons/22x22/go-down.png",QSize(22, 22));

    goHomeIcon.addFile(":icons/16x16/go-home.png",QSize(16, 16));
    goHomeIcon.addFile(":icons/22x22/go-home.png",QSize(22, 22));

    findIcon.addFile(":icons/16x16/find.png",QSize(16, 16));
    findIcon.addFile(":icons/22x22/find.png",QSize(22, 22));

    zoomInIcon.addFile(":icons/16x16/zoom-in.png",QSize(16, 16));
    zoomInIcon.addFile(":icons/22x22/zoom-in.png",QSize(22, 22));

    zoomOutIcon.addFile(":icons/16x16/zoom-out.png",QSize(16, 16));
    zoomOutIcon.addFile(":icons/22x22/zoom-out.png",QSize(22, 22));

    closeIcon = style->standardIcon(QStyle::SP_TitleBarCloseButton);


}
