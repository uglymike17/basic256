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

#include <QtGlobal>

#ifdef Q_OS_WASM

// WASM settings persistence (WASM.md Phase 7). On WebAssembly the Emscripten
// filesystem is ephemeral MEMFS, so anything QSettings writes is lost on
// reload. wasm-deploy/idbfs.js (a --pre-js) mounts IDBFS -- backed by the
// browser's IndexedDB -- at /persist and loads it before main(); Main.cpp
// points NativeFormat QSettings there. These helpers flush the mount back to
// IndexedDB (FS.syncfs(false)) after settings change so they survive a reload.
//
// The actual FS.syncfs always runs on the main thread (where the Emscripten FS
// lives); the helpers are safe to call from any thread (e.g. OP_SETSETTING on
// the interpreter thread) and marshal to the main thread as needed.
namespace WasmSettings {
    // Create the debounce timer. Call once, on the main thread (from Main.cpp).
    void init();

    // Debounced persist: coalesces bursts (e.g. SETSETTING in a loop) into a
    // single FS.syncfs ~1 s after the last call. Safe from any thread.
    void persistSoon();

    // Immediate persist -- for explicit, infrequent user actions (saving
    // Preferences, deleting settings) and on graceful quit, where waiting out
    // the debounce could lose the write if the tab is closed. Safe from any
    // thread, but intended for main-thread callers.
    void persistNow();
}

#endif // Q_OS_WASM
