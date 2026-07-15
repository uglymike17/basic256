// BASIC-256 WASM persistence bootstrap.
//
// The Emscripten filesystem is ephemeral MEMFS -- anything written during a
// session is gone on reload. Mount IDBFS (backed by the browser's IndexedDB)
// at /persist and load its contents BEFORE Qt's main() runs, so QSettings
// (pointed at /persist in Main.cpp) sees previously-saved preferences and
// SETSETTING data. Persisting back to IndexedDB (FS.syncfs(false)) happens
// from C++ after changes -- see src/core/WasmSettings.cpp.
//
// This file is added to the link as --pre-js (see CMakeLists.txt EMSCRIPTEN
// block), so it becomes part of basic256.js and runs regardless of the HTML
// shell (the deploy serves a vendored index.html, not Qt's basic256.html).
//
// addRunDependency / removeRunDependency gate main() until the async initial
// FS.syncfs(true) load completes -- essential, since Qt reads settings very
// early in startup. preRun runs on the main thread only (never on the pthread
// workers), which is exactly where the Emscripten FS lives and where the mount
// belongs.
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    try { FS.mkdir('/persist'); } catch (e) { /* already present -- fine */ }
    FS.mount(IDBFS, {}, '/persist');
    addRunDependency('idbfs-load');
    FS.syncfs(true, function (err) {
        if (err) console.error('BASIC-256: IDBFS initial load failed:', err);
        removeRunDependency('idbfs-load');
    });
});
