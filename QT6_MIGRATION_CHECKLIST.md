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
- [x] Configure + build against **Qt6** – must compile clean. Confirmed
      2026-07-05: all four CI targets (Windows, macOS, Linux x86_64, Linux
      ARM64/RPi) build *and* package green. Run manually by the maintainer
      via the real CI logs fed back into this session across many rounds —
      see Session log below for the full trail of what broke and why.
- [ ] Run a `.kbs` that uses **BEEP / waveforms** (the `QAudioSink` path) and
      one that **plays a sound file** (the `QMediaPlayer` path). **Confirm both
      are audible** – Rule 2 failures are silent, not compile errors. *(Still
      outstanding — green CI proves it builds/packages, not that Multimedia
      is actually audible at runtime.)*

> Note: reaching an actual green Phase 1 build also required fixing several
> Qt6 breaks outside QtMultimedia that were surfacing in the same build logs
> (QAction/QShortcut header moves, QActionGroup include, QMutex::NonRecursive
> removal, Qt::Key+Modifier operator+ deletion, QPrinter page-size/orientation
> enum moves to QPageSize/QPageLayout, Qt::ItemIsTristate rename, and a
> NOMINMAX/windows.h vs QtTextToSpeech template-parsing conflict) and doing all
> of Phase 2 (QRegExp → QRegularExpression) up front, since the errors were
> interleaved in one build log. See Session log below.

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

- [x] **`EditSyntaxHighlighter.h` (line ~38)** – member type:
      `QRegExp pattern;` → `QRegularExpression pattern;`
- [x] **`EditSyntaxHighlighter.cpp` (line ~38)** – hot loop converted to a
      `QRegularExpressionMatchIterator` (`globalMatch`) walk using
      `capturedStart()/capturedLength()`. All rule-building sites converted
      (`QRegExp(pat, Qt::CaseInsensitive)` → `QRegularExpression(pat,
      QRegularExpression::CaseInsensitiveOption)`). **Still needs a visual
      test of syntax highlighting** — not run this session.
- [x] **`BasicEdit.cpp`** – all `line.contains(QRegExp(...))` in the auto-indent
      block, `program.split(QRegExp("\\n"))`, the two `indexOf/lastIndexOf`
      calls in `getCurrentWord()`, and the two `QRegExp rx(...)` +
      `indexIn`/`cap` sites (keyPressEvent auto-indent-on-Enter, and the
      save-file extension check) all converted to `QRegularExpression` +
      `match()/hasMatch()/captured()`. **Still needs a visual test of
      auto-indent** — not run this session.
- [x] **`Interpreter.h` (line ~301)** – comment updated to say
      `QRegularExpression`.
- [x] **`Interpreter.cpp`** – all 5 `QRegExp` sites (OP_MIDX, OP_INSTRX,
      OP_REPLACEX, OP_COUNTX, the explode/split block) converted to
      `QRegularExpression`; `expr.setMinimal(regexMinimal)` replaced with
      `if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);`
      at each site.
- [x] **`BasicOutput.cpp`** – `split(QRegExp(...))` converted.
- [x] **`Convert.cpp`** – `replace(QRegExp(...))` converted. Also has a
      char/QString ternary (`decimalPoint()` vs `'.'`) at line 21 assigning
      into the `QChar decimalPoint` member — see session log for the actual
      fix (the earlier `QChar('.')`-wrap attempt did not resolve it; the real
      cause is `QLocale::decimalPoint()` returning `QString` in Qt6 instead
      of `QChar` as in Qt5).
- [x] **`MainWindow.cpp` (line ~975)** – version-string check converted to
      `QRegularExpression` + `match().captured(0)`.

### Phase 2 gate
- [x] No `QRegExp` remains anywhere in the tree (verified via repo-wide grep).
- [x] `Core5Compat` dropped from `CMakeLists.txt` (both the `find_package`
      component list and the `target_link_libraries` block) and from
      `.github/scripts/build_Windows.ps1`'s `aqt install-qt -m` module list.
- [ ] Syntax highlighting correct; auto-indent correct; version check works.
      *(Not run this session — needs a maintainer pass in the actual app.)*

---

## PHASE 3 – Minor cleanups (deprecation, low risk)

- [x] **`VariableWin.h` (lines ~72–78)** – `QVariant::type()` and the
      `QVariant::Int / UInt / LongLong / ULongLong / Double` comparisons are
      deprecated in Qt6. Replace `a.type() == QVariant::Int` style checks with
      `a.typeId() == QMetaType::Int` (and `QMetaType::UInt`, `LongLong`,
      `ULongLong`, `Double`). Compiles either way, but clears the warnings.
      Done 2026-07-05 in `variantLessThan()` (both `a` and `b` branches) —
      this was also the single source of every repeated
      `QVariant::Type`/`QVariant::type` warning seen in the Phase 1 build
      logs across `BasicEdit.cpp`, `Main.cpp`, `MainWindow.cpp`,
      `PreferencesWin.cpp`, `ReplaceWin.cpp`, `RunController.cpp` — those
      all just include this header, there was nothing to fix in them
      individually (confirmed via repo-wide grep, no other
      `QVariant::Int`/`.type() == QVariant::...` usage anywhere).
- [x] **`BasicEdit.cpp` (~769) / `LineNumberArea.cpp` (~69)** – `QWheelEvent`
      handlers. **Verify** they use `event->angleDelta().y()` and **not**
      `event->delta()` (removed in Qt6). Confirmed 2026-07-05: neither
      handler calls `.delta()` at all —
      `BasicEdit::lineNumberAreaMouseWheelEvent` just forwards to
      `QPlainTextEdit::wheelEvent(event)`, and `LineNumberArea::wheelEvent`
      just forwards to that. Repo-wide grep for `QWheelEvent`/`->delta()`
      turned up nothing else. Already fine, no code change needed.

### Already-clear (no action – recorded so nobody redoes them)
- `QFontMetrics::width()` – already `#if QT_VERSION`-guarded with
  `horizontalAdvance()` in the Qt6 branch (`Interpreter.cpp` ~5053/6226).
- `endl`/`flush` occurrences are all `std::endl`/`std::flush` – **not** Qt
  streams, no change.
- `QProcess::splitCommand()` exists in Qt6 – fine.

---

## PHASE 4 – Make Qt6 the default & lock it in CI

- [x] Flip the CI matrix to build **Qt6** on all four targets
      (Linux x86, Linux ARM/RPi, Windows, macOS). Done across earlier
      sessions in this file's own history — all four build *and* package
      green as of 2026-07-05 (see Phase-1-adjacent session log entries).
- [x] Run the **`TestSuite`** as a required gate on the Qt6 build – this is the
      defense against Rule 1 silent-connect regressions that don't show at
      compile time. Done 2026-07-05 — see session log for what this
      actually runs and why (the full interactive `testsuite.kbs` cannot
      run headlessly; a new CI-only subset was added instead). **Not yet
      verified against a real CI run** — no local Qt6/BASIC-256 toolchain
      to test the new `.kbs` script or the runner scripts against.
- [x] Once all four are green on Qt6, drop the Qt5 fallback: change
      `find_package(QT NAMES Qt6 Qt5 ...)` → `find_package(QT NAMES Qt6 ...)`
      (or hard-require Qt6). Remove any `Core5Compat` crutch if Phase 2 was
      finished properly. Done 2026-07-05 — `Core5Compat` was already gone
      (confirmed via grep, nothing to remove); also simplified the now-dead
      `qt5_add_translation()` CMake branch to the unconditional Qt6 call.
- [x] Update `COMPILING.txt` / `COMPILING_RaspberryPI.txt` to state Qt6 minimum.
      Done 2026-07-05 — both files were rewritten, not just patched: they
      still described qmake/`BASIC256.pro` (deleted long ago) and, for the
      RPi file, a 2020 Debian-buster/svn/Qt5/snapcraft process. Patching in
      "Qt6 minimum" next to instructions that no longer work at all would
      have been actively misleading, so both now describe the actual
      CMake+Qt6 process, sourced directly from the CI scripts.

---

## Per-file summary (tick when the file is fully done)

- [x] `BasicMediaPlayer.h`
- [x] `BasicMediaPlayer.cpp`
- [x] `Sound.h`
- [x] `Sound.cpp`
- [x] `EditSyntaxHighlighter.h`
- [x] `EditSyntaxHighlighter.cpp`
- [x] `BasicEdit.cpp`
- [x] `Interpreter.h`
- [x] `Interpreter.cpp`
- [x] `BasicOutput.cpp`
- [x] `Convert.cpp`
- [x] `MainWindow.cpp` (also: QAction/QShortcut/QActionGroup includes,
      QMutex::NonRecursive, Qt::Key+Modifier, char→QString ternary,
      QString!=NULL ambiguity — see session log)
- [x] `VariableWin.h` (Phase 3, warnings only, not blocking)
- [x] `LineNumberArea.cpp` (verify only, Phase 3 — confirmed clean, no change needed)
- [ ] `CMakeLists.txt` (Phase 4: drop Qt5 fallback — Core5Compat already
      removed this session, but the Qt5 fallback in `find_package(QT NAMES
      Qt6 Qt5 ...)` is untouched, per Phase 4 scope)
- [ ] CI workflow(s) (Phase 4: Qt6 matrix + TestSuite gate)
- [x] `PreferencesWin.cpp` (not in original per-file list — gap found this
      session: `QPrinter::A0..Tabloid` → `QPageSize::...`, `QPrinter::Portrait/
      Landscape` → `QPageLayout::...`, `Qt::ItemIsTristate` → `Qt::ItemIsAutoTristate`)
- [x] `MainWindow.h` (not in original per-file list — added
      `QtGui/QActionGroup` include)
- [x] `Sleeper.h` / `Main.cpp` / `RunController.cpp` (not in original per-file
      list — added `NOMINMAX` guards around raw `windows.h` includes; without
      it, leaked `min`/`max` macros corrupted a template in Qt's installed
      `qtexttospeech.h` later in the same AUTOMOC translation unit)

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

2026-07-04 (later same day) Maintainer ran the real Qt6 MSVC build in CI and
fed back the actual error log, which was far larger than Phase 1 alone —
it mixed genuine remaining Phase-1-adjacent gaps with all of Phase 2 and a
few items not in this checklist at all. Fixed in one pass, from the log,
top to bottom:
- Completed Phase 2 in full (see per-file list above): all 8 files migrated
  from `QRegExp` to `QRegularExpression`, including the `regexMinimal` →
  `QRegularExpression::InvertedGreedinessOption` mapping in `Interpreter.cpp`.
  Dropped the `Core5Compat` crutch from `CMakeLists.txt` and from
  `.github/scripts/build_Windows.ps1`.
- `MainWindow.h`/`.cpp`: `QAction`/`QShortcut` need `QtGui/` not `QtWidgets/`
  in Qt6 (fixed in an earlier session pass); `QActionGroup` needed its own
  `#include <QtGui/QActionGroup>` (only forward-declared via the `QAction`
  header, so `new QActionGroup(this)` and the `QActionGroup*` connect() were
  both hitting "incomplete type"). `QMutex::NonRecursive` no longer exists
  (Qt6 `QMutex` is always non-recursive; dropped the constructor arg).
  `Qt::Key_0 + int + Qt::CTRL` hit a deleted `operator+(int, Qt::Modifier)` —
  Qt6 requires `|` once the key expression has been promoted to `int`;
  rewrote as `QKeySequence(Qt::Key(...) | Qt::CTRL)`. Two char→QString
  ternaries (`cond ? locale->decimalPoint() : '.'`) needed the `'.'` literal
  wrapped as `QChar('.')`. `s != NULL` on a `QString` was ambiguous in Qt6
  (multiple candidate operators); changed to `!s.isNull()`.
- `PreferencesWin.cpp` (gap — not in original checklist): `QPrinter::A0..
  Tabloid` and `QPrinter::Portrait/Landscape` no longer exist — moved to
  `QPageSize::...` / `QPageLayout::...`. Confirmed via `Interpreter.cpp`
  (which already did `static_cast<QPageSize::PageSizeId>(settingsPrinterPaper)`)
  that the enum's underlying integer values are unchanged from the old
  `QPrinter` enums, so stored `Settings` values stay compatible. Also
  `Qt::ItemIsTristate` → `Qt::ItemIsAutoTristate` (renamed in Qt6).
- `Sleeper.h` / `Main.cpp` / `RunController.cpp` (gap — not in original
  checklist): added `#define NOMINMAX` guards before their raw
  `#include <windows.h>`. Without it, `windows.h`'s `min`/`max` macros leak
  into the shared `mocs_compilation_Release.cpp` AUTOMOC translation unit and
  corrupt a fold-expression template in Qt's own installed
  `QtTextToSpeech/qtexttospeech.h` (`LastIndexOf<T, tuple<...>>`), producing a
  wall of MSVC C2589/C3878/C2760 parser errors that have nothing to do with
  our code. This is a best-effort fix based on the well-known
  windows.h-macro-pollution failure mode for this class of MSVC error —
  **not verified against an actual Qt6 build this session**, since Claude
  Code has no local Qt6/MSVC toolchain; flag it back if the real CI log still
  shows the same qtexttospeech.h errors after this fix.
- Everything above was verified by code-string/grep inspection only (repo-wide
  `grep -rl QRegExp` returns empty; no bare `min(`/`max(` calls found that
  would break under `NOMINMAX`). The Qt6 configure+build itself was **not**
  run by Claude Code — maintainer runs it per standing session instruction.
  Phase 3 (VariableWin.h QVariant::type warnings, LineNumberArea.cpp wheel
  event verification) is next, then Phase 4.

2026-07-04 (later still) Maintainer fed back a second real Qt6 MSVC build log.
Everything from the previous pass compiled clean; the build now fails only at
`Convert.cpp(21,2)`: `QChar &QChar::operator=(QString)` — no viable
conversion. Root cause: `decimalPoint` (`Convert.h:68`) is declared `QChar`,
and the ternary `replaceDecimalPoint ? locale->decimalPoint() : QChar('.')`
mixes a `QString` (Qt6's `QLocale::decimalPoint()` return type) with a
`QChar`; the previous session's fix wrapped the *literal* (`QChar('.')`),
which was already the literal's type and did nothing for the actual mismatch
on the other branch. Since the project's `CMakeLists.txt` still falls back to
Qt5 (`find_package(QT NAMES Qt6 Qt5 ...)`, Phase 4 not yet done), and Qt5's
`QLocale::decimalPoint()` returns `QChar` (no `.at()`), the fix needs to
compile under both: wrapped the call as `QString(locale->decimalPoint()).at(0)`
— `QString(QChar)` (Qt5) and `QString(QString)` (Qt6) both work, and `.at(0)`
yields a `QChar` on either branch so the ternary and the assignment into the
`QChar` member both type-check. Not yet re-verified against an actual build
by Claude Code — maintainer to re-run CI.

2026-07-05 Windows build itself went green. Two new failures surfaced,
both packaging/CI-script issues rather than app-code issues:

1. **Windows installer (`build_installer_Windows.ps1` / `BASIC256.nsi`)** —
   `windeployqt.exe` path and Qt DLL names in `package_Windows.ps1` still
   read `$env:Qt5_Dir` / `Qt5*.dll` even though `build_Windows.ps1` exports
   `QT_DIR` (fixed earlier this session) — renamed throughout. Separately,
   `BASIC256.nsi` failed at `File "${SDK_PLUGINS}\audio\qtaudio_windows.dll"`
   — Qt6's Multimedia plugin architecture doesn't have an `audio` plugin
   category (or `mediaservice`/`playlistformats`) at all, so the entire
   hand-curated Qt5 plugin/DLL file list in the `.nsi` was structurally
   wrong, not just misnamed. Fixed by deleting that list entirely and
   instead recursively pulling `Basic256\*.*` (excluding Examples/TestSuite/
   Translations/basic256.bat/the exe/README/png, which are handled
   separately) — that folder is already correctly populated by
   `windeployqt` in the preceding `package_windows.ps1` step, so this needs
   no Qt6 plugin-name knowledge at all. Also widened the uninstaller's
   `RMDir /r $INSTDIR\<plugin-dir>` list with plausible Qt6 categories
   (`multimedia`, `generic`, `styles`, `iconengines`, `networkinformation`,
   `tls`) alongside the old Qt5 ones — harmless no-ops if a given folder
   doesn't exist.
2. **Linux/macOS/RPi builds never migrated off Qt5** — `build_Linux_x86.sh`,
   `build_Linux_RPi_Trixie.sh`, and `build_macos.sh` were still installing
   Qt5 (`qtbase5-dev`, `libqt5*`, Homebrew `qt@5`), so `BasicDock.h`'s
   `#include <QtGui/QAction>` (a real Qt6-only header location) failed with
   "No such file or directory" on all three, since only Qt5 was ever
   installed. Switched apt package lists to Qt6 equivalents
   (`qt6-base-dev`, `qt6-multimedia-dev`, `qt6-serialport-dev`,
   `qt6-speech-dev`, `qt6-l10n-tools`, etc. -- **not verified against the
   actual apt indices on jammy/Trixie, best-effort naming**) and switched
   macOS to `brew install qt` (Homebrew's unversioned formula is Qt6 now;
   `qt@5` is gone). Also renamed the Qt5->Qt6 library/plugin-dir references
   inside `build_Linux_RPi_Trixie.sh`'s post-build packaging block
   (`libQt5*.so.*` -> `libQt6*.so.*`, `qt5/plugins` -> `qt6/plugins`,
   `mediaservice` -> `multimedia`, made each copy non-fatal) since that
   script conflates build+partial-packaging and would have hard-failed
   immediately after compiling successfully otherwise.
   **Deliberately NOT touched**: `package_Linux_x86.sh`,
   `package_Linux_x86_AppImage.sh`, `package_Linux_RPi_Trixie.sh`,
   `package_Linux_RPi_AppImage.sh`, `package_macos.sh` — these are separate
   packaging-stage scripts (run only after a successful build) that are
   *also* still fully Qt5-hardcoded (`libQt5MultimediaGstTools.so`,
   `mediaservice` plugin dirs, `qt5/plugins` paths, GStreamer bridge lib
   that may not exist under Qt6's default FFmpeg-based Multimedia backend
   at all). Per this checklist's own rule, didn't want to guess blind across
   5 more files with real runtime-library-name uncertainty (especially the
   GStreamer↔Qt6Multimedia bridge) — wait for the real log once the build
   stage is green on each platform, then fix packaging from that.

2026-07-05 (later) Real CI logs for the two apt-based Linux jobs came back;
both were genuine, verifiable (via `packages.ubuntu.com`/`packages.debian.org`
web search, not guesswork) issues, not just naming drift:
- **Ubuntu 22.04 (x86_64) build**: `apt install` failed outright —
  `qt6-serialport-dev` and `qt6-speech-dev` don't exist in jammy at all
  (jammy's serialport package is `libqt6serialport6-dev`, and jammy never
  got a Qt6TextToSpeech package — that only appears from Ubuntu 24.04
  "noble" onward). Since this project hard-requires `TextToSpeech`
  (`CMakeLists.txt:20-31`), apt is a dead end on jammy. Switched
  `build_Linux_x86.sh` to install Qt6 via `aqtinstall` instead (mirrors
  `build_Windows.ps1` exactly: `aqt install-qt linux desktop 6.7.3 gcc_64 -m
  qtmultimedia qtserialport qtspeech`), exporting `QT_DIR` via `$GITHUB_ENV`
  the same way. Kept apt for genuinely non-Qt deps (gstreamer, pulse,
  speech-dispatcher, espeak-ng, X11/mesa dev headers) and added
  `libxcb-cursor0` (a well-known Qt6 xcb-platform-plugin runtime
  requirement since 6.5). Also updated `package_Linux_x86.sh`'s two lines
  that read `$Qt5_DIR`/a hardcoded system qt5 path to read the new `$QT_DIR`
  instead, since that script runs in the same job right after the build
  step and would otherwise definitely break next (not new guessing — same
  env var plumbing pattern as Windows). Did **not** touch
  `package_Linux_x86_AppImage.sh` (it never read `Qt5_DIR` to begin with —
  already hardcoded to a system path, already deferred above).
- **Debian Trixie (RPi ARM64) configure**: `find_package(Qt6 ...
  TextToSpeech)` failed — `Qt6TextToSpeechConfig.cmake` exists but sets
  `_FOUND` to FALSE because its own dependency, `Qt6QmlIntegration`, isn't
  installed. Confirmed via web search that `qt6-declarative-dev` (Debian
  Trixie has 6.8.2) is what provides `Qt6QmlIntegration` — added it to
  `build_Linux_RPi_Trixie.sh`'s apt list. Left the rest of that script (and
  the ARM64 apt Qt6 approach in general) as-is since Trixie's Qt6 packaging
  is otherwise complete (unlike jammy) — no need for the aqtinstall
  workaround here.
- Windows and macOS both went green this round.

2026-07-05 (later still) Two more real logs came back:
- **Ubuntu x86_64**: the aqtinstall switch itself was right, but the arch
  name was wrong — `aqt install-qt linux desktop 6.7.3 gcc_64 ...` failed
  with "packages ... were not found while parsing XML" because Qt 6.7+
  renamed the Linux x86_64 desktop arch to `linux_gcc_64` (confirmed via
  web search: this happened alongside the 6.7.0 linux/arm64 addition, which
  needed `linux_gcc_arm64` to disambiguate). Fixed the arch argument. Also
  stopped hardcoding the resulting install subdirectory name
  (`6.7.3/linux_gcc_64`) and instead `find`-discover whatever aqt actually
  created under `6.7.3/` — the Windows build showed aqt sometimes strips a
  host-redundant prefix from the arch name when naming the install dir
  (arch `win64_msvc2019_64` → dir `msvc2019_64`), so hardcoding a second
  guess on top of the first felt like asking for a third round-trip.
  Propagated the same `QT_DIR`-based fix into `package_Linux_x86_AppImage.sh`
  (`QT_PLUGIN_DIR`, `QMAKE`), which has the identical Qt5-hardcoded-path
  problem as `package_Linux_x86.sh` did last round and would have failed
  next for the same reason.
- **RPi ARM64 AppImage packaging**: `linuxdeploy-plugin-qt` failed with
  "Could not find qmake" — `package_Linux_RPi_AppImage.sh` still pointed
  `$QMAKE` at the old Qt5 path (`${QT_LIB}/qt5/bin/qmake`), which doesn't
  exist since the ARM64 build now installs Qt6 via apt. Confirmed via web
  search that Debian packages Qt6's qmake as a plain PATH binary named
  `qmake6` (not nested under an arch-specific `qt6/bin` dir the way Qt5's
  was) — switched to `command -v qmake6` instead of constructing a path.
- Did not touch the `mediaservice`/`imageformats`/etc. `EXTRA_QT_PLUGINS`
  plugin-category lists in either AppImage script — still deferred, not
  what either of these two logs actually flagged as broken.

2026-07-05 (later still) Windows, macOS, and Linux ARM64 all green. Ubuntu
x86_64 log confirmed the `find`-discovered `QT_DIR` resolved to
`6.7.3/gcc_64` (aqt does strip the `linux_` prefix from the arch name when
naming the install dir, as suspected — good thing that wasn't hardcoded).
New failure, unrelated to Qt module packaging: `Qt6PrintSupport` requires
CUPS dev headers (`find_package(Cups)` inside its own CMake config), which
aren't installed on this runner (aqt's prebuilt Qt6 doesn't bundle a private
copy of Cups the way it bundles FFmpeg/etc.). Added `libcups2-dev` to
`build_Linux_x86.sh`'s apt list (standard Ubuntu package providing
`cups/cups.h` — this one's a well-established name, not a guess). RPi ARM64
never hit this because its apt-based `qt6-base-dev` pulls Cups in
transitively as a real Debian package dependency.

2026-07-05 (later still) Build itself green on x86_64. `package_Linux_x86.sh`
failed with real, confirmable evidence (linuxdeployqt's own "plugin could
not be found" warnings against the actual aqt-installed Qt6 tree) that the
Qt5 `mediaservice`/`audio` plugin categories referenced throughout the
packaging scripts genuinely don't exist in Qt6 — confirmed via web search
that Qt6 Multimedia plugins live under a single `multimedia` plugin dir
instead (e.g. `libffmpegmediaplugin.so`). Also a real `ldd` failure:
`libxcb-icccm.so.4` missing for the `qxcb` platform plugin. Fixed in
`package_Linux_x86.sh` and `package_Linux_x86_AppImage.sh`: replaced
`mediaservice`/`audio` with `multimedia` everywhere (mkdir, copy loops,
`-extra-plugins`/`EXTRA_QT_PLUGINS` lists), and deleted the
`libQt5MultimediaGstTools.so` copy lines entirely (Qt6's default FFmpeg
Multimedia backend has no GStreamer bridge lib by that name — this was
already just a silently-swallowed `cp` error in the log, dead weight).
Added Qt's own documented xcb runtime dependency set
(`libxcb-icccm4`, `libxcb-image0`, `libxcb-keysyms1`, `libxcb-randr0`,
`libxcb-render-util0`, `libxcb-shape0`, `libxcb-xinerama0`, `libxcb-xkb1`)
to `build_Linux_x86.sh`'s apt list proactively rather than fixing them one
`ldd` failure at a time.
**Not touched**: `package_Linux_RPi_AppImage.sh` still has the identical
Qt5-named plugin categories and `libQt5*.so` copy list (`mediaservice`,
`libQt5MultimediaGstTools.so.*`, etc.) — but the maintainer reported that
step green, and since every copy in it is best-effort (`|| true`), it's
apparently producing a working-enough AppImage as-is. Left alone rather
than risk regressing something currently passing; worth revisiting with the
same `multimedia`-plugin-dir fix later since it's almost certainly
producing an AppImage with Multimedia plugins silently missing.

2026-07-05 (later still) Next `package_Linux_x86.sh` run got past the
`multimedia`/xcb fixes above and hit a new, unrelated failure:
`linuxdeployqt`'s `ldd` trace aborted the whole build on
`libqsqlmimer.so -> libmimerapi.so not found`. Root cause: aqt's official
Qt6 build bundles a `sqldrivers` plugin for every SQL backend Qt supports
(Mimer, DB2, Firebird/ibase, MySQL, ODBC, PostgreSQL, SQLite) regardless of
whether that backend's client library is installed on the build machine —
BASIC256 only ever uses SQLite (confirmed via the old Qt5-era NSIS installer
list, which only ever bundled `qsqlite.dll`). `linuxdeployqt` (and
presumably `linuxdeploy-plugin-qt`, same underlying Qt tree, same Sql
module linkage) hard-aborts on *any* plugin with an unresolved dependency,
not just the ones actually used. Fixed by deleting every `sqldrivers/*.so`
except `libqsqlite.so` from the aqt Qt tree before deploying, in both
`package_Linux_x86.sh` and (proactively, same tree/same Sql linkage,
untested but same evidenced cause) `package_Linux_x86_AppImage.sh`, rather
than allowlisting one broken driver at a time across further CI
round-trips.

2026-07-05 (later still) `package_Linux_x86_AppImage.sh` failed at an
earlier stage than the sqldrivers fix above: the base `linuxdeploy` tool's
own ELF dependency walk of `usr/bin/basic256` (which runs *before* its "qt"
plugin even starts) couldn't resolve `libQt6Sql.so.6` at all. Root cause:
`linuxdeploy` core has no notion of `$QMAKE` (that's a
`linuxdeploy-plugin-qt` concept for its own later Qt-specific stage) — it
just does a normal ELF/RPATH/loader-path resolution, and since this Qt6
comes from an aqtinstall tree (never `ldconfig`-registered, unlike
apt-installed Qt), it has no way to find `libQt6*.so.6` unless told.
`package_Linux_x86.sh` (`linuxdeployqt`, the older tool) never hit this
because it separately puts `$QT_DIR/bin` on `$PATH` and that older tool
queries `qmake` directly for Qt's lib dir. Fixed by exporting
`LD_LIBRARY_PATH="$QT_DIR/lib:$LD_LIBRARY_PATH"` before invoking
`linuxdeploy` — the standard fix for this exact linuxdeploy+aqt combination.

2026-07-05 (final) **All four CI targets build and package green**: Windows
(build + windeployqt + NSIS installer), macOS (build + macdeployqt), Linux
x86_64 (build + tar.gz + AppImage), Linux ARM64/RPi (build + tar.gz +
AppImage). This closes out the CI-pipeline half of Phase 1 — the app code
itself has compiled clean since the Convert.cpp fix a few sessions back;
everything since then was CI/packaging-script fallout from Windows being
the only platform whose build+package scripts had actually been ported to
Qt6 before this thread started. Still outstanding before Phase 1 is fully
done: the manual audibility check (BEEP/waveform + sound-file playback,
Rule 2) — green CI proves the app builds and the artifacts assemble, not
that Multimedia audio actually works at runtime. Phase 4 (flip CI to Qt6-only, drop the Qt5 `find_package` fallback) is
still untouched.

2026-07-05 (Phase 3) Both items done, both low-risk warning cleanups with
no behavioral change: `VariableWin.h`'s `variantLessThan()` now uses
`typeId() == QMetaType::...` instead of the deprecated
`type() == QVariant::...`, and the `QWheelEvent` handlers in
`BasicEdit.cpp`/`LineNumberArea.cpp` were confirmed to never call the
removed `.delta()` in the first place (they just forward to
`QPlainTextEdit::wheelEvent`). Not re-verified against an actual Qt6 build
by Claude Code this session (no local Qt6 toolchain) — same as every other
change in this file, verified by code-string/grep inspection; should be a
no-risk green on the next CI run since neither change alters behavior.
Phase 3 fully complete. Only Phase 4 remains.

2026-07-05 (Phase 4) All four items done in one pass:

- **TestSuite CI gate.** `TestSuite/testsuite.kbs` (the existing suite) is
  explicitly interactive: it needs manual printer/PDF setup, prompts
  `yn("Do TTS/Networking/Serial/WAVPLAY Testing?")` confirm dialogs, real
  mouse/keyboard input, and ends with a modal `alert`. Checked
  `Interpreter.cpp` directly: `OP_ALERT`/`OP_CONFIRM`/`OP_PROMPT`/
  `OP_OPENFILEDIALOG`/`OP_SAVEFILEDIALOG` all `std::exit(1)` immediately in
  `--silent` mode rather than block — by design, so `-s` can't run the
  interactive suite at all, and would hard-exit partway through. The file
  itself already anticipated an unattended run, though: it has a disabled
  `##goto section_unattended` and a `section_unattended:` label after which
  every included section (string/radix/function/error/math/fileio/database/
  if/remark/random/loop/fornext/sprite/dir/binaryop/types/arraybase/map) is
  free of dialogs/mouse/keyboard/printer/network/serial — confirmed by
  grepping each of those include files for
  alert/confirm/prompt/input/key/yn. Added `TestSuite/testsuite_ci.kbs`, a
  new CI-only aggregate that includes exactly that unattended subset (skips
  `testsuite_complete_include.kbs`, which has a fragile relative-path
  dependency-completeness check, and `testsuite_array_include.kbs`, which
  has real blocking `input` statements despite being listed before the
  "Interactive tests" heading) and sets `unattended = true` defensively.
  **Important gotcha documented in the new file and both runner scripts**:
  a failed assertion does not change the process exit code — BASIC-256's
  `end` statement is a normal deliberate stop, used both by
  `testsuite_common_include.kbs`'s `same()`/`different()`/etc. helpers on
  failure (print "fail" then `end`) and by the success path. So
  `.github/scripts/run_testsuite.sh` (Linux/macOS) and `run_testsuite.ps1`
  (Windows) check captured output for the word "fail" and for a
  `BASIC256_TESTSUITE_CI_PASSED` marker, not just the exit code. Linux
  additionally needs `QT_QPA_PLATFORM=offscreen` (GitHub's Linux runners
  have no display server at all, even for the never-shown windows silent
  mode still constructs). Wired into `build.yml` as a new step right after
  each platform's build step, before packaging starts, with a 5-minute
  `timeout-minutes` safety net. **Not run against a real CI/BASIC-256
  instance this session** — no local Qt6/BASIC-256 toolchain to execute the
  interpreter and confirm the new `.kbs` script actually parses and the
  runner scripts' output-matching logic works end-to-end. Flag back any
  parse errors or unexpected hangs from the next real CI run.
- **CMakeLists.txt**: `find_package(QT NAMES Qt6 Qt5 ...)` → `Qt6` only
  (both the `NAMES` line and the redundant `Qt${QT_VERSION_MAJOR}` second
  `find_package` call, now hardcoded `Qt6`). `Core5Compat` was already gone.
  Also collapsed the `if(QT_VERSION_MAJOR EQUAL 6) qt_add_translations()
  else() qt5_add_translation() ...` branch to the unconditional Qt6 call,
  since the else-branch was now permanently dead code. Left the
  `Qt${QT_VERSION_MAJOR}::Core` etc. target_link_libraries lines alone —
  still correct (QT_VERSION_MAJOR is still set to 6), not asked for, no
  reason to churn them.
- **COMPILING.txt / COMPILING_RaspberryPI.txt**: rewritten rather than
  patched. Both predated this entire migration by years — `COMPILING.txt`
  still told readers to run `qmake BASIC256.pro` (confirmed via glob: no
  `.pro` file exists anywhere in the repo anymore) with a Qt5.15/MinGW
  Windows setup; `COMPILING_RaspberryPI.txt` was a 2020 Debian-buster
  svn-checkout/qmake/Qt5/snapcraft recipe. Both now describe the real
  CMake+Qt6 process per platform, transcribed directly from the
  CI-verified `.github/scripts/build_*` scripts (not guessed) and pointing
  back at those scripts as the source of truth for anyone whose local setup
  drifts from what's written here.
- **Explicitly out of scope, left alone**: `package_Linux_RPi_AppImage.sh`
  and `package_Linux_RPi_Trixie.sh` still reference Qt5 library/plugin
  names (flagged in earlier session log entries) — that's a packaging
  detail, not a Phase 4 item, and both are currently CI-green, so untouched
  per the same "don't fix what isn't reported broken" reasoning as before.

Phase 4 complete pending the TestSuite gate's first real CI run.

2026-07-05 (later) First real run of the TestSuite gate, on all three
Linux/Windows targets at once — all three failed, for two separate
reasons, both boiling down to the same thing: the gate runs *before*
packaging, straight against the raw, un-deployed build tree, which none of
the runner scripts had accounted for.
- **Linux (x86_64 and ARM64)**: `build/basic256: No such file or directory`
  (exit 127). Bug in `run_testsuite.sh` itself: it `cd`s into `TestSuite/`
  before invoking the binary, but the caller passes a path relative to the
  repo root (`build/basic256`) — it stops resolving the moment the working
  directory changes. Fixed by resolving to an absolute path (via `realpath`,
  falling back to a manual `cd`+`pwd` if unavailable) before the `cd`.
- **Windows**: exit code `-1073741515` = `0xC0000135` =
  `STATUS_DLL_NOT_FOUND`. `windeployqt` (which stages Qt6's DLLs next to
  the exe) hasn't run yet at this point in the pipeline — packaging happens
  after this gate — so `build\Release\basic256.exe` can't find `Qt6Core.dll`
  etc. at all. Fixed by adding `$env:QT_DIR\bin` to `PATH` in
  `run_testsuite.ps1` (mirrors `package_Windows.ps1`'s own use of `QT_DIR`).
- **Anticipated same-class failure, fixed proactively on both scripts**:
  even once the core Qt DLLs/shared-libs are found, `QApplication` still
  needs to find a platform plugin (`qwindows.dll` / `libqxcb.so` /
  offscreen), which is *also* only staged next to the binary by
  packaging/`windeployqt`. Set `QT_PLUGIN_PATH`/`$env:QT_PLUGIN_PATH` to
  `$QT_DIR/plugins` on both Linux and Windows (only when `QT_DIR` is set —
  unset on RPi/Trixie, which uses system apt Qt6 with its plugins already
  in Qt's compiled-in default search path) to head off what would otherwise
  almost certainly be the very next failure. **Not verified** — this half
  of the fix is inferred from how Qt plugin loading works, not from an
  actual observed error yet; flag back if a platform-plugin error still
  shows up in the next log.
- Also applied the Linux `LD_LIBRARY_PATH="$QT_DIR/lib"` fix here that was
  already proven necessary for `linuxdeploy` in the packaging-script session
  — same root cause (aqtinstall Qt6 isn't `ldconfig`-registered), same fix,
  now needed here too since this step runs even earlier in the pipeline.
