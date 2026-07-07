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


#ifndef BASICKEYBOARD_H
#define BASICKEYBOARD_H

#include <QObject>
#include <QKeyEvent>
//#include <QDebug>

class BasicKeyboard : public QObject
{
    Q_OBJECT
public:
    explicit BasicKeyboard();
    virtual ~BasicKeyboard();
    void keyPressed(QKeyEvent *e);
    void keyReleased(QKeyEvent *e);
    void reset();
    bool isPressed(int);
    int count();
    int getLastKey(int);

private:
    int lastKey;
    QString lastText;
    Qt::KeyboardModifiers lastModifiers;
    void setModifiers(Qt::KeyboardModifiers);
    void addKey(int);
    void addKey(int, int);
    void removeKey(int);
    void removeKey(int, int);
    std::list<std::pair<int, int>> pressedKeysMap;
};

#endif // BASICKEYBOARD_H
