/** Copyright (C) 2026, BASIC256 contributors
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

#pragma once

#include <QString>
#include <QUrl>
#include <QtGlobal>

// Where a media path (SOUNDLOAD, IMGLOAD, sprite/IMAGELOAD ...) is fetched from
// when it is not a local file.
//
// On the desktop a bare "sounds/bounce.mp3" is a path relative to the working
// directory, and that is where it is found. A browser has neither a working
// directory nor a filesystem -- the Emscripten MEMFS is empty -- so the same
// path used to resolve to nothing at all: SOUNDLOAD fell through to
// ERROR_SOUNDFILE and the image loads handed a scheme-less string to
// QUrl::fromUserInput(), which does not point at the server either.
//
// The browser's equivalent of "the working directory" is the page's own URL, so
// that is what a relative path is resolved against here. ./sounds/bounce.mp3
// next to basic256.html is fetched from the server that served the page, and the
// identical program keeps working unchanged on the desktop, where the path still
// hits the real CWD. Same-origin by construction (a relative path cannot name
// another host).
namespace MediaPath {

#ifdef Q_OS_WASM
// Capture the page URL. Call once from main(), on the **main thread**, before any
// program runs: `location` is a main-thread concept, and the interpreter runs on
// a worker, where it would refer to the worker script instead.
void init();
#endif

// The URL to fetch `path` from. Desktop: QUrl::fromUserInput(path), unchanged.
// WASM: a relative path is resolved against the page URL.
QUrl downloadUrl(const QString &path);

// Can `path` be fetched at all? Desktop (and WASM, for an absolute URL): only
// with an http/https/ftp scheme -- the pre-existing rule. WASM additionally
// accepts a relative path, which is the whole point of this file.
bool isFetchable(const QString &path);

} // namespace MediaPath
