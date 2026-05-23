BASIC-256 is an easy to use version of BASIC designed to teach anybody how to program. It was/is aimed at teaching the beginnings of programming to youngsters. 
In fact the original name of Basic256 was Kidbasic, but it is already quite capable for everyday hobby use in its current state.
A Qt5-based program, it has a 3-pane IDE with edit-, output- and graphics-windows. 
The original code and current downloadable version resides on SourceForge (https://sourceforge.net/projects/kidbasic/) and is at version 2.0.0.11. It has an example directory but all programs there need to be updated. There is also a Testsuite that works on the official SourceForge release but blocks on the one here.

Unfortunately, development of Basic256 has stopped apparently after a failed attempt to port it to Qt6.
Some have tried to get involved with development (RiOn and comick) and a most recent try has even moved sourceforge to github ( GitHub - comick/basic256)

This new  GitHub repository is my attempt to restart Basic256 and takes the r946 branch of the original code (the branch from which resulted the 2.0.0.11 version) with the aim of trying to modernize the codebase.

The aim for this branch is
 - make it compile on Windows, Linux-Intel and Linux-ARM (RPi) from a single Actions pipeline. (Qt being cross-platform, MacOS port may also be looked at).
 ==> At the moment, I'm concentrating on Windows. Linux-Intel, Linux-ARM, Portable app will have to wait
 - port from qmake to CMake
 ==> Seems to have gone ok
 - make clean-ups & modernisations where possible. (
 ==> synchronious ESpeak has been replaced by asynchronious Qt texttoSpeech

I asked Clause.com to do an Audit and show a way forward for this. I previously asked ChatGPTs for an audit. Windows, Linux x86 and RPi are included although Linux x86 and RPi are still to be tested.  Once these 3 are up and running, I'll make an official release and spread the word...

Here is Claude's feedback:

# BASIC256 Project — Deep Analysis & Roadmap

**Repository:** uglymike17/basic256  
**Date of review:** May 2026  
**Audience:** Project owner, non-C++ programmer

---

## 1. What this project actually is

BASIC256 is a revival effort of an educational BASIC interpreter (think old-school BASIC for kids, with graphics and sound). The original died on SourceForge after a failed Qt6 migration. This fork takes the last stable source (r946 / v2.0.0.11) and attempts to modernize the build system and get it compiling again on modern platforms — primarily Raspberry Pi, Windows and Linux-x86 and potentially MacOS.

The project is 71% C++, 13% Yacc (grammar), and 7% Lex (tokenizer) — the core of a language interpreter. You are not writing a simple app; you are maintaining a programming language runtime and IDE.

---

## 2. The CMakeLists.txt — Is it modern?

### What it does well

- Uses CMake 3.21 minimum — correct for 2024/2025 work.
- C++17 is set correctly — the right standard for Qt5 and Qt6.
- `CMAKE_AUTOMOC`, `AUTORCC`, `AUTOUIC` are all on — handles Qt's code generation automatically.
- Flex/Bison integration is done the proper CMake way (`BISON_TARGET`, `FLEX_TARGET`, `ADD_FLEX_BISON_DEPENDENCY`).
- Source files are listed explicitly (not via glob wildcards) — this is correct CMake practice.
- Platform-specific defines use CMake's `if(WIN32)` / `if(APPLE)` / `if(UNIX)` idioms rather than raw compiler flags.
- `target_compile_definitions`, `target_link_libraries`, `target_include_directories` all use the modern `PRIVATE` scoping — correct.
- Dual Qt5/Qt6 detection via `find_package(QT NAMES Qt6 Qt5 ...)` is the right pattern.
- `GNUInstallDirs` is included and used for install paths.

### What is outdated or problematic

**`qt5_add_translation()` is Qt5-only.**  
The comment in the file even says "When switching to Qt6, use `qt_add_translations()` instead" — but no conditional logic exists to actually do this. Right now, if Qt6 is found, this line will fail. Since the `find_package` logic tries Qt6 first, this is a latent build-breaker waiting to happen.

Fix: replace with:
```cmake
if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_translations(basic256 TS_FILES ${TS_FILES})
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
    target_sources(basic256 PRIVATE ${QM_FILES})
endif()
```

**Duplicate `find_package` block.**  
There is a blank line between the two `find_package` calls but no real issue — however the first block finds `QT` (capital) to determine the version, then the second finds `Qt${QT_VERSION_MAJOR}`. This is correct pattern but worth noting — you cannot collapse them into one.

**`qt5_add_translation` produces `QM_FILES` but those are never added to `target_sources`.**  
The `QM_FILES` variable is listed in `add_executable(... ${QM_FILES})`, which compiles the `.qm` files into the executable as resources — but only if the `.qm` files are also referenced in the `.qrc` file. If they are not in the `.qrc`, they will be compiled as empty resource slots. Verify `resources/resource.qrc` includes a reference to the translation files.

**Windows resource file path.**  
`resources/windows.rc` is appended to `SOURCES` for Win32. This is fine but the `.rc` file needs to exist and be valid. If it references the old `.ico` path from qmake days, it may fail silently on MSVC.

**No `cmake/` subdirectory.**  
All logic is in the root CMakeLists.txt. For a project of this size this is acceptable, but as platform-specific logic grows, a `cmake/` folder with helper files (`Packaging.cmake`, `Translations.cmake`) becomes important for readability.

**`COMPILING.txt` / `HOWTO_debian.txt` are probably outdated.**  
These likely still reference qmake. They should be updated to reflect CMake instructions.

### Overall CMakeLists.txt verdict

**B+ — Mostly modern, a few Qt5/Qt6 incompatibilities that will bite you.**

---

## 3. The build.yml — Is it modern?

### Structure overview

The workflow has a matrix that currently runs: Raspberry Pi ARM64 via QEMU emulation, Windows and Linux x86. There is a separate `release:` job that uploads to GitHub Releases.

### What it does well

- Uses `actions/checkout@v4.2.2`, `actions/upload-artifact@v4.6.2`, `actions/download-artifact@v4.3.0` — all pinned to specific versions, which is good security practice.
- The `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24` env variable shows awareness of GitHub's Node runtime migration.
- `fail-fast: false` is correct for multi-platform builds — one platform failing shouldn't cancel others.
- `softprops/action-gh-release@v2` for release uploads is the current standard.
- The ARM build uses `uraimo/run-on-arch-action@v2` to emulate aarch64 — this works but see below.
- The Windows steps correctly rename `win_flex.exe` → `flex.exe` and `win_bison.exe` → `bison.exe` to satisfy CMake's tool detection.
- `patchelf --set-rpath '$ORIGIN/lib'` in the ARM packaging step is the right idea.

### What is outdated or problematic

**QEMU emulation for ARM is the wrong approach in 2025/2026.**  
GitHub Actions now has native ARM64 runners: `ubuntu-24.04-arm`. QEMU-emulated builds are dramatically slower (often 10-20x), hit memory limits (hence `j1` in your build command), and are harder to debug. The `uraimo/run-on-arch-action` approach was the only option before 2024, but it is now obsolete for your use case.

Replace this entire block:
```yaml
- os: ubuntu-latest
  arch: arm64
  use_jurplel: false
  artifact_name: BASIC256-RaspberryPi-ARM64
```
...and the QEMU step with:
```yaml
- os: ubuntu-24.04-arm
  artifact_name: BASIC256-RaspberryPi-ARM64
```
Then use a standard apt-install + cmake build, just like Linux x86. No QEMU, no container, no `j1`.

**No launcher script in the ARM dist.**  
The Windows packaging creates `basic256.bat` that sets environment variables before launching. The ARM packaging has no equivalent. A `run.sh` launcher that sets `LD_LIBRARY_PATH` and `QT_PLUGIN_PATH` is the belt-and-suspenders safety net for systems that ignore rpath.

**The `Package Linux ARM` step references `ubuntu-24.04-arm` but the matrix uses `ubuntu-latest`.**  
These are different runner names. The packaging step has `if: matrix.os == 'ubuntu-24.04-arm'` — which will never be true under the current matrix definition of `os: ubuntu-latest`. This step is dead code and will never execute.

**`env: QT_VERSION: 5.15.2` is declared but never used in the active ARM build path.**  
The ARM path installs Qt via `apt`, not `jurplel`, so this env var is unused. It's leftover from the commented-out blocks and causes confusion.

**The `release:` job uses `ubuntu-22.04` as its runner.**  
Ubuntu 22.04 will reach end-of-life for GitHub Actions runners. Change to `ubuntu-24.04`.

**`jurplel/install-qt-action@v3` is outdated.**  
Version 4 of this action has been available since late 2024 and fixes several Qt module installation issues. The `qt_arch: linux_gcc_arm64` arch string also only works in v4 — v3 does not support ARM Linux at all.

### Overall build.yml verdict

**C+ — The structure is reasonable but the active ARM build path has multiple bugs that cause exactly the runtime library problems you are experiencing. The QEMU approach should be abandoned entirely in favor of native ARM64 runners.**

---

## 4. The repository — project-level analysis

### What is in the repo

Looking at the file tree:

| Item | Status |
|---|---|
| `CMakeLists.txt` | Active — your new build system |
| `BASIC256.nsi` | NSIS Windows installer script — may be outdated |
| `old github workflows qmake/` | Archived old CI — good to keep for reference |
| `BASIC256Portable/` | Separate portable build — unclear if active |
| `debian/` | Debian packaging files — presence is promising |
| `utility_maintenance_programs/` | Unclear purpose — probably old tooling |
| `LEX/` | Flex/Bison grammar files — core of the interpreter |
| `Modules/` | Unclear — probably interpreter extension modules |
| `TestSuite/` | Test programs — valuable, keep and expand |
| `Translations/` | `.ts` files for 8 languages — active |
| `wikihelp/` | Referenced in CMakeLists.txt but not visible in tree |

### Clutter and legacy weight

The repo carries significant legacy weight:

**`COMPILING.txt` and `HOWTO_debian.txt`** almost certainly reference qmake. They need to be updated to describe the CMake workflow or they will mislead anyone trying to build from source.

### What is actually working

Based on the README, the CI, and the build files, the current state is:

- **RPi ARM64 build: compiles, but runtime library loading fails.** The binary is built successfully inside the QEMU container but the resulting `dist/` folder has missing libraries and no `qt.conf`, so the application either doesn't launch or launches without a visible window.
- **Windows: commented out** — there is substantial Windows build code present but it is not running in CI.
- **Linux x86: commented out.**
- **macOS: not present.**

The project is in "it compiles but doesn't run properly" state for its primary target platform.

### The Debian packaging (`debian/` folder)

The presence of a `debian/` folder is genuinely valuable — it means someone has done the work to describe how to build a proper `.deb` package. However, this folder almost certainly still references qmake. If updated to use CMake (`dpkg-buildpackage` can invoke `cmake`), this would give you a proper installable package that users can install with `sudo apt install ./basic256.deb` rather than having to run a shell script.

### The TestSuite

There is a `TestSuite/` directory. This is valuable and underutilized. Currently there is no CI step that runs the test suite after building. A BASIC256 interpreter that passes its own test suite is a much stronger claim than one that merely compiles. Adding a test step to CI would catch regressions early.

### The `wikihelp/` issue

`CMakeLists.txt` references `./wikihelp/help` in an install rule:
```cmake
install(DIRECTORY ./wikihelp/help DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256)
```
This directory is not visible in the repository tree. It may have been accidentally excluded from `.gitignore`, or it may never have been committed. If it exists locally but not in the repo, the install step will silently produce an incomplete installation. Either commit it or remove the install rule until it exists.

---

## 5. The bigger picture: what ChatGPT recommended vs. where you actually are

The README includes a detailed ChatGPT audit. That audit was largely sound advice, but some of it is aspirational for a solo maintainer who doesn't write C++. A realistic assessment:

**ChatGPT's advice that you should follow now:**

- Switch ARM64 CI from QEMU to `ubuntu-24.04-arm` native runner — this is a one-line change with major impact.
- Fix the `qt.conf` generation in packaging — five lines.
- Fix the missing `libQt5XcbQpa` and `libQt5DBus` copies — two lines.
- Add a launcher `run.sh` script to the ARM dist — six lines.
- Delete the legacy `.pro`, `.sln`, `.vcproj` files.
- Update `COMPILING.txt`.

**ChatGPT's advice that is correct but non-urgent:**

- Reorganize source into `src/core/`, `src/ide/` etc. — good long-term architecture but requires C++ knowledge to do safely.
- Qt6 migration — worthwhile eventually, but Qt5 works fine and Qt5 LTS runs until 2025/2026.
- Adding clang-tidy, clang-format, cppcheck — useful but not blocking.
- AppImage / Flatpak packaging — good for distribution, not needed for initial working builds.

---

## 6. Prioritized action plan

These are ordered by impact and difficulty, for someone who cannot write C++.

### Immediate — fix the RPi build (no C++ required)

1. Add `qt.conf` generation to the ARM packaging step in `build.yml`.
2. Add `libQt5XcbQpa.so.*` and `libQt5DBus.so.*` to the `cp` commands.
3. Add a `run.sh` launcher script to the dist folder.
4. Add `INSTALL_RPATH` and `BUILD_WITH_INSTALL_RPATH` to `CMakeLists.txt` (copy-paste from section 2 above).

### Short-term — modernize the CI

5. Replace the QEMU ARM build with `ubuntu-24.04-arm` native runner.
6. Uncomment and fix the Linux x86 build (update from `ubuntu-22.04` to `ubuntu-24.04`, update `jurplel` to v4).
7. Uncomment the Windows build and test it.
8. Add a `TestSuite` execution step after building.

### Clean-up — reduce confusion and legacy weight

9. Delete `BASIC256.pro`, `BASIC256Portable.pro`, `basic256.sln`, `basic256.vcproj`, `BASIC256PortableSupportFileCopy.bat`, `ziplinuxsource.sh`.
10. Update `COMPILING.txt` with CMake build instructions.
11. Resolve the `wikihelp/` missing directory situation.
12. Fix the `qt5_add_translation` Qt5/Qt6 conditional in `CMakeLists.txt`.

### Medium-term — distribution quality

13. Update the `debian/` packaging to use CMake as the build system.
14. Add a GitHub Actions step that builds a `.deb` package.
15. Create a proper GitHub Release with the first working ARM64 tarball.
16. Write a `README` section explaining how to run the binary from the tarball.

### Long-term — future-proofing (requires C++ help or AI assistance)

17. Replace `qt5_add_translation` with the dual Qt5/Qt6 version.
18. Work through the deprecated Qt API list (QRegExp → QRegularExpression, etc.).
19. Port to Qt6 once the deprecated API cleanup is done.
20. Reorganize source into logical subdirectories.

---

## 7. One-paragraph summary

The project has successfully completed the hardest part — migrating from qmake to CMake and getting the interpreter to compile under a modern toolchain. The remaining problems are all in the build and packaging layer, not in the C++ code itself. The QEMU-based ARM build should be replaced with GitHub's native ARM64 runner, which would also eliminate the `j1` serial build workaround and dramatically speed up CI. Once these fixes are in place, BASIC256 would have genuinely distributable Windows, Linux x86 and ARM64 packages — which is the stated goal of the project.
