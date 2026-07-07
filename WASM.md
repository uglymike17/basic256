# BASIC256 – WebAssembly Port Plan (Claude Code execution plan)

**Goal:** restructure the tree so the interpreter core is separated from the
Qt Widgets GUI and from desktop-only platform services, then add a browser
(WebAssembly) build — while the existing desktop binary keeps its current
behavior: one executable with the IDE by default and `-g`/`-t`/`-s` modes for
graph-only / text-only / headless CLI use.

**Decisions already made by the maintainer (do not re-litigate in-session):**

- **Single desktop binary.** No separate console executable. `-s`/`-t` stay as
  the CLI story. The core/GUI split is architectural, not a product split.
- **Browser feature scope v1:** `SYSTEM` (QProcess), serial port
  (`SERIALOPEN` etc.), `NETSERVER`/TCP sockets, `DBOPEN`/SQL, `PRINTER…`,
  and `SAY` (TextToSpeech) are **stubbed in the browser** with a clear
  runtime BASIC error. Never a silent no-op.
- **Hosting:** GitHub Pages (static; cannot send COOP/COEP headers — the
  `coi-serviceworker` shim is part of the plan, see Phase 6).
- **File moves are allowed.** Work happens on a dedicated branch; sources move
  into `src/core/`, `src/gui/`, `src/app/`.
- **Qt version: current LTS = Qt 6.11.x.** Its pinned Emscripten is **4.0.7**
  (per Qt's own "Qt for WebAssembly" page for 6.11.1 — each Qt minor targets
  one exact emsdk version; binary packages are built with it and Emscripten
  does not guarantee ABI compatibility across versions, so do not drift).

This file is the source of truth for the port. Work top to bottom. Tick each
`[ ]` box as it lands and **do not skip the gate at the end of each phase.**

---

## How to drive this with Claude Code

- One **phase per session** where possible. Don't start a later phase until
  the earlier one's gate is green.
- At the **start of a session**: *"Read `WASM.md`, continue from the first
  unticked box, make only the edits described, then run/prepare the gate for
  that phase."*
- At the **end of a session**: tick completed boxes, add a one-line entry
  under **Session log**.
- Line numbers below were captured on 2026-07-06 and will drift as files move.
  **Match on the code string, never on the number.**
- **Never invent package names, aqt arch names, or Qt WASM API behavior.**
  Where this file says "verify", that means web-search the official docs or
  wait for a real CI log — exactly the discipline that worked for the Qt6
  migration.

---

## Global rules (apply to every phase)

**RULE 1 – The four desktop CI targets are the regression net.** Windows,
macOS, Linux x86_64, Linux ARM64 must build *and package* green after every
phase. A restructure phase that breaks packaging scripts is not done until
the scripts are fixed too (they reference source paths and the binary name).

**RULE 2 – Code that runs on the WASM *main* thread must never block.**
Qt for WebAssembly schedules all timers on the main thread; a blocked main
thread also stops servicing worker threads. The interpreter (a `QThread`,
i.e. a web worker in WASM) *may* block on `waitCond`/`Sleeper` — that is the
whole point of choosing the multithreaded WASM build. But GUI-side waits,
nested `exec()` loops, and `QThread::wait()` from the main thread are
forbidden in the browser. Phase 5 audits these.

**RULE 3 – Feature removal = compile-time flag + runtime BASIC error.**
Every browser-unavailable subsystem gets a CMake option
(`BASIC256_ENABLE_*`), an `#if BASIC256_ENABLE_*` guard, and — when the
guard is off — the corresponding opcode raises a normal BASIC error
("FEATURE NOT AVAILABLE ON THIS PLATFORM") through the existing `Error`
mechanism. Programs must fail loudly and debuggably, never mysteriously.
Precedent already in-tree: the `#ifdef ANDROID` guards in `Interpreter.cpp`
(e.g. ~2427, ~4761).

**RULE 4 – Core may use QtCore and QtGui-painting, nothing widget.**
"Core" is not "Qt-free". `QString`, `QImage`, `QPainter`, `QThread`,
`QMutex`, `QRegularExpression` are all fine in WASM. What core must not
touch: anything from QtWidgets, and anything behind a Phase-3 feature flag
without its guard.

---

## PHASE 0 – Prerequisites (maintainer + one small session)

- [x] **Sound is audible on all four desktop targets.** (Maintainer task,
      already flagged as the open Phase 1/2 runtime gates in
      `QT6_MIGRATION_CHECKLIST.md` — BEEP/waveform via `QAudioSink` *and*
      file playback via `QMediaPlayer`.) Do not start Phase 4+ without this:
      WASM audio is the flakiest subsystem and needs a known-good desktop
      baseline to compare against. **Confirmed by maintainer 2026-07-07.**
- [x] **Bump all desktop CI to Qt 6.11.x LTS.** Currently CI installs 6.7.3
      via aqt (Windows, Linux x86_64) and distro Qt on RPi/macOS. Change the
      aqt version pins in `.github/scripts/build_Windows.ps1` and
      `build_Linux_x86.sh` to the latest public 6.11.x. Verify (web search /
      first CI log) that the aqt arch names are unchanged for 6.11
      (`win64_msvc*`, `linux_gcc_64`). RPi (Debian Trixie apt = 6.8.2) and
      macOS (Homebrew qt) stay on their distro versions — acceptable skew;
      the WASM build is what's pinned hard to 6.11.
      **Done:** bumped both scripts to Qt `6.11.1` (latest public patch as of
      2026-07-07). The arch name is **not** unchanged — aqtinstall dropped
      `win64_msvc2019_64` some versions back; Qt ≥ 6.8 Windows desktop only
      ships `win64_msvc2022_64` (confirmed via aqtinstall docs, since
      `windows-2022` runners already carry MSVC 2022 this is a drop-in
      swap). `linux_gcc_64` is unchanged for 6.11. Note:
      `.github/workflows/build.yml:28` has an unused, now-stale
      `qt_arch: win64_msvc2019_64` matrix value (never referenced by any
      step) — left alone since it's dead code, not an active pin, but worth
      deleting in Phase 1's CMake cleanup. `COMPILING.txt`'s `6.7.3` /
      `win64_msvc2019_64` references are intentionally left for Phase 1C
      (`COMPILING.txt` is explicitly in that phase's scope, not Phase 0's).
      **Real CI blocker found and fixed:** aqtinstall 3.3.0 (latest on
      PyPI) cannot parse Qt's Windows repository layout for 6.11.0+ (old
      nested `qt6_6111/qt6_6111/Updates.xml` path removed, replaced with
      flat per-arch subdirs — aqtinstall issue #1007). The fix (PR #1000)
      merged upstream 2026-03-24 but has never shipped in a PyPI release.
      Linux/macOS aren't affected (their repo layout didn't change).
      Maintainer chose to pin `build_Windows.ps1` to install aqtinstall
      straight from the merge commit
      (`git+https://github.com/miurahr/aqtinstall.git@8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b`)
      rather than wait for a release or drop Windows to 6.10.x. **Revisit:**
      swap back to a plain `pip install aqtinstall` once a PyPI release
      contains PR #1000.
- [x] **CI green on all four targets at the new Qt version.**
      **Confirmed 2026-07-07:** GitHub Actions run
      [28862119754](https://github.com/uglymike17/basic256/actions/runs/28862119754)
      — Windows, Linux x86_64, Linux ARM64, macOS all build, package, and
      TestSuite-pass green on Qt 6.11.1 (Windows via the pinned aqtinstall
      commit above).
- [x] Record here the exact versions pinned: Qt `6.11.1`, emsdk `4.0.7`.

### Phase 0 gate
- [x] Four desktop targets build, package, and TestSuite-pass on Qt 6.11.x.
      See run 28862119754 above.
- [x] Maintainer confirms audio by ear on at least Windows + one Linux.
      **Confirmed 2026-07-07 (maintainer stated audio is OK across all four
      desktop targets, superseding the earlier "Windows + one Linux"
      minimum bar).**

---

## PHASE 1 – Source tree restructure + `basic256core` static library

Pure mechanical phase: move files, split CMake into a static core library
plus the app target, fix every path reference. **Zero behavior change.**

### 1A. New layout

```
src/
  core/      # interpreter + language runtime (QtCore/QtGui only + flags)
  gui/       # everything QtWidgets
  app/       # Main.cpp, RunController glue, resources wiring
LEX/         # stays at top level for now (plain C, referenced by core)
```

Proposed placement (verify each file's include graph with grep before
moving; if a "core" file includes a QtWidgets header, stop and note it —
that's Phase 2 work, don't silently reclassify):

- [x] `src/core/`: `Interpreter.{h,cpp}`, `Stack.{h,cpp}`,
      `Variables.{h,cpp}`, `DataElement.{h,cpp}`, `Convert.{h,cpp}`,
      `Error.{h,cpp}`, `ErrorCodes.h`, `CompileErrors.h`, `WordCodes.h`,
      `Constants.h`, `BasicTypes.h`, `md5.{h,cpp}`, `Sleeper.{h,cpp}`,
      `Sound.{h,cpp}`, `BasicDownloader.{h,cpp}`, `BasicMediaPlayer.{h,cpp}`,
      `BasicKeyboard.{h,cpp}` (it is a `QObject`, not a widget —
      `BasicKeyboard.h:26`), `Version.h`.
      **Known impurity carried into Phase 2:** `Interpreter.h:30` includes
      `BasicGraph.h` (a widget). It moves anyway; Phase 2 removes the
      include. `Interpreter.cpp:6541`'s `editwin`/`BasicEdit.h` use is the
      same story (Phase 2B).
      **Extra impurity found and fixed (not carried forward):**
      `Interpreter.cpp` also had a stray, **unused**
      `#include <QtWidgets/QMessageBox>` (no `QMessageBox::` call anywhere
      in the file) — deleted outright rather than carried into Phase 2,
      since there was no real widget dependency behind it, just dead code.
- [x] `src/gui/`: `MainWindow.*`, `BasicEdit.*`, `BasicGraph.*`,
      `BasicOutput.*`, `BasicWidget.*`, `BasicDock.*`, `BasicIcons.*`,
      `EditSyntaxHighlighter.*`, `LineNumberArea.*`, `PreferencesWin.*`,
      `ReplaceWin.*`, `VariableWin.*`, `ViewWidgetIFace.*`, `Settings.h`.
- [x] `src/app/`: `Main.cpp`, `RunController.{h,cpp}`.
- [x] `git mv` (not delete+add) so history follows the files.

### 1B. CMake

- [x] `add_library(basic256core STATIC ...)` with the `src/core/` sources +
      the LEX outputs; `target_link_libraries(basic256core PUBLIC Qt6::Core
      Qt6::Gui Qt6::Multimedia Qt6::Network ...)` — move each Qt component
      to the target that actually needs it (Widgets/PrintSupport stay on the
      app; Sql/SerialPort on core *until* Phase 3 flags them).
      **Correction from grep evidence, not guessed:** the "Widgets/
      PrintSupport stay on the app" assumption was wrong for PrintSupport.
      `Interpreter.h` directly includes `QtPrintSupport/QPrinter` and
      `QPrinterInfo` (`PRINTER...`/print-doc opcodes) and `Interpreter.cpp`
      constructs `QPrinter`/`QPrinterInfo` at ~6524-6532 — that's core code,
      so `basic256core` links `Qt6::PrintSupport` too. Full core link set
      (all confirmed by grepping actual `#include`/class usage, not the
      plan's shorthand): `Core`, `Gui`, `Network` (QTcpSocket/QTcpServer/
      QNetworkInterface/QHostInfo in `Interpreter.h`, plus
      `BasicDownloader`'s `QNetworkAccessManager`), `Sql`, `SerialPort`,
      `PrintSupport`, `Multimedia` (`Sound.h`/`BasicMediaPlayer.h`). Only
      `Widgets` and `TextToSpeech` stay app-only (`TextToSpeech` is used
      exclusively by `RunController.cpp`, confirmed via grep — matches the
      plan's own Phase 3 note that `BASIC256_ENABLE_TTS` guards
      `RunController.cpp:156-190`).
- [x] `add_executable(basic256 ...)` = `src/gui/` + `src/app/`, links
      `basic256core`.
- [x] `target_include_directories` so old flat `#include "X.h"` lines keep
      resolving (add both `src/core` and `src/gui` publicly for now; tighten
      later). Prefer this over touching hundreds of includes in this phase.
      Verified by mechanically checking every quoted `#include` under `src/`
      resolves to same-dir/`src/core`/`src/gui`/`src/app` (the only
      unresolved hit was the build-generated `basicParse.tab.h`, as
      expected). `basic256` (the exe) additionally needs `src/app` on its
      own include path — `src/gui/MainWindow.h` does
      `#include "RunController.h"`, which lives in `src/app`.
- [x] Keep translation, resource, and install rules working (paths to
      `resources/`, `Translations/` referenced from CMake). Unchanged;
      those paths were already relative to the repo root, not the moved
      sources.

### 1C. Everything that hardcodes paths

- [x] `.github/scripts/*` (13 scripts): grep for every `.cpp`/`.h`/dir
      reference and fix. The packaging scripts mostly consume the built
      binary, but verify each.
      **Verified, zero changes needed:** grepped all 13 scripts for any
      `.cpp`/`.h`/`src/` reference — none exist. They only reference the
      build output (e.g. `build\Release\basic256.exe`), and the CMake
      target name/output location is unchanged by this restructure.
- [x] `BASIC256.nsi`, `COMPILING.txt`, `COMPILING_RaspberryPI.txt`,
      `README.md` build instructions.
      `BASIC256.nsi`/`COMPILING_RaspberryPI.txt`/`README.md`: verified, no
      source-path or version references, zero changes needed.
      `COMPILING.txt`: had the `6.7.3`/`win64_msvc2019_64` references
      deliberately left over from the Phase 0 session (see that phase's log
      entry) — updated now to `6.11.1`/`win64_msvc2022_64`, plus a note
      about the aqtinstall Windows workaround from `build_Windows.ps1`.
- [x] LEX build step: the flex/bison invocation and the
      `include_exec_path` C externs (`Interpreter.cpp:77-90`) — confirm the
      generated parser still compiles into `basic256core`.
      Confirmed structurally: `BISON_BasicParser_OUTPUTS`/
      `FLEX_BasicLexer_OUTPUTS` are folded into `SOURCES_CORE`, so the
      generated `basicParse.tab.c`/`lex.yy.c` compile as part of
      `basic256core` alongside `Interpreter.cpp`, which shares C-linkage
      externs with them (`include_exec_path` et al.) — same translation
      unit grouping as before, just inside a static-lib target instead of
      the monolithic exe target.
      **Not called out by this checklist item but found and fixed:**
      `LEX/basicParse.y` and `LEX/basicParse.l` `#include "../X.h"` the
      moved headers by relative path (`../BasicTypes.h`,
      `../CompileErrors.h`, etc.) — these resolved to the repo root, which
      no longer has them. Repointed to `../src/core/X.h`.
      `resources/windows.rc` and `resources/basic256.rc` had the same
      `#include "../Version.h"` relative-path problem — also repointed to
      `../src/core/Version.h`.

### Phase 1 gate
- [x] All four desktop CI targets build + package green. No functional diff
      expected; run the TestSuite gate as usual.
      **No local Qt/flex/bison toolchain available in this session** (no
      `cmake`/`qmake`/`flex`/`bison` on PATH) to do a real local
      compile-and-link sanity check before pushing, so this phase leaned
      more heavily than usual on static verification: every quoted
      `#include` under `src/` was mechanically checked to resolve, CMake
      `if`/`endif` and paren balance were checked, and every Qt module
      actually referenced by each moved file was grepped directly rather
      than assumed from the plan's shorthand (see 1B correction above).
      **Confirmed 2026-07-07:** GitHub Actions run
      [28864003223](https://github.com/uglymike17/basic256/actions/runs/28864003223)
      — Windows, Linux x86_64, Linux ARM64, macOS all build, package, and
      TestSuite-pass green, first try, no CI fixes needed.

---

## PHASE 2 – Cut the interpreter's direct lines to the GUI

Today `Interpreter.cpp` reaches the GUI through raw externs
(`Interpreter.cpp:64-73`): `sound`, `mymutex`, `mydebugmutex`, `waitCond`,
`waitDebugCond`, `graphwin`, `editwin`, `basicKeyboard`, `guiState`. The
mutex/waitcond ones are fine (they work in WASM pthreads — leave them). The
widget pointers are the problem. Good news: the actual usage surface is
narrow.

### 2A. `GraphicsBuffer` — the drawing state moves out of the widget

What the interpreter actually touches on `graphwin` (grep-verified):
`image`, `spritesimage`, `displayedimage`, `image->copy/fill/pixel/save/
width/height`, `sprites_clip_region`, `draw_sprites_flag`, and the
mouse/click state (`mouseX/Y/B`, `clickX/Y/B`).

- [x] New core class `GraphicsBuffer` (plain `QObject` or even plain class,
      `src/core/`): owns the `QImage`s, sprite clip region, draw flags, and
      the mouse/click atomics. No painting-to-screen, no QWidget.
      Implemented as a plain class (no `Q_OBJECT` — it has no signals/slots,
      so no reason to pay for moc) in `src/core/GraphicsBuffer.{h,cpp}`:
      owns `image`/`displayedimage`/`spritesimage`, `sprites_clip_region`,
      `draw_sprites_flag`, `mouseX/Y/B`, `clickX/Y/B`; `resizeBuffers(w,h)`
      (the buffer-management half of the old `BasicGraph::resize`) and
      `updateScreenImage()` (sprite compositing, moved verbatim).
- [x] `BasicGraph` (widget) becomes a *view*: holds a `GraphicsBuffer*`,
      paints `displayedimage` in `paintEvent`, and writes mouse/click state
      into the buffer from its mouse events.
      `BasicGraph::graphics` is created in `BasicGraph`'s constructor and
      deleted in its destructor. `resize()` now calls
      `graphics->resizeBuffers(w,h)` for the buffer half and keeps
      `gridlinesimage`/window-fit/transform logic (genuinely view-only,
      untouched). All mouse-event handlers, `paintEvent`, `slotCopy`,
      `slotPrint`, `slotClear` now go through `graphics->`. Caught one bug
      while converting `mouseMoveEvent`: a tooltip line read the old
      (now-removed) `mouseX`/`mouseY` widget members directly, which would
      have been a compile error post-refactor — fixed to use the already-
      equal local `x`/`y` instead. `updateScreenImage()` stays on
      `BasicGraph` as a one-line delegator to `graphics->updateScreenImage()`
      so `RunController.cpp`'s call site didn't need to change.
- [x] Interpreter: replace every `graphwin->X` with `graphics->X` where
      `graphics` is a `GraphicsBuffer*` passed in at construction (extend
      the existing `Interpreter(mainwin->locale)` construction in
      `RunController.cpp:84`). Delete `extern BasicGraph * graphwin;` from
      `Interpreter.cpp` and remove `#include "BasicGraph.h"` from
      `Interpreter.h:30`.
      Done exactly as described:
      `Interpreter(QLocale*, GraphicsBuffer*, BasicKeyboard*)` (the
      `basicKeyboard` param is 2B, same commit). `RunController.cpp:85`
      now does `new Interpreter(mainwin->locale, graphwin->graphics,
      basicKeyboard)` — `graphwin` (and `basicKeyboard`) are still
      MainWindow-owned GUI-layer globals; only the *interpreter's* access
      is now constructor injection instead of an extern reach-in.
- [ ] **Deferred, not done this session (maintainer decision 2026-07-07):**
      "In `-s` silent mode, `Main.cpp` constructs a bare `GraphicsBuffer`
      with no view." Checked `Main.cpp` first: this doesn't match how `-s`
      actually works today. `MainWindow` (and its *entire* widget tree —
      `BasicGraph`, `BasicEdit`, `BasicOutput`, docks, `RunController` /
      `Interpreter`) is unconditionally constructed in every mode including
      `-s`; it is only never `.show()`n. Skipping that construction for
      `-s` would require a second, headless creation path for
      `RunController`/`Interpreter` that doesn't exist today — a materially
      bigger and riskier change than "extract `GraphicsBuffer`", touching
      exactly the code path CI's TestSuite runs through. Left `Main.cpp`
      untouched; `-s` mode's behavior is unchanged (still builds the full,
      hidden widget tree, `graphwin->graphics` still exists and is what
      `-s` mode's `Interpreter` uses). Revisit as a real Phase 2D or fold
      into Phase 4 if WASM's headless/startup-cost needs actually require
      it.

### 2B. The two small ones

- [x] `editwin`: used exactly once in the interpreter
      (`Interpreter.cpp:6541`, print doc name). Replace with a plain
      `QString programTitle` member on Interpreter, set by RunController
      before each run. Delete the extern.
      Done. `Interpreter::programTitle` (public `QString`) replaces
      `editwin->title` at the one call site
      (`printdocument->setDocName(...)`). `RunController.cpp` sets
      `i->programTitle = currentEditor->title;` in both `startDebug()` and
      `startRun()`, right after `currentEditor = editwin;` is confirmed
      non-null — i.e. before every run, exactly as specified. Also dropped
      the now-dead `#include "BasicEdit.h"` from `Interpreter.cpp`.
- [x] `basicKeyboard`: 5 uses; it's already a portable `QObject`. Pass the
      pointer in via constructor instead of extern (GUI feeds it key events
      exactly as today).
      Done — see the Interpreter constructor change above.
      `MainWindow.cpp` still owns/creates the global `basicKeyboard`
      (unchanged) and still feeds it key events via `BasicGraph`/
      `BasicOutput`'s own `extern BasicKeyboard *basicKeyboard;` (GUI-layer
      externs are fine — RULE 3/2 is specifically about the *interpreter*
      not reaching into GUI globals). `RunController.cpp` picked up its own
      `extern BasicKeyboard * basicKeyboard;` to pass the pointer through
      at `Interpreter` construction.

### 2C. Signals stay

The interpreter's `signals:` block (output text, dialogs, clipboard, etc.)
already crosses threads correctly via queued connections into
RunController/MainWindow — that is the right mechanism for WASM too. No
change, but:

- [x] Grep every `connect(` on Interpreter signals for
      `Qt::BlockingQueuedConnection` or return-value emits
      (e.g. `returnImage`, `Interpreter.h:189`) and list them in this file —
      they block the *interpreter* thread (fine), but confirm none block the
      GUI thread waiting on the interpreter.
      **Audit result, all clear, nothing to fix:**
      - `Qt::BlockingQueuedConnection` (5 sites, all in
        `RunController.cpp`'s constructor / `startDebug()` / `startRun()`):
        `varWinAssign(Variables**, int, int)`,
        `varWinAssign(Variables**, int, int, int, int)`,
        `varWinDropLevel(int)` (all → `varwin`), and `seekLine(int)` (→
        `currentEditor`, connected once in each of `startDebug()`/
        `startRun()`). All are emitted *from the interpreter thread*, which
        blocks until the GUI-thread slot returns — that's RULE 2's
        explicitly-fine case (interpreter waits on GUI, not the reverse).
        None are emitted from GUI code waiting on the interpreter.
      - Return-value emits (`returnImage`/`returnInt`, e.g.
        `getClipboardImage()`/`getClipboardString()`/`setClipboardImage()`):
        these use default (queued, non-blocking-at-emit) connections, and
        the interpreter thread instead blocks itself on the existing
        `mymutex`/`waitCond` pattern (`waitCond->wait(mymutex)` after
        `emit`, `waitCond->wakeAll()` in the GUI-thread slot after it's
        done). Same shape as the `BlockingQueuedConnection` cases: the
        interpreter thread waits, the GUI thread's slot runs to completion
        and returns immediately, never blocking on the interpreter. This
        is also the exact mechanism `goutputReady()` uses to synchronize
        `graphwin->updateScreenImage()`/`graphwin->update()` after sprite
        draws — traced end-to-end while doing the 2A work, confirms the
        `GraphicsBuffer` refactor didn't touch this synchronization at all.
      - No `connect(` on any Interpreter signal exists outside
        `RunController.cpp` — it is the sole hub, as this section assumed.
      - Pre-existing, unrelated to this phase: `varWinAssign(Variables**,
        int, int, QString)` and `varWinDimArray(Variables**, int, int,
        int)` are declared as interpreter signals but have no `connect(`
        anywhere — dead signals. Not a Phase 2 concern, left alone.

### Phase 2 gate
- [ ] Desktop CI green ×4 + TestSuite.
- [ ] **Manual verification needed from the maintainer** (no local Qt
      toolchain or display available this session, same constraint as
      Phases 0-1): run a graphics example, a sprite example, and a
      mouse-input example in the IDE; run a graphics program under `-s`
      and confirm `IMGSAVE` produces the same PNG as before.
      Note while investigating this: `TestSuite/testsuite_ci.kbs` (the
      automated `-s` subset CI already runs) *deliberately excludes*
      sprite/graphics/image tests for a documented, pre-existing reason
      unrelated to this refactor — `-s` mode's `waitForGraphics()` skips
      sprite compositing entirely (`if (guiState == GUISTATESILENT)
      return;` before `update_sprite_screen()`), so the on-screen buffer
      never changes under `-s` and those assertions are unwinnable
      regardless of correctness (see the comment above the sprite
      `include` line in `testsuite_ci.kbs`). `IMGSAVE` itself saves
      `graphics->image` (the raw draw buffer), not `displayedimage`, so
      it's unaffected by that skip and should be safe to verify under
      `-s` — but this still needs an actual maintainer-run comparison,
      not something CI's existing gate covers.
- [x] `grep -n "extern BasicGraph\|extern BasicEdit\|extern BasicKeyboard"
      src/core/` returns nothing.
      Confirmed clean.

---

## PHASE 3 – Platform feature flags (+ the desktop dress rehearsal)

Gate the six browser-absent subsystems. Do this on desktop, prove it on
desktop, *then* go near Emscripten — a desktop build with all flags OFF is a
cheap simulation of the WASM feature surface with a debugger available.

- [ ] Add a new error code `ERROR_NOTAVAILABLE` ("Feature not available on
      this platform") in `ErrorCodes.h` + message in `Error.cpp`.
- [ ] CMake options, all `ON` by default:
      `BASIC256_ENABLE_PROCESS` (QProcess / `OP_SYSTEM`,
      `Interpreter.cpp:3954`),
      `BASIC256_ENABLE_SERIAL` (QSerialPort, `Interpreter.cpp:~2446`),
      `BASIC256_ENABLE_SQL` (QtSql, `Interpreter.cpp:~1023, ~5372`),
      `BASIC256_ENABLE_PRINTER` (QPrintSupport, `Interpreter.cpp:~980,
      ~4630`),
      `BASIC256_ENABLE_TCP` (QTcpServer/QTcpSocket, `Interpreter.cpp:~5597,
      ~5632` — note plain sockets partially exist in Qt WASM via
      Emscripten tunneling, but a BASIC `NETSERVER` cannot listen in a
      browser; gate it all in v1),
      `BASIC256_ENABLE_TTS` (QTextToSpeech — lives in
      `RunController.cpp:156-190`, so this flag guards app-layer code and
      the `SAY` signal path raises `ERROR_NOTAVAILABLE`).
- [ ] Each flag: wrap includes, members, and opcode bodies; the `#else`
      branch of each opcode = `error->q(ERROR_NOTAVAILABLE);` (mirror the
      existing ANDROID-guard style). CMake only links the Qt component when
      its flag is on.
- [ ] **Dress rehearsal:** add a scratch desktop CI job (or local build)
      with *all six flags OFF*. It must compile, run the TestSuite subset,
      and a test `.kbs` calling `SYSTEM` must print the new error and
      continue per normal error semantics.

### Phase 3 gate
- [ ] Default (all-ON) desktop CI green ×4 — byte-for-byte same feature set
      as before.
- [ ] Flags-OFF desktop build green + error-path verified.

---

## PHASE 4 – Emscripten toolchain + first WASM build

- [ ] Toolchain in CI (new job in `build.yml`):
      emsdk **4.0.7** (`emsdk install 4.0.7 && emsdk activate 4.0.7`), plus
      Qt 6.11.x **wasm multithreaded** binaries and a same-version desktop
      host Qt (needed for `QT_HOST_PATH` — moc/rcc run on the host).
      Install both via aqtinstall. **Verify the aqt invocation against
      current aqt docs before writing it** (for Qt ≥ 6.8 the wasm packages
      moved under the `all_os`/`wasm` host/target with arch
      `wasm_multithread`; the desktop-host requirement is unchanged). Same
      no-guessing rule as the Qt6 migration's aqt work — expect one
      round-trip on names.
- [ ] Configure with Qt's toolchain file
      (`<qt-wasm>/bin/qt-cmake -S . -B build-wasm
      -DQT_HOST_PATH=<qt-host> -DBASIC256_ENABLE_{PROCESS,SERIAL,SQL,
      PRINTER,TCP,TTS}=OFF`).
- [ ] Emscripten link settings on the `basic256` target (WASM only):
      - Initial heap: heap growth is unsupported with pthreads, so size it
        up front (start at 512 MB; sprite-heavy fractal programs are the
        memory hogs).
      - `PTHREAD_POOL_SIZE`: count the threads we actually start
        (interpreter, sound internals, downloader) — start at 8. Qt's
        default is 4 and exceeding it requires returning to the event loop
        first, which a `RUN` click can't guarantee.
- [ ] Expect and fix in this phase (compile-time only):
      - Multimedia: Qt Multimedia has a WASM backend, but
        `setSourceDevice()` (the `sound:` in-memory path,
        `Sound.cpp:840`) is documented as unsupported in WASM. For v1,
        guard the in-memory `QMediaPlayer` path under
        `#ifdef Q_OS_WASM` → `ERROR_NOTAVAILABLE`; keep the `QAudioSink`
        tone path (BEEP/waveforms — the one fractal/demo programs use) and
        URL-based playback. Web-Audio bridge is Phase 7.
      - Printing already excluded by flag; grep for any stray
        QtPrintSupport include outside guards.
- [ ] Artifact: `basic256.html`, `basic256.js`, `basic256.wasm`,
      `qtloader` files uploaded as a CI artifact.

### Phase 4 gate
- [ ] WASM CI job links successfully and uploads artifacts.
- [ ] Local smoke test: serve the artifact with COOP/COEP (e.g. Qt's
      `emrun` or a 10-line python server sending the two headers), open in
      Chrome + Firefox, IDE appears, a Hello World `.kbs` typed into the
      editor runs and PRINTs.
- [ ] Desktop CI still green ×4 (the flags/ifdefs must not leak).

---

## PHASE 5 – Browser runtime adaptation

The app now loads; make it *usable*. All items guarded `#ifdef Q_OS_WASM`
unless noted.

- [ ] **Main-thread blocking audit (RULE 2).** Known sites to fix or verify:
      - `RunController.cpp:186-197` TTS wait loop — already dead in WASM
        (TTS flag off), but confirm it's inside the guard.
      - Nested-exec dialogs: `MainWindow.cpp:1473` (`msgBox.exec()`),
        `PreferencesWin.cpp:682`, `BasicEdit.cpp:260`. `exec()` on the WASM
        main thread cannot yield to the browser. Convert to `open()` +
        signal/slot result handling (do it unconditionally — it's the
        modern pattern and keeps one code path).
      - Grep the GUI layer for `->wait(`, `waitCond->wait` usage on the GUI
        side, and any `while(...)` polls with `processEvents()`.
- [ ] **File open/save.** The browser has no real filesystem; MEMFS is
      transient. Wrap program load/save: `QFileDialog::getOpenFileContent()`
      / `QFileDialog::saveFileContent()` under `Q_OS_WASM`, existing dialogs
      elsewhere. BASIC-program file I/O (`OPEN`/`WRITE`/`READ` on files)
      keeps working against MEMFS within a session — document that
      persistence is not guaranteed (IDBFS is Phase 7).
- [ ] **Examples in the browser.** Ship `Examples/` (or a curated subset —
      full dir may be large) as Qt resources or an emscripten
      `--preload-file` pack so File→Open Example works.
- [ ] **Settings.** `QSettings` on WASM is IndexedDB-backed and
      asynchronous; verify PreferencesWin behavior, tolerate
      first-run-empty settings.
- [ ] **Clipboard, fonts, HiDPI:** quick manual checks; Qt bundles a
      fallback font, clipboard needs the page served over HTTPS.
- [ ] **NETREAD/downloader:** `QNetworkAccessManager` maps to fetch() —
      works, but is subject to CORS. Note this in the browser README; don't
      try to fix CORS.

### Phase 5 gate
- [ ] In-browser: load an example, edit, run, stop, re-run; graphics
      example animates; BEEP audible; mouse/keyboard examples respond;
      save/download a `.kbs`, reload it via upload.
- [ ] A `.kbs` that calls `SYSTEM`/`DBOPEN`/`SAY` shows the
      ERROR_NOTAVAILABLE message in the text window and continues normally.
- [ ] Desktop CI green ×4.

---

## PHASE 6 – Hosting + deploy pipeline (GitHub Pages)

GitHub Pages cannot send the `Cross-Origin-Opener-Policy` /
`Cross-Origin-Embedder-Policy` headers that SharedArrayBuffer (i.e. the
multithreaded build) requires. The standard workaround is the
**coi-serviceworker** shim: a small MIT-licensed JS file included by the
page that re-serves it to itself with the headers injected (costs one
automatic reload on first visit).

- [ ] Add `coi-serviceworker.min.js` to the deployed page and a
      `<script>` include in the generated/created `basic256.html` (use a
      custom shell page rather than patching Qt's default at deploy time).
- [ ] New workflow step/job: on push to `main` (or tag), build WASM, place
      artifacts + shim + a small landing page into `gh-pages` via
      `actions/deploy-pages`.
- [ ] `.nojekyll` in the deploy root (Jekyll can mangle files/underscores).
- [ ] Size pass: release build, `-Oz`, consider `-flto`. GitHub Pages
      gzips text-ish types automatically but verify the served `.wasm` is
      compressed; if not, note it (Brotli pre-compression isn't possible on
      Pages — acceptable for v1).
- [ ] README: link the live page, list the v1 browser limitations (the six
      flags, in-memory sounds, no persistent files, CORS on NETREAD).
- [ ] **Fallback recorded:** if the serviceworker shim proves flaky on
      target browsers, Cloudflare Pages/Netlify support a `_headers` file
      with real COOP/COEP — the deploy job is host-agnostic, only the shim
      differs.

### Phase 6 gate
- [ ] Public URL loads on stock Chrome, Firefox, Edge, Safari (Safari is
      the usual straggler — test it explicitly); threads confirmed working
      (a program with `PAUSE`/input waits doesn't freeze the page).

---

## PHASE 7 – Post-v1 improvements (optional, unordered)

- [ ] In-memory sound playback via a Web Audio bridge
      (`emscripten::val` + `decodeAudioData`; KDAB has a worked example of
      exactly this pattern for Qt 6 WASM).
- [ ] `SAY` via the browser's Web Speech API behind `BASIC256_ENABLE_TTS`'s
      WASM variant.
- [ ] IDBFS mount for a persistent `/home/web_user` so saved programs
      survive reloads.
- [ ] A trimmed "player" build (graph window only, program preloaded from
      URL parameter) for embedding fractal/demo programs in web pages.
- [ ] Revisit binary size: dynamic linking / `qt-cmake` deploy options,
      strip unused Qt features.

---

## Per-file / per-area summary (tick when fully done)

- [ ] Tree restructure (`src/core`, `src/gui`, `src/app`) + CMake split
- [ ] CI scripts path fixes (13 scripts + `BASIC256.nsi`)
- [ ] `GraphicsBuffer` extraction; `BasicGraph` as view
- [ ] `editwin` extern removed (programTitle)
- [ ] `basicKeyboard` extern removed (constructor injection)
- [ ] Feature flags ×6 + `ERROR_NOTAVAILABLE` + dress-rehearsal build
- [ ] WASM CI job (emsdk 4.0.7 + Qt 6.11 wasm_multithread + host Qt)
- [ ] Heap / PTHREAD_POOL_SIZE link settings
- [ ] Sound WASM guards (`setSourceDevice` path)
- [ ] Dialog `exec()` → `open()` conversions
- [ ] WASM file open/save (`getOpenFileContent`/`saveFileContent`)
- [ ] Examples packaged for browser
- [ ] gh-pages deploy + coi-serviceworker + landing page
- [ ] README browser-limitations section

---

## Why this order

Phases 1–3 are pure desktop refactors with the full CI safety net and a
debugger; they convert "port to WASM" into "flip a toolchain". Phase 3's
flags-OFF desktop build surfaces 90% of feature-surface bugs where they're
cheap to fix. Only Phases 4–6 involve the browser at all, and by then the
only genuinely new variables are the toolchain, threading headers, and the
sandbox — each isolated in its own phase gate.

---

## Session log

*(newest last — same convention as QT6_MIGRATION_CHECKLIST.md)*

- 2026-07-07: Phase 0 — maintainer confirmed audio audible on all four
  desktop targets. Bumped `build_Windows.ps1` / `build_Linux_x86.sh` aqt
  pins from Qt 6.7.3 to 6.11.1; Windows arch corrected `win64_msvc2019_64`
  → `win64_msvc2022_64` (aqtinstall dropped the 2019 arch for Qt ≥ 6.8;
  `linux_gcc_64` unchanged). First CI push: Linux x86_64/ARM64/macOS green,
  Windows failed twice with the same aqtinstall/Qt-6.11-repo-layout bug
  (see above) — fixed by pinning aqtinstall to the upstream merge commit
  containing PR #1000 in `build_Windows.ps1`. Re-pushed; run 28862119754
  green on all four targets (build+package+TestSuite). **Phase 0 gate
  closed.** Next up: Phase 1 (source tree restructure).

- 2026-07-07: Phase 1 — mechanical restructure. `git mv`'d all root
  `.cpp`/`.h` into `src/core`, `src/gui`, `src/app` per the 1A plan exactly.
  Deleted one unused `#include <QtWidgets/QMessageBox>` found in
  `Interpreter.cpp` (dead code, not a real widget dependency — not carried
  into Phase 2). Split `CMakeLists.txt` into `basic256core` STATIC +
  `basic256` exe; corrected the plan's Qt-component-per-target shorthand
  after grepping actual usage (`PrintSupport` belongs on core, not app —
  see 1B). Fixed relative-include breakage in `LEX/basicParse.y`/`.l` and
  `resources/*.rc` (`../X.h` → `../src/core/X.h`) that the plan's 1C list
  didn't explicitly name. `.github/scripts/*`, `BASIC256.nsi`,
  `COMPILING_RaspberryPI.txt`, `README.md` needed no changes (verified, not
  assumed). `COMPILING.txt` got the Qt 6.11.1 version bump left over from
  Phase 0. No local Qt/CMake/flex/bison toolchain available to sanity-build
  before pushing — relying on CI as the real gate. Run 28864003223 green on
  all four targets, first try. **Phase 1 gate closed.** Next up: Phase 2
  (cut the interpreter's direct lines to the GUI).

- 2026-07-07: Phase 2 — extracted `GraphicsBuffer` (new `src/core/` plain
  class) out of `BasicGraph`: owns the pixel buffers, sprite state, and
  mouse/click state; `BasicGraph` is now a view holding a `GraphicsBuffer*`.
  Interpreter's constructor is now `Interpreter(QLocale*, GraphicsBuffer*,
  BasicKeyboard*)`, replacing the `graphwin`/`basicKeyboard` externs with
  constructor injection; every `graphwin->X` in `Interpreter.cpp` became
  `graphics->X`. `editwin->title` replaced with a new `Interpreter::
  programTitle` set by `RunController` before each run. Maintainer decided
  to skip the plan's "Main.cpp constructs a bare GraphicsBuffer for -s
  mode" bullet this session (see 2A note) — `-s` still builds the full
  hidden widget tree unchanged, only the interpreter's access pattern
  changed. 2C signal audit: all `BlockingQueuedConnection`/return-value-emit
  cases confirmed to block the interpreter thread only, never the GUI
  thread — no changes needed. Caught and fixed one real bug while
  converting `BasicGraph::mouseMoveEvent` (a tooltip line reading the
  old, now-removed widget-level `mouseX`/`mouseY`, which would not have
  compiled). Also caught a stray CRLF→LF line-ending flip from an earlier
  `sed` edit on `Interpreter.cpp` (its blob is pinned to CRLF, unlike most
  of the repo) that would have produced a whole-file noise diff — restored
  before committing. `grep -n "extern BasicGraph\|extern BasicEdit\|extern
  BasicKeyboard" src/core/` returns nothing, confirming the gate's grep
  check. CI push pending; manual verification (graphics/sprite/mouse-input
  examples, `-s` IMGSAVE comparison) needs the maintainer — no local Qt/
  display available this session, and `TestSuite/testsuite_ci.kbs`
  deliberately excludes sprite/graphics coverage for an unrelated,
  pre-existing `-s`-mode reason (see Phase 2 gate notes).
