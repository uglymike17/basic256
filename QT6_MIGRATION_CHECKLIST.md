# BASIC256 – Qt6 Migration Checklist (Claude Code execution plan)

**Status of the codebase:** the build system is already Qt6-aware
(`find_package(QT NAMES Qt6 Qt5 ...)` with the full component set), and the
scattered API deprecations have already been cleared. What remains is
**concentrated in QtMultimedia and QRegExp**, plus two minor cleanups. Until
QtMultimedia is converted, the project **will not compile under Qt6**.

This file is the source of truth for the migration. Work top to bottom.
Tick each `[ ]` box as it lands and **do not skip the build gate at the end of
each phase.**

---

## How to drive this with Claude Code

- One **phase per session** where possible. Phases are ordered by risk and by
  dependency; don't start a later phase until the earlier one builds green.
- At the **start of a session**, tell Claude Code: *"Read
  `QT6_MIGRATION_CHECKLIST.md`, continue from the first unticked box, make only
  the edits described, then run the build gate for that phase."*
- At the **end of a session**, have it tick the boxes it completed and write a
  one-line note under **Session log** at the bottom.
- **Never let it invent API calls.** Every change in this file is pinned to an
  exact line and an exact replacement. If reality differs from a line number
  (files drift), match on the *code string*, not the number.

---

## Three global rules (apply to every phase)

**RULE 1 – Convert every string-based `connect()` to the functor form.**
Qt6 keeps `SIGNAL()/SLOT()` string connects, but if a signal was *renamed*
(and several multimedia signals were), the string connect **compiles fine and
silently never fires**. This is the exact class of dead-signal bug already hit
in `Sound.cpp`. So convert:

```cpp
// OLD (string-based, fails silently on renamed signals):
connect(obj, SIGNAL(someSignal(int)), this, SLOT(someSlot(int)));
// NEW (functor-based, compiler catches renames):
connect(obj, &SomeClass::someSignal, this, &ThisClass::someSlot);
```

**RULE 2 – In Qt6 every `QMediaPlayer` needs a `QAudioOutput` or it is silent.**
`QMediaPlayer` no longer has its own audio output or `setVolume()`. You must
create a `QAudioOutput`, attach it with `player->setAudioOutput(audioOut)`, and
set volume **on the QAudioOutput** as a **float in 0.0–1.0** (not int 0–100).
A player with no audio output compiles and plays *nothing*. This is the #1
migration trap – verify audibility by ear, not just by "it built."

**RULE 3 – Build gate after every phase.** Configure against Qt6 explicitly and
build. Do not proceed on warnings-as-only-signal; a clean Qt6 configure+build is
the pass condition for the phase.

```bash
cmake -S . -B build-qt6 -DCMAKE_PREFIX_PATH="$QT6_DIR" -DQT_VERSION_MAJOR=6
cmake --build build-qt6 -j
```

> Note (this repo): Rule 3's automated build gate is being run manually by the
> maintainer, not by Claude Code, per session instruction. Edits below were
> made and verified by code-string inspection; the Qt6 configure+build itself
> has not yet been executed by Claude Code.

---

## PHASE 1 – QtMultimedia (blocking; do first)

Four files: `BasicMediaPlayer.h`, `BasicMediaPlayer.cpp`, `Sound.h`, `Sound.cpp`.
This is the only phase that currently stops the Qt6 build.

### 1A. `BasicMediaPlayer.h`

- [x] Add include near the other Qt includes (after `#include <QMediaPlayer>`):
  ```cpp
  #include <QAudioOutput>
  ```
- [x] Add a private member (audio sink for the player – Rule 2):
  ```cpp
  QAudioOutput *audioOutput;
  ```
- [x] Change the `waitForState` declaration type
  `QMediaPlayer::State` → `QMediaPlayer::PlaybackState`:
  ```cpp
  // OLD:  void waitForState(QMediaPlayer::State, int);
  // NEW:  void waitForState(QMediaPlayer::PlaybackState, int);
  ```

### 1B. `BasicMediaPlayer.cpp`

- [x] **Constructor** – attach an audio output (Rule 2).
- [x] `loadFile()` – both calls: `setMedia(` → `setSource(`.
- [x] `waitForSeekable()` – the error-signal connect converted to functor
  (`errorOccurred`).
- [x] `waitForState()` signature + body converted to `PlaybackState`; the
  malformed `timeout()`→`SIGNAL(QMediaPlayer::stateChanged())` connect
  replaced with a valid functor connect to `playbackStateChanged`.
- [x] `state()`: `QMediaPlayer::state()` → `QMediaPlayer::playbackState()`.
  Enumerators left unchanged (`PlayingState`, `StoppedState`, `PausedState`).
- [x] `error()`: confirmed unchanged, compiles as-is.

### 1C. `Sound.h`

- [x] Replaced the audio include and added the device includes
  (`QAudioSink`, `QAudioDevice`, `QMediaDevices`, kept `QAudioOutput`).
- [x] Changed the low-level sink member type: `QAudioOutput* audio;` →
  `QAudioSink* audio;`.
- [x] Added a `QAudioOutput* mediaAudioOut;` member for the `QMediaPlayer`
  file-playback path (Rule 2).
- [x] *(Gap found during implementation, not in original per-line list)*
  Changed `handleMediaStateChanged(QMediaPlayer::State)` →
  `handleMediaStateChanged(QMediaPlayer::PlaybackState)` — required because
  `prepareConnections()` now connects it via functor to the renamed
  `playbackStateChanged` signal, which requires an exact type match.

### 1D. `Sound.cpp`

- [x] **Format setup**: the `setSampleSize`/`setCodec`/`setSampleType` triad
  replaced with `format.setSampleFormat(QAudioFormat::Int16);`.
- [x] **Device query**: `QAudioDeviceInfo` → `QAudioDevice` / `QMediaDevices`;
  `nearestFormat()` → `preferredFormat()`.
- [x] **Low-level sink construction** (both occurrences, `beep:` playback and
  generated-sound playback): `new QAudioOutput(format, ...)` →
  `new QAudioSink(format, ...)`.
- [x] **QMediaPlayer file path** – attached a `QAudioOutput` (Rule 2) at all
  **three** `new QMediaPlayer(...)` construction sites: the memory (`sound:`)
  path, the file path, and the web/http path. *(Original checklist only
  listed the memory path; the other two were a gap — without this fix, file
  and web sound playback would silently produce no audio under Qt6.)*
- [x] Set source from the buffer: `setMedia(QMediaContent(), buffer)` →
  `setSourceDevice(buffer)`.
- [x] *(Gap found during implementation)* `setMedia(QUrl::fromLocalFile(...))`
  and `setMedia(QUrl::fromUserInput(...))` → `setSource(...)` at the file-path
  and web-path construction sites. *(Not in the original per-line list; Qt6
  removes the `setMedia(QUrl)` overload entirely, so leaving these would have
  been a hard compile error, contradicting Phase 1's own build gate.)*
- [x] Clear-source calls (both occurrences): `media->setMedia(QMediaContent())`
  → `media->setSourceDevice(nullptr)`.
- [x] Player volume (both occurrences, `updatedMasterVolume` and the fade
  timer): moved to `mediaAudioOut->setVolume(...)`, float 0.0–1.0.
- [x] Player state read (the seek wait-loop condition): `media->state()` →
  `media->playbackState()`.
- [x] `media->error()` (in `lastError()`): confirmed unchanged, compiles as-is.
- [x] *(Gap found during implementation — Rule 1)* Converted the remaining
  string-based connects to the renamed `QMediaPlayer` signals, everywhere
  they appear in this file, not just the one location originally called out:
  - Three `SIGNAL(stateChanged(QMediaPlayer::State))` waits (in `seek()`,
    `waitLastMediaSeekTakeAction()`, `length()`) → functor connects to
    `&QMediaPlayer::playbackStateChanged`.
  - `prepareConnections()`'s `stateChanged` connect driving
    `handleMediaStateChanged` → functor connect to `playbackStateChanged`.
  - `prepareConnections()`'s `error` connect driving `handleMediaError` →
    functor connect to `&QMediaPlayer::errorOccurred`.
  - Left untouched (per checklist guidance): `timeout()`, `seekableChanged(bool)`,
    `durationChanged(qint64)`, `positionChanged(qint64)`,
    `mediaStatusChanged(...)` connects — these signals were not renamed in Qt6.

### Phase 1 gate
- [ ] Configure + build against **Qt6** – must compile clean. *(Not run by
      Claude Code this session — maintainer is running the build manually.)*
- [ ] Run a `.kbs` that uses **BEEP / waveforms** (the `QAudioSink` path) and
      one that **plays a sound file** (the `QMediaPlayer` path). **Confirm both
      are audible** – Rule 2 failures are silent, not compile errors.

---

## PHASE 2 – QRegExp → QRegularExpression

`QRegExp` is not in Qt6 core (only in the `Qt6::Core5Compat` shim). Migrate it.
8 files use it. Semantics differ: `indexIn()/matchedLength()` become
`match()` returning a `QRegularExpressionMatch`; the greedy/minimal flag
(`regexMinimal`, `Interpreter.h:301`) maps to
`QRegularExpression::InvertedGreedinessOption`.

> **Fast-unblock option (optional):** if you want a green Qt6 build *before*
> doing this properly, add `Core5Compat` to the `find_package` COMPONENTS and
> link `Qt6::Core5Compat`, and `QRegExp` keeps working unchanged. Treat that as
> a temporary crutch, not the finish line – WASM/long-term wants it gone.

Per-file (each is `#include <QRegularExpression>` + convert usage):

- [ ] **`EditSyntaxHighlighter.h` (line ~38)** – member type:
      `QRegExp pattern;` → `QRegularExpression pattern;`
- [ ] **`EditSyntaxHighlighter.cpp` (line ~38)** – this is the hot loop.
      Convert the `indexIn`/`matchedLength` walk to a
      `QRegularExpressionMatchIterator` (`globalMatch`) or a
      `match()` + `capturedStart()/capturedLength()` loop. **Test highlighting
      visually** – this is the most behavior-sensitive conversion.
- [ ] **`BasicEdit.cpp`** – many `line.contains(QRegExp("...", Qt::CaseInsensitive))`
      (auto-indent block ~288–357) and the `indexOf/lastIndexOf(QRegExp(...))`
      at ~498–499, plus `program.split(QRegExp("\\n"))` at ~288.
      - `str.contains(QRegExp(pat, Qt::CaseInsensitive))` →
        `str.contains(QRegularExpression(pat, QRegularExpression::CaseInsensitiveOption))`
      - `str.split(QRegExp("\\n"))` → `str.split(QRegularExpression("\\n"))`
      - `indexOf(QRegExp(pat), from)` / `lastIndexOf(QRegExp(pat))` →
        same call with `QRegularExpression(pat)`.
      **Test auto-indent** on a program covering for/next/if/while/function/case.
- [ ] **`Interpreter.h` (line ~301)** – the `regexMinimal` flag comment; find the
      matching use in `Interpreter.cpp` and map `setMinimal(true)` →
      `QRegularExpression::InvertedGreedinessOption`.
- [ ] **`Interpreter.cpp`** – convert its `QRegExp` uses; apply the minimal→
      InvertedGreediness mapping where `regexMinimal` is honored.
- [ ] **`BasicOutput.cpp`** – convert its `QRegExp` use(s).
- [ ] **`Convert.cpp`** – convert its `QRegExp` use(s).
- [ ] **`MainWindow.cpp` (line ~950)** – version-string check
      `QRegExp rx("\\d+\\.\\d+\\.\\d+\\.\\d+")`. Convert to `QRegularExpression`
      and use `match().hasMatch()` instead of `indexIn() != -1`.

### Phase 2 gate
- [ ] Qt6 build clean (and, if you didn't use the Core5Compat crutch, **no**
      `Core5Compat` in the link line).
- [ ] Syntax highlighting correct; auto-indent correct; version check works.

---

## PHASE 3 – Minor cleanups (deprecation, low risk)

- [ ] **`VariableWin.h` (lines ~72–78)** – `QVariant::type()` and the
      `QVariant::Int / UInt / LongLong / ULongLong / Double` comparisons are
      deprecated in Qt6. Replace `a.type() == QVariant::Int` style checks with
      `a.typeId() == QMetaType::Int` (and `QMetaType::UInt`, `LongLong`,
      `ULongLong`, `Double`). Compiles either way, but clears the warnings.
- [ ] **`BasicEdit.cpp` (~769) / `LineNumberArea.cpp` (~69)** – `QWheelEvent`
      handlers. **Verify** they use `event->angleDelta().y()` and **not**
      `event->delta()` (removed in Qt6). The scan found no `.delta()` calls, so
      this is likely already fine – just confirm and tick.

### Already-clear (no action – recorded so nobody redoes them)
- `QFontMetrics::width()` – already `#if QT_VERSION`-guarded with
  `horizontalAdvance()` in the Qt6 branch (`Interpreter.cpp` ~5053/6226).
- `endl`/`flush` occurrences are all `std::endl`/`std::flush` – **not** Qt
  streams, no change.
- `QProcess::splitCommand()` exists in Qt6 – fine.

---

## PHASE 4 – Make Qt6 the default & lock it in CI

- [ ] Flip the CI matrix to build **Qt6** on all four targets
      (Linux x86, Linux ARM/RPi, Windows, macOS).
- [ ] Run the **`TestSuite`** as a required gate on the Qt6 build – this is the
      defense against Rule 1 silent-connect regressions that don't show at
      compile time.
- [ ] Once all four are green on Qt6, drop the Qt5 fallback: change
      `find_package(QT NAMES Qt6 Qt5 ...)` → `find_package(QT NAMES Qt6 ...)`
      (or hard-require Qt6). Remove any `Core5Compat` crutch if Phase 2 was
      finished properly.
- [ ] Update `COMPILING.txt` / `COMPILING_RaspberryPI.txt` to state Qt6 minimum.

---

## Per-file summary (tick when the file is fully done)

- [x] `BasicMediaPlayer.h`
- [x] `BasicMediaPlayer.cpp`
- [x] `Sound.h`
- [x] `Sound.cpp`
- [ ] `EditSyntaxHighlighter.h`
- [ ] `EditSyntaxHighlighter.cpp`
- [ ] `BasicEdit.cpp`
- [ ] `Interpreter.h`
- [ ] `Interpreter.cpp`
- [ ] `BasicOutput.cpp`
- [ ] `Convert.cpp`
- [ ] `MainWindow.cpp`
- [ ] `VariableWin.h`
- [ ] `LineNumberArea.cpp` (verify only)
- [ ] `CMakeLists.txt` (Phase 4: drop Qt5 fallback)
- [ ] CI workflow(s) (Phase 4: Qt6 matrix + TestSuite gate)

---

## Why this order (keep in mind for the WASM port next)

QtMultimedia is first because it's the only thing blocking a Qt6 build, and
because the WASM port will need a **different** audio backend in the browser –
so getting a clean `QAudioSink` / `QAudioOutput` seam now means the WASM variant
swaps the backend instead of forking these classes. When you design the Phase 1
edits, keep the audio setup isolated enough that a `#ifdef` (or a small backend
class) can later route to Web Audio for the Emscripten build.

---

## Session log

2026-07-04 Phase 1 (1A–1D) done: BasicMediaPlayer.h/.cpp and Sound.h/.cpp
converted to Qt6 QtMultimedia APIs. Found and fixed three gaps not covered by
the original per-line list (see notes inline above): missing setMedia(QUrl)→
setSource conversions at the file/web playback sites, missing QAudioOutput
attachment on two of the three QMediaPlayer construction sites (Rule 2), and
several string-based stateChanged/error connects on renamed Qt6 signals
(Rule 1). Build gate (cmake configure+build against Qt6, and audibility check)
was intentionally left unchecked — maintainer is running the build manually
per session instruction. Phase 2 (QRegExp) next.
