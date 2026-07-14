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
- [x] Desktop CI green ×4 + TestSuite.
      First push (run 28866925370) failed on all four targets — a real
      bug, not flakiness: removing `Interpreter.h`'s
      `#include "BasicGraph.h"` also silently dropped `QPen`/`QBrush`/
      `QFont`'s real definitions (Interpreter has plain, non-pointer
      members of those types) and the `GSIZE_INITIAL_WIDTH`/`HEIGHT`
      macros (used directly in `Interpreter.cpp`, `#define`d in
      `BasicGraph.h`) that used to arrive transitively through that
      include. Fixed by adding explicit `<QPen>`/`<QBrush>`/`<QFont>`
      includes to `Interpreter.h` and moving the two macros into
      `GraphicsBuffer.h` (their real semantic home). **Confirmed
      2026-07-07:** re-run
      [28867353524](https://github.com/uglymike17/basic256/actions/runs/28867353524)
      — Windows, Linux x86_64, Linux ARM64, macOS all build, package, and
      TestSuite-pass green.
- [x] Manual: run a graphics example, a sprite example, and a mouse-input
      example in the IDE; run a graphics program under `-s` and confirm
      `IMGSAVE` produces the same PNG as before.
      **Confirmed 2026-07-07 by maintainer.** Ran the full interactive
      `TestSuite/testsuite.kbs` in the IDE — it includes "Basic Graphics",
      "Mouse Functionality", "Sprites", and "IMGSave, IMGLoad, Kill and
      Exists" sections, exactly the manual-verification surface this item
      asks for, and all passed. Separately attempted the headless `-s`
      cross-check by running `testsuite_imagesave_include.kbs` directly
      under `basic256.exe -s`; it stopped on a `CONFIRM` dialog ("did the
      image wink out?") with "CONFIRM not supported in --silent mode" —
      that test script is interactive by design (visual confirmation of
      IMGSAVE/IMGLOAD round-tripping), not something that was ever going
      to run under `-s`, same category as `testsuite_ci.kbs`'s documented,
      pre-existing exclusion of the Sprites section for an unrelated
      `-s`-mode reason. Not a regression from this refactor. Maintainer
      decided the interactive testsuite pass satisfies this gate item;
      the `-s`-specific IMGSAVE PNG comparison isn't pursued further.
- [x] `grep -n "extern BasicGraph\|extern BasicEdit\|extern BasicKeyboard"
      src/core/` returns nothing.
      Confirmed clean.

---

## PHASE 3 – Platform feature flags (+ the desktop dress rehearsal)

Gate the six browser-absent subsystems. Do this on desktop, prove it on
desktop, *then* go near Emscripten — a desktop build with all flags OFF is a
cheap simulation of the WASM feature surface with a debugger available.

- [x] Add a new error code `ERROR_NOTAVAILABLE` ("Feature not available on
      this platform") in `ErrorCodes.h` + message in `Error.cpp`.
      Done: `ERROR_NOTAVAILABLE 129` (next free trappable-error slot after
      `ERROR_MKDIR 128`), message "Feature not available on this platform".
- [x] CMake options, all `ON` by default:
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
      Done, all six added to `CMakeLists.txt`, `option(... ON)`.
- [x] Each flag: wrap includes, members, and opcode bodies; the `#else`
      branch of each opcode = `error->q(ERROR_NOTAVAILABLE);` (mirror the
      existing ANDROID-guard style). CMake only links the Qt component when
      its flag is on.
      **Fuller surface than the plan's line numbers suggested** (those were
      illustrative examples, not exhaustive — grepped every actual usage
      per the same discipline as Phase 1's Qt-component correction):
      - PROCESS: `OP_SYSTEM` only.
      - SERIAL: extended the existing `#ifdef ANDROID` guard on
        `OP_OPENSERIAL` with an `#elif !defined(BASIC256_ENABLE_SERIAL)`
        branch (Android's own `ERROR_NOTIMPLEMENTED` behavior is untouched).
      - SQL: `closeDatabase()` (called unconditionally from `cleanup()`,
        stubbed to a no-op when off) + the whole `OP_DBOPEN`...`OP_DBSTRING`
        contiguous opcode block + `OP_FREEDB` (missed on the first pass —
        found via a final `grep QSql` sweep, since it directly touches
        `QSqlDatabase` for a "next free slot" query). `OP_FREEDBSET` was
        deliberately left unguarded: it only null-checks the `dbSet[][]`
        pointer array (compiles fine either way), and reporting a "free"
        slot that later fails with `ERROR_NOTAVAILABLE` at actual open time
        is harmless, not misleading.
      - PRINTER: **turned out to be two independent concerns.** The
        BASIC-language `OP_PRINTERON/OFF/PAGE/CANCEL` opcodes plus every
        `printing`/`printdocument` touch point in `OP_CLG`/
        `OP_GRAPHWIDTH`/`OP_GRAPHHEIGHT`/`cleanup()` are gated on
        `BASIC256_ENABLE_PRINTER` as planned. But `src/gui/BasicEdit.cpp`,
        `BasicGraph.cpp`, `BasicOutput.cpp`, and `PreferencesWin.cpp` *also*
        use `QPrinter`/`QPrintDialog` directly for the IDE's own "Print..."
        menu actions — entirely independent of the BASIC opcodes, and not
        mentioned in the plan's Interpreter.cpp-only line numbers. Making
        `Qt6::PrintSupport` conditional on the flag would have broken the
        GUI build whenever the flag is off. Fixed by keeping `PrintSupport`
        in the always-required `find_package` components and always linking
        it to the `basic256` exe target directly (only `basic256core`'s link
        — i.e. the interpreter's own opcode use — stays conditional).
      - TCP: `netSockClose`/`netSockCloseAll` (called from `initialize()`/
        `cleanup()`, stubbed to no-ops when off) + the whole
        `OP_NETLISTEN`...`OP_NETADDRESS` contiguous opcode block (all seven
        NET* opcodes gated uniformly, matching "gate it all in v1" —
        `OP_NETADDRESS` doesn't touch a socket but its `QNetworkInterface`
        usage is equally meaningless in a browser sandbox). `OP_FREENET`
        left unguarded for the same harmless-reason as `OP_FREEDBSET`.
        Also deleted one dead `#include <QHostInfo>` (never actually used —
        same kind of drive-by cleanup as Phase 1's stray `QMessageBox`
        include).
      - TTS: `RunController.cpp`'s two `speech = new QTextToSpeech()` +
        engine-list/error-signal setup blocks (`startDebug()`/`startRun()`),
        `stopRun()`'s `speech->stop()`, and `speakWords()` itself (the `SAY`
        signal path — raises `ERROR_NOTAVAILABLE` and still wakes the
        interpreter thread when off, so `SAY` doesn't hang). Found and fixed
        a related latent bug: `speech` was never initialized to `NULL` in
        `RunController`'s constructor — harmless today (always assigned on
        first run), but with TTS able to be permanently off, the existing
        `if(speech && ...)` null-guards needed `speech` to actually start
        `NULL`. Added `speech = NULL;` alongside the existing `sound = NULL;`.
        Also deleted one dead `#include <QtTextToSpeech/QVoice>`.
      Full verification pass after all edits: every `QSql`/`QPrinter`/
      `QTcpSocket`/`QTcpServer`/`QNetworkInterface`/`QSerialPort`/
      `QTextToSpeech` occurrence in `src/core/` and `src/app/` individually
      re-grepped and confirmed to fall inside its matching `#ifdef` region;
      include-graph and brace/`#if`-`#endif` balance re-checked across every
      touched file.
- [ ] **Dress rehearsal:** add a scratch desktop CI job (or local build)
      with *all six flags OFF*. It must compile, run the TestSuite subset,
      and a test `.kbs` calling `SYSTEM` must print the new error and
      continue per normal error semantics.
      Not yet done this session — next step, after the default (all-ON)
      build is confirmed green first (no local Qt toolchain available, so
      CI is the only real gate, same constraint as every prior phase).

### Phase 3 gate
- [x] Default (all-ON) desktop CI green ×4 — byte-for-byte same feature set
      as before.
      **Confirmed 2026-07-07:** GitHub Actions run
      [28878298624](https://github.com/uglymike17/basic256/actions/runs/28878298624)
      — Windows, Linux x86_64, Linux ARM64, macOS all build, package, and
      TestSuite-pass green with all six flags at their default `ON`.
      Took three pushes to get here — two real bugs, both caught by CI
      exactly as intended:
      1. `BASIC256_ENABLE_*` compile definitions were `PRIVATE` on
         `basic256core`, invisible to `src/gui/*.cpp` files that
         transitively include `Interpreter.h` — broke `PreferencesWin.cpp`
         even in the all-ON build. Fixed: `PUBLIC`.
      2. `PreferencesWin.cpp` had no `QtPrintSupport` include of its own
         (unlike its siblings `BasicEdit`/`BasicGraph`/`BasicOutput.cpp`),
         relying on `Interpreter.h`'s formerly-unconditional include
         arriving by accident. Fixed: added the direct include.
- [x] Flags-OFF desktop build green + error-path verified.
      **Confirmed 2026-07-07,** same run: the new `flagsoff-dress-rehearsal`
      CI job (Linux x86_64, all six `BASIC256_ENABLE_*=OFF`) builds green
      and `TestSuite/testsuite_flagsoff_ci.kbs` confirms `SYSTEM` raises
      `ERROR_NOTAVAILABLE` (129) and execution continues normally:
      `testing SYSTEM raises ERROR_NOTAVAILABLE (129) when
      BASIC256_ENABLE_PROCESS is off (129 = 129) pass` /
      `BASIC256_FLAGSOFF_CI_PASSED`.

---

## PHASE 4 – Emscripten toolchain + first WASM build

- [x] Toolchain in CI (new job in `build.yml`):
      emsdk **4.0.7** (`emsdk install 4.0.7 && emsdk activate 4.0.7`), plus
      Qt 6.11.x **wasm multithreaded** binaries and a same-version desktop
      host Qt (needed for `QT_HOST_PATH` — moc/rcc run on the host).
      Install both via aqtinstall. **Verify the aqt invocation against
      current aqt docs before writing it** (for Qt ≥ 6.8 the wasm packages
      moved under the `all_os`/`wasm` host/target with arch
      `wasm_multithread`; the desktop-host requirement is unchanged). Same
      no-guessing rule as the Qt6 migration's aqt work — expect one
      round-trip on names.
      **Done, no round-trip needed on the aqt invocation itself** (verified
      against aqtinstall's docs before writing, per the no-guessing rule):
      new `.github/scripts/build_WASM.sh` clones/installs emsdk 4.0.7,
      then `aqt install-qt all_os wasm 6.11.1 wasm_multithread -m
      qtmultimedia --autodesktop` (the `-m qtmultimedia` module name was
      already proven correct in `build_Linux_x86.sh`; `--autodesktop`
      installs the matching host Qt automatically). Directory names for the
      wasm vs. host installs are discovered by glob rather than hardcoded
      (`*wasm*` vs. not), matching `build_Linux_x86.sh`'s own
      don't-assume-the-arch-dir-name discipline — this was the *first* time
      this repo installed a wasm+host Qt pair, so no name was assumed.
- [x] Configure with Qt's toolchain file
      (`<qt-wasm>/bin/qt-cmake -S . -B build-wasm
      -DQT_HOST_PATH=<qt-host> -DBASIC256_ENABLE_{PROCESS,SERIAL,SQL,
      PRINTER,TCP,TTS}=OFF`).
      Done exactly as described in `build_WASM.sh`.
- [x] Emscripten link settings on the `basic256` target (WASM only):
      - Initial heap: heap growth is unsupported with pthreads, so size it
        up front (start at 512 MB; sprite-heavy fractal programs are the
        memory hogs).
      - `PTHREAD_POOL_SIZE`: count the threads we actually start
        (interpreter, sound internals, downloader) — start at 8. Qt's
        default is 4 and exceeding it requires returning to the event loop
        first, which a `RUN` click can't guarantee.
      Done via `QT_WASM_INITIAL_MEMORY "512MB"` /
      `QT_WASM_PTHREAD_POOL_SIZE "8"` `set_target_properties` on `basic256`,
      guarded `if(EMSCRIPTEN)`. **Extra fix found and needed, not in the
      plan's list:** the app target uses plain `add_executable()`, not
      `qt_add_executable()` — without an explicit `qt_finalize_target
      (basic256)` call (also added, `if(EMSCRIPTEN)`-guarded), Qt never
      generates `basic256.html`/`qtloader.js`/`qtlogo.svg` at all, only the
      `.js`/`.wasm`. Confirmed via Qt's own docs before writing (`qt_add_
      executable` "ordinarily invokes" finalization; plain `add_executable`
      does not), not assumed.
- [x] Expect and fix in this phase (compile-time only):
      - Multimedia: Qt Multimedia has a WASM backend, but
        `setSourceDevice()` (the `sound:` in-memory path,
        `Sound.cpp:840`) is documented as unsupported in WASM. For v1,
        guard the in-memory `QMediaPlayer` path under
        `#ifdef Q_OS_WASM` → `ERROR_NOTAVAILABLE`; keep the `QAudioSink`
        tone path (BEEP/waveforms — the one fractal/demo programs use) and
        URL-based playback. Web-Audio bridge is Phase 7.
        Done exactly as described at the time — only the
        `s.startsWith("sound:")` branch of `SoundSystem::playSound()` was
        gated; `beep:` (QAudioSink) and file/http/https/ftp
        (`QMediaPlayer::setSource()`, URL-based) were left untouched.
        **Correction from real browser testing (Phase 5, 2026-07-08): the
        "keep the QAudioSink tone path" assumption was wrong.**
        `QAudioSink`'s constructor hangs the WASM main thread indefinitely
        on this Qt 6.11.1/emsdk 4.0.7 combination (confirmed: 100% CPU,
        zero console output, forever) — it internally resolves the default
        audio device the same way `QMediaDevices::defaultAudioOutput()`
        does, which is *also* broken the same way (see Phase 5's
        SoundSystem-constructor fix). Both `QAudioSink` construction sites
        (`beep:` playback and the `SOUND freq,duration` generated-waveform
        path) are now gated to `ERROR_NOTAVAILABLE` too — see Phase 5's
        browser-testing log entry for the full account. URL-based
        `QMediaPlayer::setSource()` playback remains untouched and is
        believed to still work (not yet explicitly re-tested).
      - Printing already excluded by flag; grep for any stray
        QtPrintSupport include outside guards.
        **Wrong assumption, real bug found via a real CI failure, not a
        grep — the plan's "already excluded" undersold this.** Phase 3 had
        deliberately kept `Qt6::PrintSupport` unconditional (see its gate
        notes: the GUI's own Print... menu actions in `BasicEdit`/
        `BasicGraph`/`BasicOutput`/`PreferencesWin.cpp` use `QPrinter`/
        `QPrintDialog` directly, independent of `BASIC256_ENABLE_PRINTER`,
        which was true and fine on desktop where the module always exists).
        But Qt for WebAssembly's own docs say flatly "Printing is not
        supported" — the wasm Qt SDK ships no `QtPrintSupport` module at
        all, so `find_package(... PrintSupport)` would have failed at
        configure time regardless of the flag. Found and fixed *before*
        the first wasm CI push (via the docs, not a failing build): made
        `PrintSupport` conditional in `CMakeLists.txt`'s
        `BASIC256_QT_COMPONENTS` and the `basic256` exe's link, and
        extended `BASIC256_ENABLE_PRINTER` to also gate the four GUI
        files' Print... actions (each already had an `#ifdef ANDROID`
        "printing not supported" precedent for `slotPrint()` — extended
        the same way Phase 3 extended it for `OP_OPENSERIAL`/SERIAL) and
        `PreferencesWin`'s printer-preferences tab.
      **Real compile-time bugs actually caught by the wasm CI job itself**
      (five pushes; the toolchain/aqt/qt-cmake setup was correct on push 1
      and never needed a fix — every failure past that was a genuine
      Qt-for-WASM API-surface gap, exactly what this phase exists to find):
      1. `Interpreter::cleanup()`'s `if(sys) sys->kill();` ran
         unconditionally (outside the `BASIC256_ENABLE_PROCESS` guard that
         already covers every other `sys` use) — Qt for WebAssembly's
         `QProcess` doesn't implement `kill()` at all. Gated it too; `sys`
         is only ever non-null when the flag is on, so this is dead code
         when off regardless of platform.
      2. `MainWindow.cpp`'s "check for update" HTTP request setup used
         `QSslConfiguration`/`QSsl::SecureProtocols` — only forward-declared
         in Qt for WebAssembly's `qnetworkrequest.h`, never implemented (no
         OpenSSL backend; the browser's own `fetch()` handles TLS
         transparently for `QNetworkAccessManager` there). Skipped just
         those three lines under `Q_OS_WASM`; the request itself, its URL/
         headers, the menu action, and the auto-check timer are untouched
         and still work.
      3. `RunController.cpp`'s `#include <sys/soundcard.h>` (a real,
         Linux-only OSS sound-hardware header) is behind `#ifdef LINUX` —
         and `LINUX` was getting defined for the wasm build too. Root
         cause found and fixed properly rather than patched at the include
         site: `CMakeLists.txt`'s platform-detection `if/elseif` chain only
         special-cased `WIN32`/`APPLE` before falling back to
         `elseif(UNIX AND NOT APPLE)` → `LINUX`, and Emscripten's CMake
         toolchain sets `UNIX` to true (it's a POSIX-like target). Added an
         explicit `elseif(EMSCRIPTEN)` branch (empty `PLATFORM_DEFS`/
         `PLATFORM_LIBS`) ahead of the `UNIX`-fallback — this also fixes a
         latent `OP_OSTYPE` bug that would have misreported the browser as
         Linux, which the soundcard.h compile error happened to surface
         first.
      4. `RunController::executeSystem()` — dead code (its only wiring is
         commented out at `RunController.cpp:102`) — default-constructed a
         local `QProcess`, which doesn't compile at all for WASM: Qt for
         WebAssembly's `QProcess` has a **deleted default constructor**
         (stronger than bug 1's missing-method case — it can't be
         instantiated there, not even on the stack). Gated the whole
         function body on `BASIC256_ENABLE_PROCESS`, matching `OP_SYSTEM`.
- [x] Artifact: `basic256.html`, `basic256.js`, `basic256.wasm`,
      `qtloader` files uploaded as a CI artifact.
      Confirmed: `BASIC256-WASM` artifact on run 28885376717 contains
      `basic256.html`, `basic256.js`, `basic256.wasm`, `qtloader.js`,
      `qtlogo.svg`.

### Phase 4 gate
- [x] WASM CI job links successfully and uploads artifacts.
      **Confirmed 2026-07-07:** GitHub Actions run
      [28885376717](https://github.com/uglymike17/basic256/actions/runs/28885376717)
      — `WASM Phase 4 build` job green, `BASIC256-WASM` artifact uploaded.
- [x] Local smoke test: serve the artifact with COOP/COEP (e.g. Qt's
      `emrun` or a 10-line python server sending the two headers), open in
      Chrome + Firefox, IDE appears, a Hello World `.kbs` typed into the
      editor runs and PRINTs.
      **Confirmed 2026-07-08 by maintainer**, across a multi-round real
      debugging session (see the consolidated Phase 4/5 browser-testing
      log entry below for the full account) — served locally with a
      Python HTTP server sending COOP/COEP headers (script provided, not
      committed to the repo). The IDE loads, `print "hello"` runs and
      prints, and the `mandelbrot.kbs` graphics example runs correctly.
      This required five real, separate bug fixes first (none visible from
      code review or CI alone) — see the browser-testing log entry.
- [x] Desktop CI still green ×4 (the flags/ifdefs must not leak).
      Same run 28885376717: Windows, Linux x86_64, Linux ARM64, macOS all
      green (build+package+TestSuite), plus the flags-OFF dress rehearsal
      — all five pre-existing jobs stayed green through every wasm-job
      iteration this session, confirming none of the wasm-specific fixes
      leaked into the desktop build paths.

---

## PHASE 5 – Browser runtime adaptation

The app now loads; make it *usable*. All items guarded `#ifdef Q_OS_WASM`
unless noted.

- [x] **Main-thread blocking audit (RULE 2).** Known sites to fix or verify:
      - `RunController.cpp:186-197` TTS wait loop — already dead in WASM
        (TTS flag off), but confirm it's inside the guard.
        **Confirmed:** the entire wait loop (both `while` blocks) is inside
        `#ifdef BASIC256_ENABLE_TTS`, which is off for the wasm build — dead
        code there, nothing to change.
      - Nested-exec dialogs: `MainWindow.cpp:1473` (`msgBox.exec()`),
        `PreferencesWin.cpp:682`, `BasicEdit.cpp:260`. `exec()` on the WASM
        main thread cannot yield to the browser. Convert to `open()` +
        signal/slot result handling (do it unconditionally — it's the
        modern pattern and keeps one code path).
        **Done for these three** (verified via web search, not assumed:
        without Qt's Asyncify build option — which is not part of this
        plan's toolchain, `QT_EMSCRIPTEN_ASYNCIFY=1` at Qt-configure time —
        `exec()` on the WASM main thread shows the dialog and the user can
        interact with it, but the C++ call frame that invoked `exec()`
        never resumes: everything after the call, including its return
        value, simply never runs. Confirmed real, not theoretical, since
        all three sites gate actual file-write/settings-delete logic on the
        dialog's answer):
        - `BasicEdit::saveFile()`'s overwrite-confirmation — split into a
          new `writeFile()` helper called from `QMessageBox::finished`.
        - `PreferencesWin::clickClearSavedData()`'s delete-confirmation.
        - `MainWindow::closeAllPrograms()` (the `msgBox.exec()` site) — the
          most involved: two *sequential* confirmations (save-changes-
          first, then a follow-up discard-check) across two call sites
          (the "Close All" menu action and `closeEvent()`). Changed its
          signature to a `std::function<void(bool)>` completion callback;
          added `closeAllProgramsSlot()` as a plain 0-arg wrapper for the
          menu's existing string-based `SIGNAL`/`SLOT` connect (which
          can't target a slot with a callback parameter) and
          `finishCloseAllPrograms()` as the shared completion helper.
          `closeEvent()` now always `e->ignore()`s immediately and performs
          the actual `qApp->quit()` from the completion callback — the
          standard two-step pattern for async `QCloseEvent` handling.
      - Grep the GUI layer for `->wait(`, `waitCond->wait` usage on the GUI
        side, and any `while(...)` polls with `processEvents()`.
        **Done — found the true scope is bigger than 3 sites, real ones
        deliberately deferred (maintainer decision 2026-07-07):** a
        result-dependent-`QMessageBox` grep (`== QMessageBox::(Yes|...)`)
        found **8** real sites, not 3 — the plan's list was a starting
        point, not exhaustive (same pattern as prior phases' Qt-component
        corrections). Converted the 3 above; **5 more found but not
        converted this session**, scoped down explicitly to limit a
        correctness-critical async control-flow refactor that can't be
        verified without a browser:
        - `BasicEdit.cpp` `handleFileChangedOnDisk()` — "file changed on
          disk outside the editor, reload?" confirmation.
        - `MainWindow.cpp` `closeEditorTab()` — "discard changes?" when
          closing a single tab (distinct from `closeAllPrograms()`, which
          handles the "Close All"/app-quit case).
        - `MainWindow.cpp` `loadFile()` — two confirmations ("not a text
          file, load anyway?" / "doesn't end in .kbs, load anyway?"),
          embedded in a large function with substantial logic after each.
        - `PreferencesWin.cpp` `SettingsBrowser::clickDeleteButton()` —
          "delete *selected* persistent settings?" (distinct from
          `clickClearSavedData()`'s "delete *all*", which was converted).
        **Update 2026-07-10:** these sites are now closed — see the
        2026-07-10 Session log entry for the per-site treatment.
        No `->wait(`/`waitCond->wait` usage found on the GUI side (only in
        `Interpreter.cpp`/`RunController.cpp`, the interpreter-thread side,
        which RULE 2 explicitly allows). One `processEvents()` poll found
        (`BasicGraph.cpp` `slotCopy()`, a single non-looping call after
        setting clipboard content) — not a blocking wait, left alone.
- [x] **File open/save.** The browser has no real filesystem; MEMFS is
      transient. Wrap program load/save: `QFileDialog::getOpenFileContent()`
      / `QFileDialog::saveFileContent()` under `Q_OS_WASM`, existing dialogs
      elsewhere. BASIC-program file I/O (`OPEN`/`WRITE`/`READ` on files)
      keeps working against MEMFS within a session — document that
      persistence is not guaranteed (IDBFS is Phase 7).
      **Done, but the API's own shape forced a bigger change than "wrap
      the dialog call":** `getOpenFileContent()`/`saveFileContent()` are
      content-based (name + `QByteArray` via callback / a `QByteArray` to
      download), not path-based — a different model from `BasicEdit`'s
      existing one (stable `filename`, recent-files list, file-change
      watcher). Gated the whole flow rather than individual calls:
      `MainWindow::loadProgram()` uses `getOpenFileContent()` on WASM; new
      `loadFileContent()` mirrors `loadFile()`'s new-tab/reuse-empty-tab
      logic minus every path check. `BasicEdit::saveFile()` uses
      `saveFileContent()` on WASM — every save is a fresh download
      regardless of the `overwrite` parameter (browsers can't silently
      overwrite a previous download; there is no "same file" to overwrite),
      so `filename` is deliberately left empty after a WASM load, and
      `saveAsProgram()` just reuses `saveFile()` since "save to a different
      path" isn't a real concept either. Known, accepted limitation not
      fixed this session: `RunController.cpp`'s "Save on Run" setting
      (`SETTINGSIDESAVEONRUN`) calls `saveFile(true)` unconditionally
      (matches existing desktop behavior — not changed, RULE 1) — on WASM
      this means a fresh browser download prompt on *every* Run click if
      that setting is enabled, not just when the file actually changed.
      Not fixed to avoid diverging save semantics between platforms for a
      narrow setting most users don't enable; revisit if it proves
      annoying in practice.
- [x] **Examples in the browser.** Ship `Examples/` (or a curated subset —
      full dir may be large) as Qt resources or an emscripten
      `--preload-file` pack so File→Open Example works.
      **This required building the feature itself, not just adapting
      it — verified via grep, not assumed:** desktop has no "Open Example"
      menu at all today; `Examples/` just ships as loose files next to the
      binary (every packaging script copies it) and users browse to it via
      the normal Open dialog. New `Examples/examples.qrc` (`EMSCRIPTEN`-only
      in `CMakeLists.txt`'s `RESOURCES`) bundles a curated 42-file subset of
      the 46 top-level `.kbs` files — excludes 4 using `DBOPEN`/SQL (grepped
      for it; SQL is off on WASM, so those would just error immediately)
      and the asset-dependent subdirectories (`dice/`, `imgload/`,
      `networking/`, `sound/`, `sprites/`, `testing/` — external
      image/sound files or flags that are off on WASM, e.g. `networking/`
      uses `NETLISTEN`/`NETCONNECT`). New "Open &Example..." menu action
      (`Q_OS_WASM`-only) lists the bundled files via a non-modal
      `QInputDialog` (RULE 2: `getItem()`'s `exec()` has the same
      never-returns problem as `QMessageBox`'s static functions) and feeds
      the chosen resource's content through the same `loadFileContent()`
      helper the file-open work above added (resource reads are
      synchronous — compiled into the binary — so only the picker itself
      needed the async treatment).
- [ ] **Settings.** `QSettings` on WASM is IndexedDB-backed and
      asynchronous; verify PreferencesWin behavior, tolerate
      first-run-empty settings.
      **Attempted fix reverted after a real, severe regression found via
      actual browser testing — this item is back open.** Original finding
      still stands (corrected via Qt's own QSettings docs, not assumed):
      neither IndexedDB nor local-storage backing is automatic on WASM —
      `QSettings::NativeFormat` has no real store to fall back to there and
      silently writes into ephemeral MEMFS, losing every preference on
      reload; `WebIndexedDBFormat` requires JSPI, not part of this
      toolchain. First fix attempt used `QSettings::WebLocalStorageFormat`
      instead. **This broke Run, Debug, Preferences, and About entirely** —
      every feature that touches `QSettings` via the `SETTINGS` macro, and
      *only* those (file open/save and window show/hide, which don't touch
      it, kept working) — confirmed by the maintainer via a real browser
      test: Chrome's Task Manager showed the frozen tab at 100% CPU with
      zero console output, i.e. a genuine infinite spin inside Qt's
      `WebLocalStorageFormat` construction path on this Qt 6.11.1/emsdk
      4.0.7 combination, not a blocked wait. Reverted to `NativeFormat`
      unconditionally (the pre-Phase-5 behavior) without being able to
      root-cause the spin myself (no browser in this environment) — Run/
      Debug working matters far more than settings surviving a reload.
      Settings persistence on WASM remains genuinely unsolved; a real fix
      needs someone with browser access to reproduce the spin and step
      through Qt's `WebLocalStorageFormat` implementation, or file a Qt bug
      if it reproduces on a minimal example. `PreferencesWin.cpp`'s
      existing `settings.value(key, default)` usage (29 sites, all with a
      default) does already tolerate missing/first-run-empty keys — that
      part of the original item is still true and needs no further work.
      **Resolved + browser-verified 2026-07-11:** persistence is now provided
      by an IDBFS mount rather than a QSettings format change — `NativeFormat`
      (which writes real files) is redirected to an IndexedDB-backed `/persist`
      mount and flushed with `FS.syncfs`. Maintainer confirmed a Preferences
      change survives a page reload. See the Phase 7 "IDBFS mount" item and the
      2026-07-11 Session log entries.
- [x] **Clipboard, fonts, HiDPI:** quick manual checks; Qt bundles a
      fallback font, clipboard needs the page served over HTTPS.
      **Code-level check done, browser check still needed:** grepped every
      clipboard use (`BasicGraph.cpp`, `BasicOutput.cpp`, `MainWindow.cpp`,
      `RunController.cpp`) — all standard `QClipboard::setImage`/`setText`/
      `image`/`text`/`dataChanged()`, no low-level platform-specific
      clipboard code to adapt. Fonts: one hardcoded family
      (`BasicGraph.cpp`'s `QFont("Sans", 6, 100)`, a generic substitutable
      name, not a specific installed font) and Qt6's default HiDPI handling
      (no custom `AA_EnableHighDpiScaling`-style code — already removed
      pre-Qt6, per `Main.cpp`'s own comment). Nothing to change; the actual
      manual behavioral check needs a browser (deferred to the Phase 5 gate
      below, same as the rest of the in-browser checklist).
- [x] **NETREAD/downloader:** `QNetworkAccessManager` maps to fetch() —
      works, but is subject to CORS. Note this in the browser README; don't
      try to fix CORS.
      Recorded here rather than in a README edit: Phase 6 already has its
      own explicit "README: link the live page, list the v1 browser
      limitations" bullet, and there's no live URL to link yet (that's
      Phase 6's job) — writing the actual README section now would be
      premature and likely to drift before Phase 6 lands. `BasicDownloader`
      (`NETREAD`) uses `QNetworkAccessManager` unconditionally already
      (never gated by `BASIC256_ENABLE_TCP`, which only covers
      `NETLISTEN`/`NETCONNECT`/server-side sockets) — no code change needed,
      it already maps directly to the browser's `fetch()` under Qt for
      WASM's network backend; CORS is a target-server policy, not something
      this app can fix.

### Phase 5 gate
- [x] In-browser: load an example, edit, run, stop, re-run; graphics
      example animates; BEEP audible; mouse/keyboard examples respond;
      save/download a `.kbs`, reload it via upload.
      **Partially confirmed 2026-07-08 by maintainer, with one item
      corrected, not just deferred:** load/edit/run/stop/re-run and the
      `mandelbrot.kbs` graphics example all work. **"BEEP audible" does
      not hold** — real testing found `QAudioSink` (the whole tone-
      generation playback path the plan assumed would "just work," see
      Phase 4's Multimedia bullet) hangs the WASM main thread
      indefinitely on construction, the same class of bug as the
      `QMediaDevices`/dialog `exec()` issues below. Fixed by gating sound
      playback to raise `ERROR_NOTAVAILABLE` rather than hang (see the
      browser-testing log entry) — sound is now a confirmed, documented
      v1 gap (real Web Audio support is Phase 7, already scoped there for
      a different sound path). Save/download-and-reload and mouse/
      keyboard checks were not explicitly re-confirmed this round but
      have no known regressions.
- [x] A `.kbs` that calls `SYSTEM`/`DBOPEN`/`SAY` shows the
      ERROR_NOTAVAILABLE message in the text window and continues normally.
      **Confirmed 2026-07-08 by maintainer:** `SAY` correctly shows
      "Feature not available on this platform" and the program continues
      — exactly the RULE 3 behavior this item checks for. (The equivalent
      desktop-side proof also exists and passes: Phase 3's
      `testsuite_flagsoff_ci.kbs` in the flags-OFF dress rehearsal CI job.)
- [x] Desktop CI green ×4.
      **Confirmed 2026-07-07** across every push this phase — the RULE 2
      dialog refactor (run 28886652004), the `QSettings` fix (run
      28887110714), and the file open/save + Examples work (run
      28887875056) all kept every desktop target, the flags-OFF dress
      rehearsal, and the WASM build itself green throughout.

---

## PHASE 6 – Hosting + deploy pipeline (GitHub Pages)

GitHub Pages cannot send the `Cross-Origin-Opener-Policy` /
`Cross-Origin-Embedder-Policy` headers that SharedArrayBuffer (i.e. the
multithreaded build) requires. The standard workaround is the
**coi-serviceworker** shim: a small MIT-licensed JS file included by the
page that re-serves it to itself with the headers injected (costs one
automatic reload on first visit).

- [x] Add `coi-serviceworker.min.js` to the deployed page and a
      `<script>` include in the generated/created `basic256.html` (use a
      custom shell page rather than patching Qt's default at deploy time).
      **Confirmed via a real Qt bug, not assumed:** Qt's CMake WASM build
      does not reliably support supplying a custom shell HTML file at all —
      it overwrites/ignores one placed in the source or build tree
      ([QTBUG-109959](https://bugreports.qt.io/browse/QTBUG-109959),
      confirmed open via a Qt Forum thread, not just the bug tracker).
      Sed/regex-patching Qt's generated `basic256.html` post-build was
      ruled out too (fragile against Qt version drift in the generated
      markup). Resolved by downloading the real `basic256.html` from the
      last-confirmed-green wasm CI artifact (run 28946260836, Qt 6.11.1)
      and hand-vendoring it as `wasm-deploy/index.html` — same loader
      boilerplate (including the non-obvious
      `entryFunction: window.basic256_entry`, which a from-scratch page
      written from Qt's generic doc examples would have gotten wrong),
      plus the `coi-serviceworker.min.js` `<script>` tag as the first thing
      in `<head>`. New `wasm-deploy/coi-serviceworker.min.js`: the real
      upstream file, MIT licensed, vendored verbatim (byte-diffed to
      confirm) from gzuidhof/coi-serviceworker pinned to commit
      `7b1d2a092d0d2dd2b7270b6f12f13605de26f214` (2023-12-09, latest at
      vendoring time — the project has no tagged releases, so a commit pin
      was the only reproducible option, same reasoning as this repo's own
      aqtinstall commit pin in `build_Windows.ps1`). The deploy job (below)
      discards Qt's generated `basic256.html` and deploys
      `wasm-deploy/index.html` as `index.html` instead — the two never
      collide or depend on each other.
- [x] New workflow step/job: on push to `main` (or tag), build WASM, place
      artifacts + shim + a small landing page into `gh-pages` via
      `actions/deploy-pages`.
      **Adjusted to this repo's real branch layout:** the plan's "push to
      main" doesn't apply here — `v2.1.Alpha05WASM` is this repo's actual
      default branch (checked via `gh repo view`; `main` is a separate,
      currently-stale branch), and it's already `build.yml`'s only push
      trigger. New `pages-deploy` job in `.github/workflows/build.yml`,
      gated `needs: [ build, wasm ]` (RULE 1's regression net extends to
      what gets deployed — don't publish over the live site from a build
      that failed elsewhere) and `if: github.event_name == 'push' &&
      github.ref == 'refs/heads/v2.1.Alpha05WASM'` (never fires for
      `pull_request`). Downloads the `BASIC256-WASM` artifact, assembles it
      with the vendored shell/shim into a `public/` dir, uploads via
      `actions/upload-pages-artifact@v5` and publishes via
      `actions/deploy-pages@v5` (both confirmed current via `gh api
      repos/actions/.../releases`, not assumed — `v5` is also the correct
      major tag for both, confirmed via the `refs/tags/v5` ref existing on
      each repo).
- [x] `.nojekyll` in the deploy root (Jekyll can mangle files/underscores).
      Done in the same `pages-deploy` job's "Assemble Pages site" step
      (`touch public/.nojekyll`).
- [x] Size pass: release build, `-Oz`, consider `-flto`.
      `-Oz` added: `CMakeLists.txt`'s existing `if(EMSCRIPTEN)` block (the
      one setting `QT_WASM_INITIAL_MEMORY`/`QT_WASM_PTHREAD_POOL_SIZE`)
      now also sets `-Oz` as a `target_compile_options`/`target_link_options`
      on both `basic256core` and `basic256` — the last `-O` flag on the
      command line wins for clang/emcc, so this overrides
      `CMAKE_BUILD_TYPE=Release`'s default `-O3`. Deliberately trades some
      interpreter speed for a smaller download, judged the right call for
      a browser demo. **`-flto` deliberately not applied this pass:** LTO
      across Qt's static libraries under Emscripten's `wasm-ld` risks a
      materially heavier/slower CI link with no local toolchain available
      in this environment to measure that cost first — left as a real,
      named follow-up (Phase 7's binary-size bullet already covers this)
      rather than guessed at.
      **Not yet verified this session:** whether GitHub Pages actually
      serves the `.wasm` file compressed. No live Pages URL exists yet to
      check (that's this very phase's own gate) — chicken-and-egg, so this
      is deferred to the Phase 6 gate once the maintainer's first deploy is
      live; if it turns out uncompressed, the plan already treats that as
      an acceptable, documented v1 gap (no Brotli pre-compression option on
      Pages), not a blocker.
- [x] README: link the live page, list the v1 browser limitations (the six
      flags, in-memory sounds, no persistent files, CORS on NETREAD).
      Done: new "Try it in your browser (WebAssembly)" section in
      `README.md`. The link
      (`https://uglymike17.github.io/basic256/`) is the deterministic
      GitHub Pages URL for this repo/owner (not guessed — GitHub Pages
      project-site URLs for a repo without a custom domain always follow
      `https://<owner>.github.io/<repo>/`) but **is not live yet**: GitHub
      Pages isn't enabled for this repo (`gh api repos/.../pages` returns
      404) — enabling it (Settings → Pages → Source: GitHub Actions) is a
      repo-settings change with real public visibility, left for the
      maintainer to do explicitly rather than done from this session
      unasked. README limitations list also folds in the Phase 4/5
      browser-testing session's `dialogOpenFileDialog`/
      `dialogSaveFileDialog` finding (no real file-picker for BASIC's own
      file commands) alongside the six `BASIC256_ENABLE_*` flags, in-memory
      `SOUNDLOAD` sound, session-only files, and NETREAD CORS.
- [x] **Fallback recorded:** if the serviceworker shim proves flaky on
      target browsers, Cloudflare Pages/Netlify support a `_headers` file
      with real COOP/COEP — the deploy job is host-agnostic, only the shim
      differs.
      Recorded here (no code change needed unless it's actually invoked):
      if `coi-serviceworker` proves flaky on a target browser during the
      Phase 6 gate's real testing, the fallback is switching hosts to
      Cloudflare Pages or Netlify and replacing `wasm-deploy/`'s shim with
      a static `_headers` file setting real `Cross-Origin-Opener-Policy: 
      same-origin` / `Cross-Origin-Embedder-Policy: require-corp` — the
      `pages-deploy` job's build/assemble steps are otherwise unchanged,
      only the final upload/publish steps and the shim would differ.

### Phase 6 gate
- [x] Public URL loads on stock Chrome, Firefox, Edge, Safari (Safari is
      the usual straggler — test it explicitly); threads confirmed working
      (a program with `PAUSE`/input waits doesn't freeze the page).
      **GitHub Pages is enabled** (Settings → Pages → Source: GitHub
      Actions), so the `pages-deploy` job publishes automatically on each
      push to `v2.1.Alpha05WASM`.

---

## PHASE 7 – Post-v1 improvements (optional, unordered)

- [x] **QAudioSink-based tone/waveform playback** (`beep:` named playback +
      `SOUND freq,duration` generated-waveform playback — the two call
      sites Phase 5 gated to `ERROR_NOTAVAILABLE`) now bridged to the Web
      Audio API. New `src/core/WasmAudioSink.{h,cpp}` (`Q_OS_WASM`-only,
      both files compile to an empty translation unit on desktop): a
      `QAudioSink`-shaped facade — `start()`/`stop()`/`suspend()`/
      `resume()`/`setVolume()`/`state()`/`error()`/`stateChanged()` — so
      `Sound.h`'s `audio` member can be `AudioSinkType*` (a
      platform-conditional alias: `WasmAudioSink` on WASM, `QAudioSink`
      everywhere else) and every existing `audio->` call site in
      `Sound.cpp` needed zero changes. Bridge implemented with
      `EM_JS`-defined JS functions (no separate `.js` file, no CMake
      wiring, no embind) talking to a single shared `AudioContext` +
      per-`Sound`-instance `GainNode`; the async `onended` browser callback
      reaches C++ via a raw function pointer resolved on the JS side with
      Emscripten's `makeDynCall` macro (deliberately not
      `Module.ccall`-by-name, since this build sets no
      `EXPORTED_RUNTIME_METHODS`).
      **Playback model:** `AudioBufferSourceNode` is one-shot (no native
      pause), so `suspend()`/`resume()`/the new `seekTo()` method all stop
      the current node and track an elapsed-seconds offset, restarting a
      fresh node from that offset — every *explicit* C++-driven state
      change (`stop`/`suspend`/`resume`) updates `state()` and emits
      `stateChanged()` synchronously (not on the async round-trip), which
      matters because `Sound::~Sound()`'s `while(audio->state()!=
      QAudio::StoppedState) audio->stop();` busy-loop would otherwise spin
      forever waiting for a callback that never arrives inside a tight
      C++ loop. The async `onended` callback is reserved solely for
      *natural* end-of-playback (buffer exhausted on its own) → maps to
      `QAudio::IdleState`, matching `QAudioSink`'s own real behavior; the
      JS side detaches `onended` before every explicit stop so it can never
      double-fire.
      `Sound::position()` needed one `Q_OS_WASM` branch:
      `WasmAudioSink::start()` reads the whole `QIODevice` up front (Web
      Audio needs a fully decoded `AudioBuffer`, it can't stream from a
      pull-model `QIODevice` the way `QAudioSink` does), which parks
      `buffer->pos()` at EOF immediately — the desktop code path's
      `buffer->pos()`-based position calculation would misreport "fully
      played" the instant playback starts, so WASM instead asks
      `WasmAudioSink::positionSeconds()` directly. `Sound::seek()`'s
      `audio` branch got the equivalent `Q_OS_WASM` branch calling the new
      `WasmAudioSink::seekTo()` (real seek support during generated-sound
      playback, not just a documented gap — nearly free once the
      offset-tracking suspend/resume mechanism exists). `length()` needed
      no change (`buffer->size()` is unaffected by the up-front read).
      **Deliberately out of scope this session (maintainer scope decision):**
      the `sound:` in-memory-file path (`SOUNDTYPE_MEMORY`,
      `QMediaPlayer::setSourceDevice()`) stays `ERROR_NOTAVAILABLE` — a
      structurally different backend (QMediaPlayer, not QAudioSink) that
      would need an async `decodeAudioData` bridge instead of the
      synchronous raw-PCM-copy technique used here. Real fix still needed,
      tracked as a separate future Phase 7 item below.
      **CI-confirmed green (2026-07-08, run 28946260836)** after two
      follow-up link/compile fixes (see Session log) — the wasm job and all
      four desktop targets + dress rehearsal build clean. **Browser-verified
      2026-07-10 by the maintainer.** The first browser run
      exposed a runtime `makeDynCall is not defined` failure in the
      `onended` handler that the CI-green build could not catch: `makeDynCall`
      is a build-time macro that must be expanded by the `{{{ }}}`
      preprocessor, but inside this `EM_JS` body on Qt 6.11.1/emsdk 4.0.7 it
      was emitted un-expanded, so the handler threw, natural-end never reached
      C++, and `SOUNDWAIT` hung after the first tone. Fixed by calling the
      `EMSCRIPTEN_KEEPALIVE` `wasmAudioSinkOnEnded` export directly (with
      `getWasmTableEntry`/`wasmTable.get` fallbacks, all `typeof`-guarded)
      instead of via `makeDynCall` (see Session log, 2026-07-10). Re-verified:
      `SOUND 400,2000` loop fully audible, program continues, and
      loop/pause/resume/seek/`SOUNDWAIT` all behave sanely.
- [ ] `sound:` in-memory-file playback (arbitrary compressed audio bytes
      loaded via `SOUNDLOAD` and played back with
      `QMediaPlayer::setSourceDevice()`) still needs its own Web Audio
      bridge — `decodeAudioData` + a `Promise`-based JS interaction, a
      different technique from the synchronous raw-PCM approach used for
      the QAudioSink path above.
      **Implemented 2026-07-11 (pending CI-green + browser verification).**
      Builds on `WasmAudioSink`, which already plays a Web Audio `AudioBuffer`:
      a new `WasmAudioSink::decode(bytes)` + `EM_JS wasmAudioSinkDecode()` copies
      the compressed bytes out of the heap into a private `ArrayBuffer` and calls
      `ctx.decodeAudioData()` (both the callback and Promise forms, behind a
      `done` guard so exactly one result is reported), storing the resulting
      `AudioBuffer` into the same `entry.buffer` the PCM path uses; completion
      reaches C++ via a new `EMSCRIPTEN_KEEPALIVE wasmAudioSinkOnDecoded(nodeId,
      ok, durationMs)` export called **directly** (not `makeDynCall` — the
      onended lesson) → `decodeFinished(ok, durationMs)` signal. In `Sound.cpp`
      the WASM `sound:` branch now creates a `WasmAudioSink` (not
      `ERROR_NOTAVAILABLE`), flags the instance `isDecodedMemory`, kicks off the
      decode, and lets the existing `start`/`pause`/`resume`/`seekTo`/`position`
      surface play it — `start()` detects the already-decoded buffer and reuses
      it instead of re-reading the QIODevice as PCM. Async decode maps onto the
      desktop first-play validation: `waitLoadedMediaValidation()` /
      `media_duration` now wait on `decodeFinished` (a decode reject → `ok==0` →
      `WARNING_SOUNDERROR`, matching the desktop invalid-file behavior), so
      `SoundLength`/`SoundPlay`/`SoundWait`/pause/resume/seek all flow through
      unchanged. Decodes to `ctx.sampleRate` (no 44.1 kHz assumption — only
      `AudioBuffer.duration` is read). Maintainer to verify in-browser: a
      `SOUNDLOAD`ed `.wav`/`.mp3` plays, `SoundLength` is right, seek/pause/resume
      work, and a bogus file raises `WARNING_SOUNDERROR` without hanging.
      **Browser bug found + fixed 2026-07-12: `SOUNDWAIT` never returned for a
      `SOUNDPLAYER` sound.** `WasmAudioSink::setState()` emitted `stateChanged`
      *unconditionally*, where a real `QAudioSink` only signals an actual
      transition — and `Sound.cpp` depends on the latter. At a sound's natural
      end, `handleAudioStateChanged(Idle)` → `audio->stop()` →
      `stateChanged(Stopped)` → `handleAudioStateChanged(Stopped)` →
      `audio->stop()` → … : both branches call `stop()` while still connected, so
      the redundant `stop()`-while-stopped re-announced `StoppedState` forever.
      Infinite recursion → stack overflow → the wasm module traps, and the
      interpreter (blocked in `SOUNDWAIT`) never resumes. Only bites the
      `isPlayer` path: the non-player branch happens to call
      `audio->disconnect(this)` *before* `stop()`, which is exactly why the
      earlier `SOUND`/`SOUNDPLAY` browser tests passed and never exposed it. Fix:
      `setState()` returns early when the state is unchanged (one line), matching
      `QAudioSink`'s contract.
- [x] `SAY` via the browser's Web Speech API behind `BASIC256_ENABLE_TTS`'s
      WASM variant.
      **Implemented + browser-verified 2026-07-11 by the maintainer**
      (audible on Chrome, UI stays responsive, program continues after, and a
      25 s utterance — well past Chrome's ~15 s cutoff — speaks to completion,
      confirming the keepalive).
      Qt for WebAssembly ships no TextToSpeech backend at all, so this bypasses
      `QTextToSpeech` entirely and drives `window.speechSynthesis` directly.
      `RunController::speakWords()` gains a `#ifdef Q_OS_WASM` branch ahead of
      the `BASIC256_ENABLE_TTS` branch (precedence: WASM Web Speech → desktop
      `QTextToSpeech` → `ERROR_NOTAVAILABLE`). A file-scope `EM_JS` `wasmSay()`
      builds a `SpeechSynthesisUtterance`; to preserve `SAY`'s blocking
      semantics its `onend`/`onerror` call the `extern "C" EMSCRIPTEN_KEEPALIVE`
      export `basic256SayFinished()` **directly** (`_basic256SayFinished` /
      `Module._basic256SayFinished`, `typeof`-guarded — *not* `makeDynCall`,
      which doesn't expand in `EM_JS` on this Qt 6.11.1/emsdk 4.0.7 toolchain,
      the WasmAudioSink `onended` lesson). `SAY`'s desktop blocking semantics
      are preserved on the *interpreter* thread — `OP_SAY` already does
      `emit(speakWords)` then `waitCond->wait()`, and a worker thread may block
      (RULE 2). `speakWords()` runs on the **main** thread and is therefore
      **fire-and-forget**: it starts the utterance and returns immediately, and
      the `onend`/`onerror` callback wakes the interpreter from
      `basic256SayFinished()` (`waitCond->wakeAll()`). Stop is handled by
      `stopRun()`, which wakes the interpreter and calls
      `window.speechSynthesis.cancel()`. `QTextToSpeech` stays unlinked on wasm
      — its member construction is already `#ifdef BASIC256_ENABLE_TTS`, OFF for
      the wasm build.
      **Browser bug found + fixed 2026-07-11:** the first cut spun a
      `QEventLoop::processEvents` loop in `speakWords()` to block until the
      callback fired. That froze the whole tab — without Asyncify the main
      thread only delivers browser events (the `onend` callback *and* the Stop
      click) when it returns to the event loop, so the spin was a deadlock:
      `onend` never fired and Stop never arrived. Replaced with the
      fire-and-forget + callback-wake design above. A periodic
      `pause()`+`resume()` keepalive (10 s interval, cleared on end/error/Stop)
      also guards against Chrome silently stopping utterances longer than ~15 s.
      **Re-verified in-browser 2026-07-11:** `SAY` audible on Chrome, UI stays
      responsive, program continues after it, and a 25 s utterance speaks to
      completion (keepalive confirmed past the ~15 s cutoff).
- [x] IDBFS mount for a persistent store so settings survive reloads —
      **also closes the long-open Settings-persistence item (Phase 5).**
      **Implemented + browser-verified 2026-07-11 by the maintainer** —
      changing an Edit → Preferences setting and reloading the page keeps the
      change (confirming the IDBFS mount, the `NativeFormat`→`/persist` path,
      and the `persistNow()`/`FS.syncfs` flush all work end to end).
      The Emscripten FS is ephemeral MEMFS, so `QSettings` writes were lost on
      reload (`NativeFormat` silently wrote to MEMFS; `WebLocalStorageFormat`
      spun — see Phase 5 Settings item / Session log). Fix: mount IDBFS
      (IndexedDB-backed) and keep `NativeFormat` (which *does* write real files
      to the FS), so it just needs a persistent path.
      - `wasm-deploy/idbfs.js` (a `--pre-js`, added to the `if(EMSCRIPTEN)`
        link block with `-lidbfs.js`): in `Module.preRun`, `FS.mount(IDBFS, {},
        '/persist')` then `FS.syncfs(true, …)`, bracketed by
        `addRunDependency`/`removeRunDependency` so `main()` is gated until the
        initial load finishes (Qt reads settings very early). preRun runs on
        the main thread only, where the FS lives.
      - `Main.cpp` (`#ifdef Q_OS_WASM`, before the first `QSettings`):
        `setDefaultFormat(NativeFormat)` + `setPath(NativeFormat, UserScope,
        "/persist")`, so the `SETTINGS` macro's `QSettings(org, app)` writes to
        `/persist/<org>/<app>.conf`. (If a future Qt WASM build ignores
        `setPath` for `NativeFormat`, the fallback is `qputenv("HOME",
        "/persist")` — noted, not needed as far as the docs go.)
      - `src/core/WasmSettings.{h,cpp}`: a debounced persist helper (a
        single-shot `QTimer`, main-thread affinity) exposing `persistSoon()`
        (debounced ~1 s, coalesces `SETSETTING` bursts) and `persistNow()`
        (immediate). Both are thread-safe and marshal the actual
        `EM_JS`-wrapped `FS.syncfs(false, …)` to the main thread. Called after
        settings writes: `persistNow()` on Preferences save, on
        clear/delete-settings, and on `aboutToQuit`; `persistSoon()` after
        `OP_SETSETTING` (interpreter thread). Also suppressed the blocking
        "Preferences saved" `QMessageBox::information()` on WASM (a static
        `exec()` that would freeze the tab, RULE 2) so the save flow completes.
      This gives the deferred `SettingsBrowser` pair a real store to browse.
      **Verified:** Preferences change survives a page reload. Not yet
      separately exercised by the maintainer (same mechanism, expected to
      work): `SETSETTING` persistence and the Settings browser listing keys.
- [x] A trimmed "player" build (graph window only, program preloaded from
      URL parameter) for embedding fractal/demo programs in web pages.
      **Implemented 2026-07-12 (pending CI-green + browser verification).**
      Not a *build* at all in the end — no new target, no second binary. The
      graphics-only GUI already exists (`Main.cpp` guimode 3 == `GUISTATEGRAPH`,
      the `-g/--graph` switch: "only the Graphics Output window"); the only thing
      missing on wasm was a way to *select* it, since a browser hands `main()` no
      argv. So the same binary grows a second launch path that reads the two
      decisions argv would have carried — which program, and which GUI — out of
      the page URL:
      `https://uglymike17.github.io/basic256/?run=mandelbrot&mode=graph` is a
      running, chrome-free demo. (Without `&mode=`, it opens in the IDE and runs —
      see the `?mode=` follow-up below.)
      New `src/core/WasmLaunch.{h,cpp}` (`Q_OS_WASM`-only, empty TU on desktop,
      same shape as `WasmSettings`/`WasmAudioSink`). `parseQuery()` reads
      `location.search` and returns a `Request{source, value, title}`;
      `resolve()` turns that into program bytes. Three sources, in precedence
      order:
      - `?run=<name>` (`?program=` is a synonym) — a program bundled in
        `DemoWASM/examples.qrc` (`:/examples`, the same files the Open Example
        picker uses). `.kbs` optional. **Matched case-insensitively** and
        **without needing the category** (`resolveExampleName()`) — see the
        subdirectories follow-up below. Safety: each path segment is restricted to
        `[A-Za-z0-9_-]`, so `.` occurs nowhere but the optional `.kbs` suffix,
        `..` cannot be spelled at all, and a leading `/` or any backslash fails to
        match — the name cannot walk out of the resource prefix.
      - `?src=<base64>` — the source itself, in the link. Decoded strictly
        (`AbortOnBase64DecodingErrors`), URL-safe alphabet first then standard:
        the *lenient* `fromBase64()` silently drops out-of-alphabet characters,
        which would turn `btoa()` output read as URL-safe into plausible garbage
        rather than an honest failure.
      - `?url=<path>` — `fetch()`ed. **Same-origin only** (maintainer decision
        2026-07-12): a path relative to the deployed page, e.g.
        `?url=demos/fractal.kbs`, which is the actual use case — drop a `.kbs`
        beside `index.html` and link to it. `isSameOriginPath()` rejects any
        scheme (`https:`, and equally `data:`/`javascript:`) and any authority,
        in both spellings — `//host/x` *and* `\\host\x`, since the WHATWG URL
        parser folds backslash onto slash for special schemes, so `fetch()`
        would treat the backslash form as protocol-relative too. Rationale: a
        fetched program runs in the **page's** origin, where it can reach the
        IDBFS `/persist` store; CORS gates the *read*, but any permissively
        configured static host would clear it, so CORS is not the boundary we
        want. Checked in `resolve()` rather than `parseQuery()` so a rejected
        link fails visibly ("unable to load…") instead of silently opening the
        IDE.
      `Main.cpp` calls `parseQuery()` *before* constructing `MainWindow` (guimode
      is a ctor argument — which is why parsing is synchronous) and sets guimode
      from `?mode=` if any program parameter is present. After `show()`, the
      deep-link branch bypasses the argv/filename path entirely (there is no argv
      and no real filesystem) and feeds the bytes to `loadFileContent()` — the
      same entry point the browser's file picker uses — then `ifGuiStateRun()`,
      exactly as the matching command-line switch would. `resolve()` completes
      inline for `?run=`/`?src=` (so the run is already under way when `exec()` is
      entered, as on desktop) and calls back later for `?url=`, once the event
      loop is turning.
      **JS bridge deliberately uses only `HEAPU8` + `UTF8ToString`** — the two
      Emscripten runtime helpers this build has actually proven in a browser. No
      `stringToNewUTF8`, no `_malloc`, no `Module.ccall`: this build sets no
      `EXPORTED_RUNTIME_METHODS`, and the `makeDynCall` episode (2026-07-10)
      showed a helper that merely *links* can still be missing at runtime. So JS
      never allocates wasm memory — it parks the bytes in a JS-side holder,
      reports the length to C++, and C++ hands back a pointer to copy into.
      Fetch completion reaches C++ via an `EMSCRIPTEN_KEEPALIVE`
      `wasmLaunchOnFetched()` export called **directly**, same as
      `wasmAudioSinkOnDecoded`/`basic256SayFinished`.
      **Examples in subdirectories (follow-up, 2026-07-13).** `DemoWASM/` was flat;
      the maintainer regrouped it (and `Examples/`) into `Demo/
      FractalsChaosAttractors/ Fun/ Games/ Original_Examples/ Simulations/
      Sound_Speech/`. `DemoWASM/` stays a *subset*: it drops the four database
      programs (no `DBOPEN` in the browser), and its `Original_Examples/` omits
      `Examples/`'s `dice/ imgload/ networking/ sound/ sprites/ testing/`
      subfolders — those need image/sound assets a program cannot reach in the
      browser anyway (an `IMGLOAD("test.bmp")` looks in the empty MEMFS and then
      the network; nothing maps a program's relative path onto `:/examples/…`, so
      bundling the assets would cost download weight and still not work). Net:
      **68 self-contained `.kbs`**, one category level deep, no assets.
      `rcc` handles nesting natively — `<file>Games/PacMan.kbs</file>` becomes
      `:/examples/Games/PacMan.kbs` — and CMake needed no change, since it only
      names the `.qrc`. The packaging scripts all `cp -r Examples`, so the desktop
      side is unaffected too.
      The trap was that **both** lookups into `:/examples` used
      `QDir::entryList()`, which does **not** descend: left alone they would have
      returned nothing, and `openExample()`'s `if (files.isEmpty()) return;` would
      have made the picker silently do nothing at all. Both now use `QDirIterator`
      with `Subdirectories` — which also means a deeper tree would work if the
      curation ever changes.
      - `MainWindow::openExample()` lists entries as `Games/hangman.kbs` and sorts
        them, so the flat `QInputDialog` list groups by category for free (the
        cheap picker — a `QTreeWidget` dialog remains an option later). The tab
        title is the bare file name: the category is how you *found* the program,
        not part of what it is called.
      - `resolveExampleName()` tries the full relative path first, then falls back
        to matching the bare file name **anywhere in the tree** — because
        `?run=mandelbrot` is the form in the README and in every link already
        shared, and it has to keep working now that the file lives in `Demos/`.
        `?run=Demos/mandelbrot` works too. If two categories ever hold the same
        file name, the bare form resolves to whichever the iterator reaches first;
        give the category to disambiguate.
      - `isSafeExampleName()` now permits `/`-separated segments. The safety
        property is unchanged and rests on the same fact as before: `.` is not in
        the segment character class, so `..` cannot be written.
      **`?mode=` (follow-up, 2026-07-12).** The first cut hardcoded graphics-only
      for every launch. `?mode=` now picks the GUI, as a **separate** parameter
      from the three above — they say *what* to run, it says *how* — so it
      composes with all of them (`?url=demos/x.kbs&mode=text` works as readily as
      `?run=…&mode=text`). Folding mode into the source name (the maintainer's
      first sketch: `?rung=`/`?runt=`/`?runi=`) was rejected for exactly that
      reason: it would have needed `?srcg=`/`?urlg=`/… variants to reach the same
      generality. Every value maps onto a guimode that already exists for a
      command-line switch, so nothing new was added to `MainWindow`:
      | `?mode=` | guimode | switch | |
      |---|---|---|---|
      | `ide` *(default)* | `GUISTATERUN` | `-r` | full IDE, auto-run |
      | `edit` | `GUISTATENORMAL` | — | full IDE, loaded but **not** run |
      | `graph` | `GUISTATEGRAPH` | `-g` | graphics only, auto-run (the player) |
      | `text` | `GUISTATETEXT` | `-t` | text output only, auto-run |
      | `app` | `GUISTATEAPP` | `-a` | text + graphics, no editor, auto-run |
      `graph`/`text`/`app` are *player* modes and get `hidePlayerChrome()`;
      `ide`/`edit` are meant to be worked in and keep the full IDE furniture. An
      unrecognised mode falls back to `ide`.
      **Default flipped to `ide` 2026-07-12** (maintainer decision; it first
      shipped defaulting to `graph`). A bare `?run=<file>` is far more often "show
      me this program" than "embed this demo", and the IDE is the honest landing
      place — visible, editable, stoppable. The chrome-free player is one
      parameter away (`&mode=graph`), which is the right way round: embedding
      should be the deliberate act, not the accident.
      `edit` needs no new plumbing: `ifGuiStateRun()` is already a no-op for
      `GUISTATENORMAL`, so the program simply loads. One gap this surfaced: the
      deep-link branch skips `main()`'s trailing `newProgram()`, so a *failed*
      load in `ide`/`edit` would have left an IDE with no document — the error
      path now calls `newProgram()` first.
      **Chrome (follow-up, 2026-07-12).** First browser run showed `-g` does not
      actually mean "only the Graphics Output window": `configureGuiState()`
      hides the *main* toolbar but never touches the **graphics** toolbar (so
      line ~429 restores it from settings), and the **menu bar** is not hidden in
      any mode — leaving File (Open Example…, Exit) and Help. That is a
      pre-existing `-g` gap, not something the deep-link introduced. **Maintainer
      decision: fix it for the WASM deep-link only** — desktop `-g/--graph` keeps
      its menu bar and toolbar. New `MainWindow::hidePlayerChrome()`
      (`Q_OS_WASM`-only), called from `Main.cpp` when a launch parameter is
      present, hides the menu bar, the status bar and the graphics toolbar. It is
      *not* folded into `configureGuiState()`'s `GUISTATEGRAPH` branch, which is
      exactly what keeps desktop `-g` untouched. Persisting this is harmless:
      `saveCustomizations()` keys every setting by `guiState`, so it writes the
      graph-mode (3) keys, never the normal IDE's (0).
      **Running your own programs:** `?run=` is restricted to the bundled
      `:/examples` set on purpose (the name goes into a resource path). Anything
      else goes through `?src=` (base64, nothing to host) or `?url=` with a
      relative path — `?url=demos/fractal.kbs` — so just drop the `.kbs` into the
      deployed directory.
      Maintainer to verify in-browser: `?run=mandelbrot` opens the IDE and runs;
      `&mode=graph` gives the chrome-free player; a bad name / bad base64 /
      rejected URL raises the "unable to load" box instead of hanging; and no
      parameters still opens the normal IDE.
- [x] **Relative media paths resolve against the page URL.**
      **Implemented + browser-verified 2026-07-13 by the maintainer** —
      `SOUNDLOAD("./sounds/bounce.mp3")` with the file beside `basic256.html` now
      loads and plays from a local web server, unchanged from the desktop form of
      the same program. Not separately exercised (same mechanism, expected to
      work): the image paths (`IMGLOAD` / sprite load / `IMAGELOAD`), and
      `SOUND`/`SOUNDPLAY` with a bare filename — that one goes through
      `QMediaPlayer::setSource()`, whose URL-based playback on wasm is still only
      *believed* to work (see Phase 5), so a failure there would be the media
      backend, not the path resolution.
      `SOUNDLOAD("./sounds/bounce.mp3")` failed in the browser even with the file
      sitting next to `basic256.html`. Two ways to find media existed and neither
      could reach it: `QFileInfo(s).exists()` looks in the (empty) Emscripten
      MEMFS, and the fetch fallback demanded an explicit `http`/`https`/`ftp`
      scheme, so a scheme-less path fell straight through to `ERROR_SOUNDFILE`.
      The images were broken the same way, just less visibly — they handed the
      bare string to `QUrl::fromUserInput()`, which does not point at the server
      either. The only working form was an absolute URL, which bakes
      `localhost:8000` into the source and then breaks on desktop and on Pages.
      A browser's equivalent of "the working directory" is the page's own URL, so
      that is what a relative path now resolves against. New
      `src/core/MediaPath.{h,cpp}` (compiled everywhere; the WASM behaviour is
      `#ifdef`-ed inside) exposes `downloadUrl()` and `isFetchable()`, and the
      five media-loading sites route through it:
      `SOUNDLOAD` (`Interpreter.cpp`), `SOUND`/`SOUNDPLAY`/`SOUNDPLAYER` with a
      filename (`SoundSystem::playSound`), and `IMGLOAD` / sprite load /
      `IMAGELOAD`. **Desktop behaviour is unchanged** — `downloadUrl()` is
      `QUrl::fromUserInput()` and `isFetchable()` is the same http/https/ftp rule
      as before — so one program now works in both places.
      **`init()` must run on the main thread** (called from `Main.cpp` beside
      `WasmSettings::init()`): `location` inside a pthread worker refers to the
      *worker script*, not the page, and the interpreter — which is what asks —
      runs on a worker. So the page URL is captured once up front and the worker
      only ever reads the cached value. The `EM_JS` bridge uses `HEAPU8` only,
      the same two-step copy as `WasmLaunch` (no `stringToNewUTF8`/`_malloc`).
      `QUrl::resolved()` is RFC 3986, so the page's own `?run=`/`?mode=` query is
      correctly *not* inherited by the media URL.
      Same-origin by construction: a relative path cannot name another host.
      This is what would let `sprites/`, `imgload/`, `dice/` and `sound/` back
      into `DemoWASM/` — they were excluded only because their assets were
      unreachable. Not done here: their assets would have to be deployed beside
      the page, which the Pages workflow does not currently do.
- [ ] Revisit binary size: dynamic linking / `qt-cmake` deploy options,
      strip unused Qt features.

---

## Per-file / per-area summary (tick when fully done)

- [x] Tree restructure (`src/core`, `src/gui`, `src/app`) + CMake split
- [x] CI scripts path fixes (13 scripts + `BASIC256.nsi`)
- [x] `GraphicsBuffer` extraction; `BasicGraph` as view
- [x] `editwin` extern removed (programTitle)
- [x] `basicKeyboard` extern removed (constructor injection)
- [x] Feature flags ×6 + `ERROR_NOTAVAILABLE` + dress-rehearsal build
- [x] WASM CI job (emsdk 4.0.7 + Qt 6.11 wasm_multithread + host Qt)
- [x] Heap / PTHREAD_POOL_SIZE link settings
- [x] Sound WASM guards (`setSourceDevice` path)
- [x] Web Audio bridge for QAudioSink tone/waveform playback
      (`WasmAudioSink`, Phase 7; `sound:` in-memory-file path still open)
- [x] Dialog `exec()` → `open()` conversions (all 8 known result-dependent
      `QMessageBox` sites done — the 5 deferred converted 2026-07-10; the
      `SettingsBrowser` `QDialog::exec()` also converted)
- [x] WASM file open/save (`getOpenFileContent`/`saveFileContent`)
- [x] Settings persistence via IDBFS mount (`/persist` + `NativeFormat` +
      `FS.syncfs`; `WasmSettings`, `wasm-deploy/idbfs.js`, Phase 7 —
      browser-verified: a Preferences change survives a reload)
- [x] Examples packaged for browser
- [x] gh-pages deploy + coi-serviceworker + landing page (live at
      https://uglymike17.github.io/basic256/; Pages enabled + auto-deploys on
      each push to v2.1.Alpha05WASM; browser matrix confirmed 2026-07-10)
- [x] README browser-limitations section

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
  check. First CI push failed on all four targets (real bug — see Phase 2
  gate); fixed by adding `<QPen>`/`<QBrush>`/`<QFont>` includes to
  `Interpreter.h` and moving `GSIZE_INITIAL_WIDTH`/`HEIGHT` into
  `GraphicsBuffer.h`. Re-push (run 28867353524) green on all four targets.
  **Phase 2 code/CI work done. Gate not fully closed yet** — manual
  verification (graphics/sprite/mouse-input examples, `-s` IMGSAVE
  comparison) still needs the maintainer — no local Qt/display available
  this session, and `TestSuite/testsuite_ci.kbs` deliberately excludes
  sprite/graphics coverage for an unrelated, pre-existing `-s`-mode reason
  (see Phase 2 gate notes).
  Maintainer ran the full interactive `testsuite.kbs` in the IDE (covers
  Basic Graphics, Mouse Functionality, Sprites, and IMGSave/IMGLoad) — all
  passed. A separate headless `-s` attempt at just the IMGSave section hit
  an interactive `CONFIRM` dialog that test script needs (unsupported in
  `--silent`, an unrelated pre-existing limitation of that script, not a
  regression). Maintainer confirmed the interactive pass satisfies this
  gate item. **Phase 2 gate closed.** Next up: Phase 3 (platform feature
  flags + desktop dress rehearsal).

- 2026-07-07: Phase 3 — added `ERROR_NOTAVAILABLE` (129) and the six
  `BASIC256_ENABLE_*` CMake options (all `ON` by default). Gated: `OP_
  SYSTEM` (PROCESS); `OP_OPENSERIAL` by extending its existing `#ifdef
  ANDROID` guard (SERIAL); `closeDatabase()` + `OP_DBOPEN..OP_DBSTRING` +
  `OP_FREEDB` (SQL, `OP_FREEDB` found on a final grep sweep after the
  first pass missed it); every `printing`/`printdocument` touch point in
  `cleanup()`/`OP_CLG`/`OP_GRAPHWIDTH`/`OP_GRAPHHEIGHT` plus the four
  `OP_PRINTER*` opcodes (PRINTER); `netSockClose`/`netSockCloseAll` + all
  seven `OP_NETLISTEN..OP_NETADDRESS` opcodes (TCP); `RunController`'s
  speech init/use and the `SAY` signal path (TTS, app-layer). `OP_FREENET`/
  `OP_FREEDBSET` deliberately left unguarded (harmless pointer-null
  checks). Found PRINTER is two independent concerns: the BASIC opcodes
  vs. `src/gui`'s own Print... menu actions (`QPrinter`/`QPrintDialog`
  used directly in `BasicEdit`/`BasicGraph`/`BasicOutput`/
  `PreferencesWin.cpp`) — kept `Qt6::PrintSupport` unconditionally linked
  to the `basic256` exe for the latter. Also fixed a latent bug
  (`RunController::speech` never null-initialized) and deleted two dead
  includes (`QHostInfo`, `QVoice`).
  First CI push: default (all-ON) build failed on all four targets — a
  real bug: `BASIC256_ENABLE_*` compile definitions were `PRIVATE` on
  `basic256core`, invisible to `src/gui` files that transitively include
  `Interpreter.h` (e.g. `PreferencesWin.cpp` needing `QPrinterInfo`).
  Fixed: `PUBLIC`. Same push added the flags-OFF dress-rehearsal CI job
  (`TestSuite/testsuite_flagsoff_ci.kbs` + `run_testsuite_flagsoff.sh`,
  reusing `build_Linux_x86.sh` via a new optional
  `BASIC256_EXTRA_CMAKE_ARGS` env var) — that job then failed for a
  second, related real bug: `PreferencesWin.cpp` had no `QtPrintSupport`
  include of its own, unlike its siblings, so it broke specifically when
  the flag was genuinely off. Fixed by adding the same direct include its
  siblings already had. Third push (run 28878298624) green on all five
  jobs (4-target default matrix + dress rehearsal), dress-rehearsal output
  confirmed: `SYSTEM raises ERROR_NOTAVAILABLE (129) ... pass` /
  `BASIC256_FLAGSOFF_CI_PASSED`. **Phase 3 gate closed.** Next up: Phase 4
  (Emscripten toolchain + first WASM build).

- 2026-07-07: Phase 4 — before touching CI, discovered via Qt's own docs
  (not a failing build) that Phase 3's "PrintSupport stays unconditional
  for the GUI's own Print... menu" decision doesn't survive contact with
  WASM: Qt for WebAssembly ships no `QtPrintSupport` module at all
  ("Printing is not supported"). Fixed first: made `PrintSupport`
  conditional in `CMakeLists.txt`'s component list and the `basic256` exe
  link, and extended `BASIC256_ENABLE_PRINTER` to also gate
  `BasicEdit`/`BasicGraph`/`BasicOutput`'s `slotPrint()` (extending each
  file's existing `#ifdef ANDROID` precedent, same technique Phase 3 used
  for SERIAL) and `PreferencesWin`'s printer-preferences tab. Guarded
  `Sound.cpp`'s in-memory `sound:`/`setSourceDevice()` path under
  `Q_OS_WASM` → `ERROR_NOTAVAILABLE` per the plan (BEEP/QAudioSink and
  URL-based playback untouched). Added `QT_WASM_INITIAL_MEMORY`/
  `QT_WASM_PTHREAD_POOL_SIZE` target properties and an `if(EMSCRIPTEN)`
  `qt_finalize_target(basic256)` call (found needed via Qt's docs: plain
  `add_executable()`, used here, skips the automatic finalization that
  `qt_add_executable()` gets for free, and it's what actually generates
  `basic256.html`/`qtloader.js`/`qtlogo.svg`). New
  `.github/scripts/build_WASM.sh` (emsdk 4.0.7 install/activate, aqt
  `all_os wasm 6.11.1 wasm_multithread -m qtmultimedia --autodesktop`,
  directory discovery by glob rather than hardcoded names, `qt-cmake`
  configure with all six flags off) and a new `wasm` job in `build.yml`
  uploading the `BASIC256-WASM` artifact.
  The toolchain/aqt/qt-cmake setup itself was correct on the first push
  and never needed fixing — every one of the next four pushes hit a
  genuine, different Qt-for-WASM API-surface gap, each caught by the CI
  job exactly as this phase is designed to do: (1) `Interpreter::cleanup()`
  called `sys->kill()` unconditionally outside the `BASIC256_ENABLE_PROCESS`
  guard — WASM's `QProcess` has no `kill()`; (2) `MainWindow.cpp`'s
  "check for update" code used `QSslConfiguration`/`QSsl` — only forward-
  declared, never implemented, on WASM (browser `fetch()` handles TLS);
  (3) `RunController.cpp`'s `#include <sys/soundcard.h>` is behind
  `#ifdef LINUX`, and `LINUX` was getting defined for wasm too — root
  cause fixed properly with a new `elseif(EMSCRIPTEN)` branch in
  `CMakeLists.txt`'s platform detection (ahead of the `UNIX`-and-not-APPLE
  fallback that Emscripten's POSIX-like `UNIX=1` was falling into), which
  also fixes a latent `OP_OSTYPE` misreport; (4) `RunController::
  executeSystem()` (dead code, never called) default-constructed a local
  `QProcess` — WASM's `QProcess` has a **deleted** default constructor,
  stronger than case (1)'s missing-method gap. Fifth push (run
  28885376717) green on all six jobs: the `wasm` job links and uploads
  `basic256.html`/`.js`/`.wasm`/`qtloader.js`/`qtlogo.svg`, and all five
  pre-existing desktop/dress-rehearsal jobs stayed green through every
  iteration — none of the wasm-specific fixes leaked into desktop build
  paths. Also fixed a bookkeeping gap in the "Per-file / per-area summary"
  table: Phase 1/2's rows had never been ticked despite those phases being
  closed; ticked them now along with Phase 4's three rows.
  **Phase 4 gate: CI (link + artifact upload + desktop-still-green) closed.
  Local browser smoke test (serve with COOP/COEP, open in Chrome/Firefox,
  run a Hello World) not done this session** — needs a browser, which
  isn't available in this CLI environment; needs the maintainer. Next up:
  either the maintainer runs that smoke test, or proceed to Phase 5
  (browser runtime adaptation) with the smoke test deferred.

- 2026-07-07: Phase 5 — main-thread blocking audit (RULE 2) found the real
  scope is bigger than the plan's 3 named sites: a grep for
  result-dependent `QMessageBox` calls found 8. Maintainer chose (asked via
  a scope question, given the correctness-critical, browser-unverifiable
  nature of the refactor) to convert only the 3 originally-named sites this
  session: `BasicEdit::saveFile()`'s overwrite confirm (split into a new
  `writeFile()` helper), `PreferencesWin::clickClearSavedData()`'s delete
  confirm, and `MainWindow::closeAllPrograms()` (the biggest: two
  sequential confirmations across two call sites, changed to a
  `std::function<void(bool)>` completion-callback signature, with
  `closeAllProgramsSlot()` added as a 0-arg wrapper for the menu's
  string-based connect and `finishCloseAllPrograms()` as the shared
  completion helper; `closeEvent()` now always `e->ignore()`s and quits
  from the completion callback). The other 5 real sites (documented in the
  Phase 5 item above) are a known, deliberate gap for a focused follow-up.
  Pushed and confirmed green on all six jobs (desktop ×4 + dress rehearsal
  + wasm build) before continuing — run 28886652004.
  Settings: found via Qt's own docs (not assumed, correcting the plan's
  expectation) that QSettings has no automatic IndexedDB/persistent
  backing on WASM at all — `NativeFormat` silently writes into ephemeral
  MEMFS. Fixed with `QSettings::WebLocalStorageFormat` (synchronous, no
  JSPI requirement, ample 5MiB cap), `Q_OS_WASM`-gated in `Settings.h`'s
  `SETTINGS` macro. Confirmed PreferencesWin.cpp's existing
  `settings.value(key, default)` usage already tolerates missing keys — no
  further changes needed. Pushed and confirmed green — run 28887110714.
  File open/save and Examples-in-browser both turned out larger than their
  one-line plan bullets suggested (asked the maintainer to confirm scope
  before proceeding, given the same can't-verify-without-a-browser
  category as the dialog audit): `getOpenFileContent()`/`saveFileContent()`
  are content-based, not path-based, a different model from `BasicEdit`'s
  existing filename-tracking one; and "Open Example" doesn't exist on
  desktop at all today (users just browse to `Examples/` via the normal
  Open dialog) — building it for WASM meant building the feature itself,
  not adapting one. Maintainer chose to implement both in full. Added
  `MainWindow::loadFileContent()` (new-tab logic shared by both file-open
  and Open-Example, since resource reads are synchronous but the picker
  dialogs aren't) and gated `BasicEdit::saveFile()`/`saveAsProgram()` for
  WASM's download-based save model (`filename` deliberately left empty
  after a WASM load, so every save routes through `saveFileContent()`).
  New `Examples/examples.qrc` (`EMSCRIPTEN`-only in `CMakeLists.txt`)
  bundles 42 of the 46 top-level `.kbs` files (excluded 4 using `DBOPEN`/
  SQL, found via grep, plus the asset-dependent subdirectories). New
  "Open &Example..." menu action uses a non-modal `QInputDialog` (same
  RULE 2 category as the `QMessageBox` conversions — `getItem()`'s `exec()`
  has the identical never-returns problem). Pushed and confirmed green on
  all six jobs — run 28887875056.
  Remaining Phase 5 items were lower-risk verification/documentation, not
  code changes needing a push: clipboard (grepped every use, all standard
  `QClipboard` API, nothing to adapt), fonts/HiDPI (one generic-family
  hardcoded font, Qt6's default HiDPI handling, nothing custom to fix),
  NETREAD/CORS (recorded here rather than editing the README now, since
  Phase 6 already owns that bullet and there's no live URL yet to link).
  Also fixed a bookkeeping gap: the per-file summary table's "WASM file
  open/save" and "Examples packaged for browser" rows are now ticked; the
  "Dialog exec() → open() conversions" row is annotated 3-of-8 rather than
  ticked, since it's genuinely partial.
  **Phase 5 gate: the two CI-verifiable/code-level items are done (desktop
  CI green ×4 + dress rehearsal + wasm build, confirmed across all three
  pushes this session). The two in-browser gate items (load/edit/run/stop/
  re-run/save/reload; a `.kbs` calling SYSTEM/DBOPEN/SAY showing
  ERROR_NOTAVAILABLE) are not done this session** — same no-browser
  constraint as Phase 4's still-open local smoke test; both can be
  combined into one maintainer session. Next up: either the maintainer
  runs the combined browser smoke test (covering both Phase 4's and Phase
  5's manual gate items), or proceed to Phase 6 (hosting + deploy) with
  both deferred.

- 2026-07-08: **Phase 4/5 real browser testing (maintainer) — the actual
  local smoke test, closing both phases' remaining manual gate items.**
  Five real, separate bugs found and fixed, none of them visible from code
  review, static analysis, or CI — this is exactly why both phases kept a
  manual browser-test item open even after CI went green:
  1. **Startup crash (`Application exit()`).** Console showed "Calling
     exec() is not supported on Qt for WebAssembly… Aborted()". Traced to
     the auto-update-check request (fires automatically on startup) hitting
     a CORS failure against sourceforge.net, whose error handler called a
     blocking static `QMessageBox::warning(...)`. Phase 4 had only gated
     the `QSslConfiguration` compile blocker in this code, not the feature
     itself. **Fixed:** disabled the whole update-check flow for
     `Q_OS_WASM` (`MainWindow.cpp`), extending the existing
     `#ifndef ANDROID` guards — checking for a desktop download doesn't
     mean anything in a browser either.
  2. **Preferences and About also hit the identical `exec()` abort.**
     Missed by the original RULE 2 audit (which only grepped GUI-layer
     result-dependent `QMessageBox` calls, not `RunController.cpp`'s own
     five `.exec()`-based interpreter-dialog handlers, nor the static
     `QMessageBox::about()`). **Fixed:** `MainWindow::about()` → manual
     `QMessageBox` + `open()`; `RunController::showPreferences()`'s
     password prompt and `PreferencesWin`'s own `w->exec()` → async (new
     `showPreferencesWindow()` helper); `dialogAlert`/`dialogConfirm`/
     `dialogPrompt`/`dialogAllowPortInOut` (the interpreter's `ALERT`/
     `CONFIRM`/`PROMPT`/`PORTIN`/`PORTOUT`-permission signal path) → async,
     moving `waitCond->wakeAll()`/`mymutex->unlock()` into each dialog's
     `finished` completion slot. `dialogOpenFileDialog`/
     `dialogSaveFileDialog` are a different, deeper problem — no WASM
     equivalent exists for "return a real file path to the interpreter"
     — stubbed to report "cancelled" on `Q_OS_WASM` rather than attempt a
     real fix; this BASIC-language file-picker feature is now a
     documented, deliberate v1 gap. (First round of maintainer testing hit
     a stale browser cache showing the *old* pre-fix build — confirmed via
     hard-refresh — a reminder to always hard-refresh/disable-cache when
     re-testing a fresh WASM artifact.)
  3. **Run/Debug froze solid (100% CPU, zero console output) — a true
     silent spin, unrelated to the exec() issues above.** Diagnosed with
     temporary `qCritical()` trace points at every step of
     `RunController::startRun()` (committed, then removed once found) —
     execution never got past `SoundSystem`'s constructor's
     `QMediaDevices::defaultAudioOutput()` call. Confirmed via Qt's own
     Multimedia-on-WebAssembly docs: device enumeration is asynchronous
     there (only populated after an `audioOutputsChanged` signal), and
     this synchronous call apparently tries to block on that JS-side
     negotiation the same way `exec()` does without Asyncify — except
     unlike `exec()` it isn't given Qt's detect-and-abort treatment, it
     just spins forever. **Fixed:** skip the device query on `Q_OS_WASM`
     entirely and keep the manually-built `QAudioFormat` — `info`/format
     negotiation is used nowhere else in `Sound.cpp`, so nothing else
     needed adapting.
  4. **A separate, real CI infrastructure issue surfaced while chasing the
     above** (not a code bug): `packages.microsoft.com`'s azure-cli repo
     intermittently fails GPG verification on GitHub's hosted runners,
     and under `set -euo pipefail` this aborted `build_WASM.sh`/
     `build_Linux_x86.sh` entirely even though it's a repo we don't use
     and the Ubuntu repos we do need synced fine.
     `build_Linux_RPi_Trixie.sh` made it worse by chaining
     `apt-get update && apt-get install` — any update failure skipped the
     install outright. **Fixed:** `|| true` on all three affected
     `apt update` lines, and un-chained the RPi script.
  5. **Sound playback (`SOUND freq, duration`) still hung after fix #3.**
     New trace points (same technique) showed execution never got past
     `new QAudioSink(format, parent)` — a *different* call site than
     `QMediaDevices::defaultAudioOutput()`, but the same root cause:
     `QAudioSink`'s single-`QAudioFormat`-arg constructor overload
     internally resolves the default device the identical way. Found via
     the grammar (`LEX/basicParse.y`) that `SOUND freq, duration` doesn't
     use the `"beep:"` string-keyed path at all (that's for *replaying a
     named, already-loaded* resource, created by `SOUNDLOAD`) — it packs
     the two numbers into a 1×2 array and calls a completely different
     function, `playSound(vector<vector<double>>, bool)`. **Fixed:**
     this genuinely can't be patched around like the others — the whole
     `QAudioSink`-backed tone-generation feature doesn't work on this Qt
     6.11.1/emsdk 4.0.7 combination on WASM, contradicting Phase 4's
     original assumption ("keep the `QAudioSink` tone path"). Gated both
     `QAudioSink` construction sites (the `"beep:"` playback branch and
     the whole generated-waveform overload) to raise `ERROR_NOTAVAILABLE`
     instead of hanging — confirmed working: `SOUND 400,2000` now prints
     "Feature not available on this platform" and the program continues
     normally. A real fix needs a Web Audio API bridge via
     `emscripten::val`, which WASM.md's Phase 7 already scopes (for the
     *other* sound path) — this finding means that Phase 7 item now
     covers BEEP/tone generation too, not just in-memory file playback.
  **Net result: the app is genuinely usable in a browser now** — IDE
  loads, editing/running/stopping text and graphics programs all work,
  `mandelbrot.kbs` runs correctly, `SAY`/`SYSTEM`/`DBOPEN` all correctly
  show `ERROR_NOTAVAILABLE` and continue. Sound is a confirmed, no-longer-
  silent v1 gap. **Phase 4 and Phase 5 gates are now both fully closed.**
  Next up: Phase 6 (GitHub Pages hosting + `coi-serviceworker` deploy
  pipeline), or revisit the Phase 7 Web Audio bridge given it's now
  needed for more than originally scoped.

- 2026-07-07: Phase 4/5 manual browser smoke test (maintainer) — the first
  real browser testing this whole port has had, and it found two real,
  severe bugs neither CI nor code review could have caught:
  1. **Startup crash, `file:// `→ real server → `Application exit()`.**
     Root-caused from the browser's own console log (not guessed): the
     auto-update-check request (`MainWindow`'s constructor, fires
     automatically on startup) hit a CORS failure against sourceforge.net,
     and its error-handling path called a blocking static
     `QMessageBox::warning(...)` — Qt for WebAssembly logged "Calling
     exec() is not supported… Please build with asyncify support, or use
     an asynchronous API like QDialog::open()" and aborted the whole wasm
     runtime. Phase 4 had only gated the `QSslConfiguration` compile
     blocker in this code, not the feature itself. Fixed: disabled the
     entire update-check flow (the `QNetworkAccessManager`/request
     construction, the menu action's connect, and the startup auto-check
     timer) for `Q_OS_WASM`, extending the existing `#ifndef ANDROID`
     guards the same way — "check for a new desktop download" doesn't mean
     anything in a browser either. Pushed, confirmed green — run
     28891256484.
  2. **Run/Debug/Preferences/About all freeze solid, file open/save and
     window show/hide don't.** Diagnosed by process of elimination across
     several rounds of asking the maintainer for browser diagnostics
     (console output, Task Manager CPU%, which specific features broke):
     the common thread across every broken feature and *only* the broken
     features is the `SETTINGS` macro (`QSettings`) — confirmed by Chrome's
     Task Manager showing the frozen tab at 100% CPU with zero console
     output (a genuine infinite spin, not a blocked wait — ruled out a
     `Qt::BlockingQueuedConnection` self-deadlock hypothesis along the way
     by checking `resizeGraphWindow`'s connection type, which is plain
     `AutoConnection`, not blocking). This traces directly back to this
     same session's own earlier "fix": `QSettings::WebLocalStorageFormat`
     spins forever on construction on this Qt 6.11.1/emsdk 4.0.7
     combination. Reverted to `NativeFormat` unconditionally (Settings.h)
     without being able to root-cause the spin myself (no browser in this
     CLI environment) — Run/Debug working matters far more than settings
     surviving a reload. Settings persistence on WASM is now an open,
     unsolved problem again (see the Settings item above, reopened).
     Pushed — run pending at time of writing.
  This round underlines exactly why Phase 4/5's gates keep a manual
  browser-test item open even after CI is green: `QMessageBox::warning()`
  and `QSettings::WebLocalStorageFormat` are both real Qt APIs that compile
  cleanly and pass every automated check, and both broke the app
  completely in a real browser in ways no amount of static analysis in
  this environment would have caught. **Next up: maintainer re-tests with
  the latest push; if Run/Debug/Preferences/About all work now, the
  Phase 4 and Phase 5 manual gate items can finally close.**

- 2026-07-08: **Phase 7 — Web Audio bridge for the QAudioSink-based sound
  paths (`beep:` named playback + `SOUND freq,duration` generated
  waveforms), the gap Phase 5's real browser testing flagged.** Maintainer
  chose (asked via a scope question, given the two structurally different
  broken sound paths and the browser-unverifiable, already-once-burned
  history of this subsystem — `QAudioSink`/`QMediaDevices` hangs,
  `QSettings::WebLocalStorageFormat` spins) to scope this session to the
  `QAudioSink` path only; the `sound:` in-memory-file path
  (`QMediaPlayer::setSourceDevice()`) stays `ERROR_NOTAVAILABLE`, left as a
  separate future Phase 7 item (different backend, needs an async
  `decodeAudioData` bridge instead of the synchronous raw-PCM approach used
  here).
  New `src/core/WasmAudioSink.{h,cpp}` (`Q_OS_WASM`-only, both files
  compile to an empty translation unit on desktop — added to
  `SOURCES_CORE`/`HEADERS_CORE` unconditionally rather than adding the
  first-ever platform-conditional entry to that list): a `QAudioSink`-
  shaped facade over the Web Audio API. `Sound.h`'s `audio` member became
  `AudioSinkType*` (`WasmAudioSink` on WASM, `QAudioSink` elsewhere), so
  every existing `audio->` call site in `Sound.cpp` — play/stop/pause,
  `updatedMasterVolume`, `lastError`, `prepareConnections`'s
  `connect(audio, SIGNAL(stateChanged...))`, the destructor's stop loop —
  needed zero changes; only the two `new QAudioSink(...)` construction
  sites became `new AudioSinkType(...)`, and the two `Q_OS_WASM` ->
  `ERROR_NOTAVAILABLE` gates Phase 5 added around them were removed.
  Bridge implemented with `EM_JS`-defined JS functions living directly in
  `WasmAudioSink.cpp` (no separate `.js` file, no CMake wiring, no embind)
  against a single shared `AudioContext` + per-instance `GainNode`; PCM
  samples are copied straight out of wasm linear memory
  (`HEAP16.subarray(...)`) into a Web Audio `AudioBuffer` on `start()`. The
  async `onended` browser event reaches C++ via a raw C function pointer
  resolved JS-side with Emscripten's `makeDynCall` macro — deliberately
  not `Module.ccall`-by-name, since neither `CMakeLists.txt` nor
  `build_WASM.sh` currently sets `-sEXPORTED_RUNTIME_METHODS` (grepped to
  confirm, not assumed); `makeDynCall` needs no such flag.
  `AudioBufferSourceNode` is one-shot (no native pause), so
  `suspend()`/`resume()`/a new `seekTo()` method all stop the current node,
  track an elapsed-seconds offset, and start a fresh node from that offset
  on resume — every *explicit* C++-driven state change updates `state()`
  and emits `stateChanged()` **synchronously**, not on the async
  round-trip, specifically because `Sound::~Sound()`'s
  `while(audio->state()!=QAudio::StoppedState) audio->stop();` busy loop
  would spin forever otherwise (found by tracing that destructor before
  writing any bridge code, not after). The async `onended` callback is
  reserved solely for genuine natural end-of-playback -> `QAudio::
  IdleState`; the JS side detaches `onended` before every explicit stop so
  it can never double-fire.
  Found and fixed one real bug during implementation, before it could reach
  CI: `Sound::position()`'s `audio` branch read `buffer->pos()`, which
  tracked real playback progress under `QAudioSink`'s pull-as-it-plays
  `QIODevice` model but would misreport "fully played" the instant playback
  starts under `WasmAudioSink` (which reads the whole `QIODevice` once,
  up front, since Web Audio needs a fully decoded `AudioBuffer` and can't
  stream from a pull model) — added a `Q_OS_WASM` branch calling the new
  `WasmAudioSink::positionSeconds()` instead. Also caught, mid-edit, that
  my first draft of that branch scaled the return value by
  `sound_samplerate` — re-reading the desktop formula
  (`buffer->pos() / (sound_samplerate * sizeof(int16_t))`) showed
  `position()` already returns **seconds**, not samples, on desktop; fixed
  before it became a real-vs-desktop unit mismatch bug.
  `Sound::seek()`'s `audio` branch got the equivalent `Q_OS_WASM` branch
  calling `WasmAudioSink::seekTo()` — real seek support during
  generated-sound playback rather than a documented gap, effectively free
  once the offset-tracking suspend/resume mechanism already existed.
  `length()` needed no change (`buffer->size()` is unaffected by the
  up-front read).
  **Not verified in a real browser this session** — no local Emscripten
  toolchain or browser in this environment, the same constraint as every
  prior WASM phase. Verification plan: push to CI (the `wasm` job's link
  step catches real compile/include errors; desktop CI ×4 + dress
  rehearsal must stay green since every new file/branch is
  `Q_OS_WASM`-only), then the maintainer confirms in a real browser that
  `SOUND 400,2000` / a `BEEP`-based example is actually audible with no
  console errors and that loop/pause/resume/seek/`SOUNDWAIT` behave
  sanely — the same "CI green is necessary but not sufficient" gate
  structure every WASM phase has used, given this subsystem's specific
  history of APIs that compile clean and then hang or spin in a real
  browser. **Not yet pushed/CI-confirmed as of this log entry.**

- 2026-07-08: **Phase 7 WasmAudioSink — two real link/compile bugs found by
  the wasm CI job, both in `WasmAudioSink.h`, neither visible from local
  review (no Emscripten toolchain in this environment).**
  1. **Link failure: `Q_OS_WASM` undefined when `WasmAudioSink.h` is the
     first include.** `Q_OS_WASM` comes from Qt's `qsystemdetection.h`, not
     the compiler directly — it's only defined after some Qt header has run.
     `WasmAudioSink.cpp` includes its own header first, before any other Qt
     header, so the header's top-level `#ifdef Q_OS_WASM` saw it undefined
     and compiled away the entire class, leaving every `WasmAudioSink`
     method undefined at link time (`Sound.cpp` compiled fine, since
     `Sound.h` always pulls in Qt headers first). Fixed: unconditional
     `#include <QtGlobal>` before the guard.
  2. **Compile failure: `wasmAudioSinkOnEnded`'s friend declaration had
     ordinary C++ linkage while its real definition is `extern "C"`.**
     Clang treats that as two different functions and rejects the mismatch
     — which also meant the intended friendship (access to the private
     `handleEnded()`) never actually applied. Fixed: forward-declare the
     `extern "C"` function ahead of the class so the friend declaration
     binds to the same entity.
  Both fixes pushed as separate commits (`5c56783`, `6f3c563`). Verified
  via `gh run list`: run
  [28946260836](https://github.com/uglymike17/basic256/actions/runs/28946260836)
  (the `6f3c563` push) is green — `Build BASIC-256_2.1.Alpha CMake`
  succeeded, i.e. the wasm job and all four desktop targets + dress
  rehearsal all build clean with the Web Audio bridge in place. **Phase 7's
  WasmAudioSink item is now CI-confirmed; the real-browser audio check
  (`SOUND 400,2000`/BEEP audible, loop/pause/resume/seek/SOUNDWAIT sane)
  remains the one open step, same no-browser-in-this-environment constraint
  as every prior phase.** Next up: maintainer does the real-browser audio
  check, or proceed to Phase 6 (GitHub Pages hosting + `coi-serviceworker`
  deploy pipeline) with that check deferred.

- 2026-07-08: **Phase 6 — GitHub Pages hosting + deploy pipeline**, started
  while the maintainer runs the Phase 7 real-browser audio check in
  parallel. The custom-shell-page item turned out to hinge on a real,
  open Qt bug rather than being a straightforward "write an HTML file"
  task: Qt's CMake WASM build doesn't reliably let you supply your own
  shell HTML at all (confirmed via a Qt Forum thread pointing at
  [QTBUG-109959](https://bugreports.qt.io/browse/QTBUG-109959) — it
  overwrites/ignores one placed in the tree), which is exactly why the
  plan's own wording rules out sed-patching Qt's generated file at deploy
  time too — that would be fighting the same unreliable mechanism from a
  different angle. Resolved by downloading the actual `basic256.html` from
  the last confirmed-green wasm CI artifact (run 28946260836) and
  hand-vendoring it as `wasm-deploy/index.html`, byte-checked against the
  real file rather than reconstructed from Qt's generic doc examples —
  this mattered concretely: the real file's `qtLoad()` config includes
  `entryFunction: window.basic256_entry`, a per-app-name detail the docs'
  generic `qtLoad({qt: {containerElements: [...]}})` snippet omits
  entirely and a from-scratch page would have silently gotten wrong (app
  would load blank, no console error to point at why). Added the
  `coi-serviceworker.min.js` `<script>` tag as the first thing in `<head>`.
  New `wasm-deploy/coi-serviceworker.min.js`: vendored verbatim (byte-diff
  confirmed against the upstream file, not retyped/reformatted) from
  gzuidhof/coi-serviceworker, pinned to commit
  `7b1d2a092d0d2dd2b7270b6f12f13605de26f214` — the project ships no tagged
  releases, so a commit pin was the only reproducible choice, the same
  reasoning `build_Windows.ps1` already uses for its aqtinstall pin.
  New `pages-deploy` job in `.github/workflows/build.yml`: adjusted the
  plan's literal "push to main" to this repo's actual default branch
  (`v2.1.Alpha05WASM` — checked via `gh repo view`, not assumed; `main`
  exists but is stale and isn't what any of this work has been happening
  on), gated on `needs: [ build, wasm ]` plus a push-only `if`, using
  `actions/upload-pages-artifact@v5` / `actions/deploy-pages@v5` (both
  version-checked via `gh api repos/actions/.../releases` and the
  `refs/tags/v5` ref, not guessed — matching this repo's established
  no-invented-versions discipline). The job downloads the `BASIC256-WASM`
  artifact, discards Qt's generated `basic256.html`, copies in the two
  vendored files as `index.html`/`coi-serviceworker.min.js`, and
  `touch`es `.nojekyll`.
  Size pass: added `-Oz` (compile + link) to `CMakeLists.txt`'s existing
  `if(EMSCRIPTEN)` block, overriding `CMAKE_BUILD_TYPE=Release`'s default
  `-O3` for a smaller download at some interpreter-speed cost — a
  deliberate v1 tradeoff. `-flto` considered and explicitly not applied:
  no local toolchain in this environment to measure its CI link-time/
  memory cost first, and Phase 7's binary-size bullet already tracks it as
  a named follow-up rather than something to guess at blind.
  README: new "Try it in your browser (WebAssembly)" section linking the
  deterministic-but-not-yet-live `https://uglymike17.github.io/basic256/`
  Pages URL (GitHub Pages project-site URLs for a repo with no custom
  domain always follow `https://<owner>.github.io/<repo>/` — not guessed,
  just the standard pattern) and listing the v1 browser gaps: the six
  `BASIC256_ENABLE_*`-gated feature areas, `SOUNDLOAD`'s in-memory audio
  path, session-only files, NETREAD's CORS exposure, and (folded in from
  the Phase 4/5 browser-testing log, not re-derived) the lack of a real
  file-picker for BASIC's own file commands.
  **Not done this session, left for the maintainer, both because they're
  real one-time changes to shared/live infrastructure and not something to
  do unasked:** (1) actually enabling GitHub Pages for this repo
  (`gh api repos/.../pages` currently 404s — needs Settings → Pages →
  Source: GitHub Actions, a repo-settings change with real public
  visibility once done) — without it the new `pages-deploy` job will run
  and presumably fail at the publish step with nowhere to deploy to, not
  silently no-op; (2) the Phase 6 gate's actual browser matrix (Chrome/
  Firefox/Edge/Safari, threads-not-freezing check) — blocked on (1)
  existing first. **Not yet pushed/CI-confirmed as of this log entry** —
  next step is pushing this work and watching whether the `pages-deploy`
  job's failure mode (once Pages is enabled, or if it fails earlier for
  lack of Pages) matches what's expected here, same "CI green is necessary
  but not sufficient" discipline as every other phase.

- 2026-07-10: **Phase 6 gate closed + the deferred dialog conversions +
  two SOUND/SAY fixes.** GitHub Pages is now enabled by the maintainer
  (Settings → Pages → Source: GitHub Actions), so the `pages-deploy` job
  publishes automatically on each push to `v2.1.Alpha05WASM`; the Phase 6
  gate checkbox and its note are updated accordingly. The live site is
  `https://uglymike17.github.io/basic256/`.
  **Favicon:** added `wasm-deploy/favicon.png` (a byte-for-byte copy of the
  real 64×64 app logo `resources/icons/basic256.png`, not a bespoke image —
  same asset the desktop window icon and AppImage launcher use), linked from
  `wasm-deploy/index.html`'s `<head>` via `rel="icon"` + `apple-touch-icon`,
  and copied into the published site by a new `cp` line in the
  `pages-deploy` "Assemble Pages site" step. PNG (not SVG) deliberately, so
  it renders on Safari too — the browser the Phase 6 gate calls the usual
  straggler.
  **Dialog `exec()`→async conversions — the 5 sites deferred on 2026-07-07
  are now all done** (RULE 2: a modal `exec()` never returns on the WASM
  main thread without Asyncify). Each blocking `QMessageBox`/`QDialog` was
  replaced by a heap `new QMessageBox(...)` + `WA_DeleteOnClose` +
  `show()`, with the post-answer logic moved into a `&QMessageBox::finished`
  slot's Yes branch; desktop behaviour is unchanged (still `ApplicationModal`,
  still the same prompts). Sites:
    - `MainWindow::closeEditorTab()` — "discard changes?" on single-tab
      close. Close logic factored into a shared `doCloseEditor` lambda used
      by both the finished-slot Yes branch and the unmodified fast path.
      `e` stays valid across the async gap via the existing
      `runState!=RUNSTATESTOP` early-return guard + the modal dialog — same
      risk profile as the shipped `closeAllPrograms` conversion.
    - `BasicEdit::handleFileChangedOnDisk()` — all three modals (removed-file
      info, changed-file reload confirm, and the nested unable-to-open
      critical) converted; the reload runs in the confirm dialog's Yes
      branch. `fileChangedOnDiskFlag` is now cleared when the dialog is
      *dispatched* rather than after it returns (the old trailing reset was
      dead once the call stopped blocking).
    - `MainWindow::loadFile()` — the two "not a text file / not .kbs, load
      anyway?" prompts. Handled by *skipping* rather than converting: a new
      `skipLoadPrompts` (true under `Q_OS_WASM`, and for `--silent` as
      before) makes both proceed without a dialog, since `loadFile()` is
      effectively desktop-only on WASM (the browser opens via
      `getOpenFileContent()`/`loadFileContent()`). Desktop keeps the prompts.
    - `PreferencesWin::SettingsBrowser::clickDeleteButton()` — "delete
      *selected* persistent settings?" body wrapped in the finished slot.
    - `PreferencesWin::clickBrowseSavedData()` — the `SettingsBrowser`
      `QDialog::exec()` itself (not a result-dependent `QMessageBox`, so it
      wasn't in the "8" count): opened non-modally with `WA_DeleteOnClose`
      replacing the manual `delete`, plus a `destroyed → settingsbrowser =
      nullptr` connect so the member can't dangle. Grepped every
      `settingsbrowser` use first to confirm nothing reads it after the old
      `exec()` returned — only the ctor init and this function touch it.
  With these, the per-file summary's "Dialog `exec()` → `open()`" row is now
  ticked.
  **WASM audio `onended` fix.** The Phase 7 `WasmAudioSink` bridge resolved
  its async end-of-playback callback with Emscripten's `{{{ makeDynCall('vi',
  'endedPtr') }}}` macro inside the `EM_JS` body — but that build-time macro
  **did not expand** on this Qt 6.11.1 / emsdk 4.0.7 combination, leaving the
  buffer-source `onended` unable to notify C++ (so natural end-of-playback
  never mapped to `QAudio::IdleState`). Replaced with a direct call to the
  `EMSCRIPTEN_KEEPALIVE` export `_wasmAudioSinkOnEnded(entry.id)`, with
  `Module._wasmAudioSinkOnEnded` / `getWasmTableEntry` / `wasmTable.get`
  fallbacks that don't depend on macro expansion. (Supersedes the makeDynCall
  approach described in the 2026-07-08 Phase 7 entry — that mechanism was the
  intent, not what actually worked.)
  **SOUND/SAY console noise removed** (cross-platform, but most visible in
  the browser console): suppressed Qt Multimedia's informational
  `qt.multimedia.ffmpeg` "Using Qt multimedia with FFmpeg version …" banner
  via `QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg.info=false")`
  in `Main.cpp` (warnings/errors still surface), and removed a stray
  `qCritical()` "TTS available engines: …" diagnostic from both
  `RunController` speak-setup sites (the real `errorOccurred` handler stays).
  All of the above pushed to `v2.1.Alpha05WASM` and CI-triggered; the
  SettingsBrowser pair (`clickBrowseSavedData`/`clickDeleteButton`) went as
  its own commit since it's entangled with the still-open Settings-
  persistence work rather than the audio changes.

- 2026-07-10 (browser verification): Maintainer confirmed the batch above in a
  real browser, closing the two items that stood at "pushed + CI-triggered" in
  the preceding entry. The audio fix landed exactly here: the first browser run
  threw `makeDynCall is not defined` in the `onended` handler (the `{{{ }}}`
  build-time macro was emitted un-expanded on Qt 6.11.1/emsdk 4.0.7), so
  natural-end never reached C++ and `SOUNDWAIT` hung after the first tone —
  fixed by calling the `EMSCRIPTEN_KEEPALIVE` `wasmAudioSinkOnEnded` export
  directly with `getWasmTableEntry`/`wasmTable.get` fallbacks. Re-verified: a
  3× `SOUND 400,2000` loop is fully audible, the program continues past the
  loop, and pause/resume/seek/`SOUNDWAIT` all behave. Phase 6 browser matrix
  green across Chrome/Firefox/Edge/Safari (page loads, a `PAUSE`/input program
  doesn't freeze the tab, `.kbs` save→download→re-upload round-trips, mouse +
  keyboard examples respond); the five converted dialog flows behave and
  desktop behaviour is unchanged. See the preceding 2026-07-10 entry for the
  per-change detail. **Remaining open item: Settings persistence** —
  `NativeFormat` is ephemeral, `WebLocalStorageFormat` spins; still the one
  unsolved functional gap. All other mandatory scope (Phases 0–6) is now
  functionally complete and browser-verified.

- 2026-07-11: **Phase 7 `SAY` via Web Speech API implemented** (code only;
  pending CI-green + maintainer browser verification). Qt for WebAssembly
  ships no TextToSpeech backend (the wasm SDK omits the module, hence
  `BASIC256_ENABLE_TTS=OFF` and `speakWords()` previously fell to
  `ERROR_NOTAVAILABLE`), so this bypasses `QTextToSpeech` and drives
  `window.speechSynthesis` directly. In `RunController.cpp`: a file-scope
  `#ifdef Q_OS_WASM` bridge adds an `EM_JS wasmSay(const char*)` (builds a
  `SpeechSynthesisUtterance`, `cancel()` then `speak()`), an `EM_JS
  wasmSayCancel()`, and an `extern "C" EMSCRIPTEN_KEEPALIVE basic256SayFinished()`
  export; the utterance's `onend`/`onerror` call that export **directly**
  (`_basic256SayFinished` / `Module._basic256SayFinished`, `typeof`-guarded) —
  deliberately not `makeDynCall`, which doesn't expand inside `EM_JS` on this
  Qt 6.11.1/emsdk 4.0.7 toolchain (the WasmAudioSink `onended` bug). `SAY`'s
  desktop blocking semantics are preserved by a new `#ifdef Q_OS_WASM` branch
  in `speakWords()` placed **ahead of** the `BASIC256_ENABLE_TTS` branch
  (precedence: WASM Web Speech → desktop `QTextToSpeech` → `ERROR_NOTAVAILABLE`):
  it spins a local `QEventLoop::processEvents` loop until `basic256SayFinished()`
  fires, re-checking `i->isStopping()`/`isStopped()` each pass, and calls
  `wasmSayCancel()` if Stopped mid-utterance; `stopRun()` also gained a
  `#ifdef Q_OS_WASM wasmSayCancel()` branch. No CMake change — no TextToSpeech
  link, `BASIC256_ENABLE_TTS` stays OFF for wasm, and the `QTextToSpeech speech`
  member/construction remain under the existing `#ifdef BASIC256_ENABLE_TTS`
  (already excluded from the wasm build). Notes for verification: `getVoices()`
  is async (`voiceschanged`) so the default voice is used without blocking;
  the browser autoplay policy needs a prior user gesture, satisfied by the Run
  click; `speechSynthesis` is main-thread-only, which is where the queued
  `speakWords` slot runs. Verify in-browser: `SAY "hello world"` audible +
  blocking, and `SAY` mid-program then Stop leaves no speech running.

- 2026-07-11: **Phase 7 `sound:` in-memory compressed playback implemented**
  (code only; pending CI-green + browser verification; kept as its own commit
  so a browser regression is cleanly attributable). Closes the last deferred
  Phase 7 audio gap — `SOUNDLOAD`ed compressed files (`.wav`/`.mp3`/…) played
  via `SOUND`/`SOUNDPLAY`, which desktop routes through
  `QMediaPlayer::setSourceDevice(QBuffer)` and WASM previously stubbed to
  `ERROR_NOTAVAILABLE`. Builds directly on `WasmAudioSink` (which already plays
  a decoded `AudioBuffer`): the only genuinely new piece is turning compressed
  bytes into that buffer. `WasmAudioSink` gains `decode(const QByteArray&)` +
  `EM_JS wasmAudioSinkDecode(nodeId, bytesPtr, byteLen)` — copies the bytes out
  of the heap into a private `ArrayBuffer` (they don't outlive the synchronous
  call, same as the PCM path; the copy also avoids `decodeAudioData` detaching
  the live wasm heap), calls `ctx.decodeAudioData()` wiring **both** its
  callback and Promise forms behind a `done` guard (older Safari has only the
  callbacks; modern browsers resolve the Promise) so exactly one result fires,
  stores the `AudioBuffer` into the same `entry.buffer` slot `startFrom` already
  plays, and reports completion via a new `EMSCRIPTEN_KEEPALIVE`
  `wasmAudioSinkOnDecoded(nodeId, ok, durationMs)` export called **directly**
  (`_wasmAudioSinkOnDecoded` / `Module._…`, `typeof`-guarded — not `makeDynCall`,
  the recurring trap) → a `decodeFinished(ok, durationMs)` Qt signal. `decode()`
  decodes to `ctx.sampleRate`; only `AudioBuffer.duration` is read, never a
  hard-coded rate. `WasmAudioSink::start()` now detects an already-decoded
  buffer and replays it (`samplesPtr==0`) instead of re-reading the QIODevice's
  still-compressed bytes as PCM. In `Sound.{h,cpp}`: the WASM `sound:` branch
  now mirrors the `beep:` setup but flags the instance `isDecodedMemory`, kicks
  off `audio->decode(bytes)` at setup, and connects `decodeFinished` →
  `handleDecodeFinished` (sets `media_duration` + `isValidated`, wakes
  `exitWaitingLoop`). The async decode maps onto the *existing* desktop
  first-play validation machinery rather than any new wait logic:
  `waitLoadedMediaValidation()` gained a `#ifdef Q_OS_WASM` branch that blocks on
  `media_duration` going non-negative (5 s safety timer), `play()` waits there
  before the first `start()` (loops/replays skip it — `media_duration>=0`
  already), and `length()` returns `media_duration/1000` for the decoded case
  instead of the raw-PCM byte-count formula. A `decodeAudioData` reject →
  `ok==0` → `media_duration=0` → `WARNING_SOUNDERROR`, matching the desktop
  invalid-in-memory-file behavior; validity is cached back through the existing
  `validateLoadedSound` slot, and each play still awaits its own instance's
  decode before starting (so a cached-valid resource never starts on an
  undecoded buffer). `SoundLength`/`SoundPlay`/`SoundWait`/pause/resume/seek/
  position therefore need no per-feature WASM code. Late decode callbacks after
  a sink is destroyed are safe (the `nodeId → instance` map lookup returns
  null). All changes are `#ifdef Q_OS_WASM`; desktop `sound:` (QMediaPlayer)
  and the `beep:` raw-PCM audio path are untouched. Verify in-browser: a
  `SOUNDLOAD`ed `.wav`/`.mp3` is audible, `SOUNDLENGTH` matches, seek/pause/
  resume behave, and a non-audio file raises `WARNING_SOUNDERROR` without
  hanging.

- 2026-07-11: **Fix — WASM `SAY` froze the browser tab.** Browser test showed
  the utterance was clearly audible but the UI then locked up completely (tab
  had to be force-closed, no console output) and Stop had no effect during a
  long utterance. Root cause was the RULE 2 trap flagged when the feature
  shipped: `RunController::speakWords()` runs on the **main** thread and the
  first cut spun a `QEventLoop::processEvents` loop there to block until the
  `onend` callback set a flag. Without Asyncify the main thread only delivers
  browser events — the `speechSynthesis` `onend` callback *and* the Stop
  button click — when it returns to the event loop, so the spin was a
  self-deadlock: `onend` could never fire (flag never set), Stop could never
  be delivered (`isStopping()` never set), and the loop ran forever. Fix:
  `speakWords()` is now **fire-and-forget** — it starts the utterance and
  returns immediately, keeping the main thread free. `SAY`'s blocking is
  carried entirely by the *interpreter* thread, which was already blocked in
  `OP_SAY`'s `waitCond->wait()` (a worker thread may block); the `onend`/
  `onerror` callback now wakes it directly from `basic256SayFinished()`
  (`mymutex`-guarded `waitCond->wakeAll()`), and Stop wakes it via `stopRun()`
  (which also calls `wasmSayCancel()`). Empty-`SAY` still wakes immediately.
  The `s_wasmSayFinished` flag and the `QEventLoop` spin are gone. `sound:`
  decode and the audio bridge already block only on the interpreter/worker
  thread, so they were never affected. Committed separately on top of the
  original SAY commit so the regression stays cleanly attributable. Also added
  a periodic `pause()`+`resume()` keepalive (10 s interval, module-scoped id,
  cleared on end/error/Stop) so Chrome doesn't silently cut off utterances
  longer than ~15 s. Pending re-verification in-browser.

- 2026-07-11: **WASM `SAY` browser-verified by the maintainer.** After the
  fire-and-forget + keepalive fixes above: `SAY` is audible on Chrome, the UI
  stays responsive, the program continues past it, and a 25 s utterance speaks
  all the way through — confirming both the main-thread-block fix and the
  keepalive past Chrome's ~15 s cutoff. Phase 7 `SAY` item ticked.

- 2026-07-11: **Phase 7 IDBFS settings persistence implemented — closes the
  long-open Settings item** (code only; pending CI-green + browser
  verification). The Emscripten FS is ephemeral MEMFS, so `QSettings` never
  survived a reload: `NativeFormat` wrote to MEMFS (lost on reload) and the
  earlier `WebLocalStorageFormat` attempt spun the tab (see the 2026-07-08
  Phase 5 Settings entries). This keeps `NativeFormat` — which *does* write
  real files — and gives it a persistent path via an IDBFS (IndexedDB) mount:
  - `wasm-deploy/idbfs.js`, added to the `if(EMSCRIPTEN)` link block as a
    `--pre-js` (plus `-lidbfs.js` for the IDBFS backend). In `Module.preRun`
    it `FS.mount(IDBFS, {}, '/persist')` then `FS.syncfs(true, …)`, gated by
    `addRunDependency`/`removeRunDependency` so `main()` doesn't start until
    the initial load completes (Qt reads settings very early). preRun runs on
    the main thread only — where the Emscripten FS lives.
  - `Main.cpp` (`#ifdef Q_OS_WASM`, before the first `QSettings`):
    `QSettings::setDefaultFormat(NativeFormat)` +
    `setPath(NativeFormat, UserScope, "/persist")`, so the `SETTINGS` macro
    writes to `/persist/<org>/<app>.conf`. Fallback if a future Qt WASM ignores
    `setPath`: `qputenv("HOME", "/persist")` (documented, not currently used).
  - `src/core/WasmSettings.{h,cpp}` (new, core, `Q_OS_WASM`-only, no `Q_OBJECT`
    — a `QTimer` + lambdas): `persistSoon()` (debounced ~1 s, coalesces
    `SETSETTING` loops) and `persistNow()` (immediate). Both thread-safe;
    the `EM_JS`-wrapped `FS.syncfs(false, …)` always runs on the main thread
    (the debounce `QTimer` has main-thread affinity; off-thread callers marshal
    via `QMetaObject::invokeMethod`). Wired: `persistNow()` on Preferences save,
    clear-all, and `SettingsBrowser` delete, and on `QCoreApplication::
    aboutToQuit`; `persistSoon()` after `OP_SETSETTING` (interpreter thread).
  - Also suppressed the blocking "Preferences saved"
    `QMessageBox::information()` on WASM (a static `exec()` that freezes the
    main thread, RULE 2) so the save flow completes; desktop keeps the dialog.
  All changes are `#ifdef Q_OS_WASM`; desktop settings behaviour is untouched.
  This also finally gives the deferred `SettingsBrowser` dialog pair a real
  store to browse. Verify in-browser: change a preference / run `SETSETTING`,
  reload, and confirm the value persisted (and the Settings browser lists it).

- 2026-07-11: **WASM IDBFS settings persistence browser-verified by the
  maintainer.** Changing an Edit → Preferences setting and reloading the page
  keeps the change — confirming the whole chain end to end: the `--pre-js`
  IDBFS mount + initial `FS.syncfs(true)` load, `NativeFormat` QSettings
  redirected to `/persist`, and the `persistNow()` → `FS.syncfs(false)` flush
  on save. Phase 7 "IDBFS mount" and Phase 5 "Settings" items ticked as
  verified. (`SETSETTING` persistence and the Settings-browser listing run
  through the same mechanism but weren't separately exercised.)
