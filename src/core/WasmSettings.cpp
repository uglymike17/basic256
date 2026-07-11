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

#include "WasmSettings.h"

#ifdef Q_OS_WASM

#include <QObject>
#include <QThread>
#include <QTimer>
#include <emscripten.h>
#include <emscripten/em_js.h>

// Persist the /persist IDBFS mount (its in-memory image) to IndexedDB. Async;
// the callback only logs on error. Must run on the main thread, where the
// Emscripten FS lives.
EM_JS(void, wasmSettingsSyncToIdb, (), {
    if (typeof FS === "undefined" || !FS.syncfs) return;
    FS.syncfs(false, function (err) {
        if (err) console.error("BASIC-256: settings persist (syncfs) failed:", err);
    });
});

namespace {
// Single-shot debounce timer, created on (and thus owned by) the main thread
// in init(). All FS.syncfs calls are marshalled to this thread.
QTimer *g_timer = nullptr;

bool onTimerThread() {
    return g_timer && QThread::currentThread() == g_timer->thread();
}
}

namespace WasmSettings {

void init() {
    if (g_timer) return;
    g_timer = new QTimer();
    g_timer->setSingleShot(true);
    g_timer->setInterval(1000);   // debounce window
    QObject::connect(g_timer, &QTimer::timeout, [](){ wasmSettingsSyncToIdb(); });
}

void persistSoon() {
    if (!g_timer) return;
    // (Re)start the debounce timer on its own (main) thread; restarting
    // coalesces a burst of writes into one sync ~1 s after the last.
    if (onTimerThread()) {
        g_timer->start();
    } else {
        QMetaObject::invokeMethod(g_timer, [](){ if (g_timer) g_timer->start(); },
                                  Qt::QueuedConnection);
    }
}

void persistNow() {
    if (!g_timer) return;
    if (onTimerThread()) {
        // Direct call: on aboutToQuit the event loop is shutting down and a
        // queued invocation might not be processed, so flush inline here.
        g_timer->stop();
        wasmSettingsSyncToIdb();
    } else {
        QMetaObject::invokeMethod(g_timer,
                                  [](){ if (g_timer) g_timer->stop(); wasmSettingsSyncToIdb(); },
                                  Qt::QueuedConnection);
    }
}

} // namespace WasmSettings

#endif // Q_OS_WASM
