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

#include "MediaPath.h"

#ifdef Q_OS_WASM

#include <QByteArray>
#include <cstdint>
#include <emscripten.h>
#include <emscripten/em_js.h>

// Read location.href across the JS/wasm boundary using only HEAPU8 -- the same
// two-step dance as WasmLaunch, and for the same reason: this build sets no
// EXPORTED_RUNTIME_METHODS, and the makeDynCall episode showed that a helper
// which merely links can still be missing at runtime. JS never allocates wasm
// memory; it parks the bytes, reports the length, and C++ hands back a pointer.
EM_JS(int, mediaPathHrefSize, (), {
    var s = (typeof location !== "undefined" && location.href) ? location.href : "";
    Module.__mediaPathHref = new TextEncoder().encode(s);
    return Module.__mediaPathHref.length;
});

EM_JS(void, mediaPathHrefCopy, (int ptr, int len), {
    if (!Module.__mediaPathHref) return;
    HEAPU8.set(Module.__mediaPathHref.subarray(0, len), ptr);
    Module.__mediaPathHref = null;
});

namespace {
// The page URL, captured once on the main thread by init(). Written before any
// program starts and only ever read afterwards, so the interpreter thread can
// read it without synchronisation.
QUrl g_pageBase;
}

namespace MediaPath {

void init() {
    const int len = mediaPathHrefSize();
    if (len <= 0) return;
    QByteArray buf(len, char(0));
    mediaPathHrefCopy(static_cast<int>(reinterpret_cast<intptr_t>(buf.data())), len);
    g_pageBase = QUrl(QString::fromUtf8(buf));
}

} // namespace MediaPath

#endif // Q_OS_WASM

namespace MediaPath {

namespace {
bool hasNetworkScheme(const QUrl &u) {
    const QString s = u.scheme();
    return s == QLatin1String("http") || s == QLatin1String("https") ||
           s == QLatin1String("ftp");
}
}

QUrl downloadUrl(const QString &path) {
#ifdef Q_OS_WASM
    // Resolve a relative path against the page. QUrl::resolved() follows RFC 3986,
    // so "./sounds/x.mp3" against "http://host/dir/page.html?run=y" gives
    // "http://host/dir/sounds/x.mp3" -- the query is not inherited, which is what
    // we want, since the page may well have been launched with ?run=/?mode=.
    const QUrl given(path);
    if (given.isRelative() && !g_pageBase.isEmpty()) {
        return g_pageBase.resolved(given);
    }
#endif
    return QUrl::fromUserInput(path);
}

bool isFetchable(const QString &path) {
    if (path.isEmpty()) return false;
#ifdef Q_OS_WASM
    // A relative path is fetchable in the browser: it means "next to the page".
    if (QUrl(path).isRelative() && !g_pageBase.isEmpty()) return true;
#endif
    // The pre-existing rule, unchanged on the desktop.
    const QUrl u(path);
    return u.isValid() && hasNetworkScheme(u);
}

} // namespace MediaPath
