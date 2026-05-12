/** Copyright (C) 2017, Florin Oprea <florinoprea.contact@gmail.com>
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

#include <QGuiApplication>
#include "BasicKeyboard.h"

BasicKeyboard::BasicKeyboard() : QObject() {
    reset();
}

BasicKeyboard::~BasicKeyboard() {
}

void BasicKeyboard::keyPressed(QKeyEvent *e){
    // used by keyPressEvent(QKeyEvent *e) in widgets that catch key events
#ifdef MACX
    // nativeScanCode() Note: On Mac OS/X, this function is not useful, because there
    // is no way to get the scan code from Carbon or Cocoa. The function always
    // returns 1 (or 0).
    int code=e->nativeVirtualKey();
#else
    int code=e->nativeScanCode();
    if(!code) code=e->nativeVirtualKey();
#endif
    //qDebug() << "+key:" << e->key() << "=" << e->text() <<", code:" << code <<", isAutoRepeat:" << e->isAutoRepeat() <<", modifiers" << e->modifiers() << QGuiApplication::queryKeyboardModifiers();
    lastKey = e->key();
    lastText = e->text();
    if(!e->isAutoRepeat()){
        // Note that if the event is a multiple-key compressed event that is partly due to auto-repeat,
        // isAutoRepeat() function could return either true or false indeterminately.
        addKey(e->key(), code);
        setModifiers(QGuiApplication::queryKeyboardModifiers());
        // do not use e->modifiers()
        // QKeyEvent::modifiers() Warning: This function cannot always be trusted.
        // The user can confuse it by pressing both Shift keys simultaneously and releasing one of them, for example.
        // Also, Qt::KeyboardModifiers QGuiApplication::keyboardModifiers() may not reflect the actual keys held on
        // the input device at the time of calling but rather the modifiers as last reported by events.
        // QGuiApplication::queryKeyboardModifiers() is the one.
        // Queries and returns the state of the modifier keys on the keyboard. Unlike QGuiApplication::keyboardModifiers,
        // this method returns the actual keys held on the input device at the time of calling the method.
    }
}

void BasicKeyboard::keyReleased(QKeyEvent *e){
    // used by keyReleaseEvent(QKeyEvent *e) in widgets that catch key events
#ifdef MACX
    // nativeScanCode() Note: On Mac OS/X, this function is not useful, because there
    // is no way to get the scan code from Carbon or Cocoa. The function always
    // returns 1 (or 0).
    int code=e->nativeVirtualKey();
#else
    int code=e->nativeScanCode();
    if(!code) code=e->nativeVirtualKey();
#endif
    //qDebug() << "-key:" << e->key() << "=" << e->text() <<", code:" << code <<", isAutoRepeat:" << e->isAutoRepeat() <<", modifiers" << e->modifiers() << QGuiApplication::queryKeyboardModifiers();
    if(!e->isAutoRepeat()){
        removeKey(e->key(), code);
        setModifiers(QGuiApplication::queryKeyboardModifiers());
        // do not use e->modifiers()
        // QKeyEvent::modifiers() Warning: This function cannot always be trusted.
        // The user can confuse it by pressing both Shift keys simultaneously and releasing one of them, for example.
        // Also, Qt::KeyboardModifiers QGuiApplication::keyboardModifiers() may not reflect the actual keys held on
        // the input device at the time of calling but rather the modifiers as last reported by events.
        // QGuiApplication::queryKeyboardModifiers() is the one.
        // Queries and returns the state of the modifier keys on the keyboard. Unlike QGuiApplication::keyboardModifiers,
        // this method returns the actual keys held on the input device at the time of calling the method.
    }
}

void BasicKeyboard::reset(){
    // reset the map of keys and the last pressed key
    // used when program starts or when widget lose focus to avoid
    // releasing keys outside and to be detectes=d as pressed
    lastKey = 0;
    lastText = QString();
    lastModifiers = 0;
    pressedKeysMap.clear();
}

bool BasicKeyboard::isPressed(int key){
    // check if specified key is pressed
    // used by BASIC-256 KEYPESSED(key_code) function
    for (std::list<std::pair<int, int>>::iterator it=pressedKeysMap.begin(); it != pressedKeysMap.end(); ++it)
        if((*it).first == key) return true;

    return false;
}

int BasicKeyboard::count(){
    // return the number of pressed keys
    // used by BASIC-256 KEYPESSED() function
    return pressedKeysMap.size();
}

int BasicKeyboard::getLastKey(int getUNICODE){
    // return the last pressed key and reset the value
    // used by BASIC-256 KEY() function
    int k;
    if (getUNICODE) {
		if (!lastText.isEmpty()) {
			k = (int) lastText[0].unicode();
		} else {
			k = 0;
		}
	} else {
		k = lastKey;
	}
	lastKey = 0;
    lastText = QString();
	return k;
}

void BasicKeyboard::setModifiers(Qt::KeyboardModifiers modifiers){
    // used by both: keyReleased and keyPressed
    // to set up special keys as pressed or released
    // Under windows: 1) press left shift and keep pressing it
    // 2) press right shift and keep pressing it
    // 3) release left shift 4) release right shift
    // RESULT: left shift will send no release code and will be on keymap as pressed
    // This check is made to ensure that no special keys are forgotten

    if (lastModifiers == modifiers) return;
    //qDebug() << "changes" << lastModifiers << modifiers;
    lastModifiers = modifiers;

    if( modifiers & Qt::ShiftModifier )
    {
            addKey(Qt::Key_Shift);
    }else{
            removeKey(Qt::Key_Shift);
    }
    if( modifiers & Qt::ControlModifier )
    {
            addKey(Qt::Key_Control);
    }else{
            removeKey(Qt::Key_Control);
    }
    if( modifiers & Qt::AltModifier )
    {
            addKey(Qt::Key_Alt);
    }else{
            removeKey(Qt::Key_Alt);
    }
    if( modifiers & Qt::MetaModifier )
    {
            addKey(Qt::Key_Meta);
    }else{
            removeKey(Qt::Key_Meta);
    }
}

// keys are mapped as pairs: Qt::Key value, and key code (nativeScanCode/nativeVirtualKey) for better results
// Why? If we map only the Qt::Key values then we can have some situation when keys are reported wrongly
// + is pressing key, - is releasing key
//
// Situation:
// press an hold shift   : + 16777248 = "" , code: 42 ,modifiers QFlags<Qt::KeyboardModifiers>(ShiftModifier)
// press key "2"         : + 64 = "@" , code: 3 , modifiers QFlags<Qt::KeyboardModifiers>(ShiftModifier)
// release shift         : - 16777248 = "" , code: 42 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
// keep pressing key "2" : + 50 = "2" , code: 3 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
//
// As you can see, the Qt::Key 64 ("@") is not marked as released, instead, after releasing shift, the key is transformed
// into Qt::Key 50 = "2". If we use only Qt::Key values then, in this situation, key Qt::Key 64 ("@") will appear as pressed.
// To fix this we need to map and look after key code (nativeScanCode/nativeVirtualKey) - if we have one.
//
// Situation:
// press an hold key "1" : + 49 = "1" , code: 2 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
// press keypad "1"      : + 49 = "1" , code: 79 , modifiers QFlags<Qt::KeyboardModifiers>(KeypadModifier)
//
// If we search only after Qt::Key value, then the number of pressed keys is 1
// If user release any of keys but hold one of them, the number of pressed keys will be 0
// even the fact that there is still a key pressed.
// To fix this we need to map and look after key code (nativeScanCode/nativeVirtualKey) - if we have one.
//
// Another situation is when user keep pressing a key while switch the keyboard layout by pressing (Alt+Shift in Windows).
// When user release the key, if the key have another role in the new layout, when is released another Qt::Key value is
// returned even the fact that both have the same key code.
//
// press an hold "ș" key (RO)     : + 536 = "?" , code: 39 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
// Alt+Shift - change layout      : + 16777251 = "" , code: 56 , modifiers QFlags<Qt::KeyboardModifiers>(AltModifier)
//                                : + 16777248 = "" , code: 42 , modifiers QFlags<Qt::KeyboardModifiers>(ShiftModifier|AltModifier)
//                                : - 16777248 = "" , code: 42 , modifiers QFlags<Qt::KeyboardModifiers>(AltModifier)
//                                : - 16777249 = "" , code: 29 , modifiers QFlags<Qt::KeyboardModifiers>(AltModifier)
//                                : - 16777251 = "" , code: 56 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
// keep pressing ș which is now ; : + 59 = ";" , code: 39 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)
// release the key                : - 59 = ";" , code: 39 , modifiers QFlags<Qt::KeyboardModifiers>(NoModifier)


void BasicKeyboard::addKey(int key){
    addKey(key, 0);
}
void BasicKeyboard::addKey(int key, int keycode){
    std::list<std::pair<int, int>>::iterator it=pressedKeysMap.begin();
    // we have a key code (nativeScanCode/nativeVirtualKey)
    if(keycode){
        while (it != pressedKeysMap.end()){
            // delete all pressed keys with the same key code (nativeScanCode/nativeVirtualKey)
            // or keys with the same Qt::Key value but no key code (nativeScanCode/nativeVirtualKey)
            if((*it).second == keycode || ((*it).second == 0 && (*it).first == key)){
                pressedKeysMap.erase(it++);
            }else{
                ++it;
            }
        }
        // add new key as pressed
        pressedKeysMap.push_front(std::make_pair(key, keycode));
    }else{
        // we do not have a key code (nativeScanCode/nativeVirtualKey)
        // check if there is already pressed a key with Qt::Key value
        while (it != pressedKeysMap.end()){
            if((*it).first == key){
                // already pressed
                return;
            }else{
                ++it;
            }
        }
        // add new key as pressed
        pressedKeysMap.push_front(std::make_pair(key, 0));
    }
}

void BasicKeyboard::removeKey(int key){
    removeKey(key, 0);
}
void BasicKeyboard::removeKey(int key, int keycode){
    std::list<std::pair<int, int>>::iterator it=pressedKeysMap.begin();
    // we have a key code (nativeScanCode/nativeVirtualKey)
    if(keycode){
        // delete all pressed keys with the same key code (nativeScanCode/nativeVirtualKey)
        // or keys with the same Qt::Key value but no key code (nativeScanCode/nativeVirtualKey)
        while (it != pressedKeysMap.end()){
            if((*it).second == keycode || ((*it).second == 0 && (*it).first == key)){
                pressedKeysMap.erase(it++);
            }else{
                ++it;
            }
        }
    }else{
        while (it != pressedKeysMap.end()){
            if((*it).first == key){
                pressedKeysMap.erase(it++);
            }else{
                ++it;
            }
        }
    }
}
