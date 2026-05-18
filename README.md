BASIC-256 is an easy to use version of BASIC designed to teach anybody how to program. It was/is aimed at teaching the beginnings of programming to youngsters. 
In fact the original name of Basic256 was Kidbasic, but it is already quite capable for everyday hobby use in its current state.
A Qt5-based program, it has a 3-pane IDE with edit-, output- and graphics-windows. 
The original code and current downloadable version resides on SourceForge (https://sourceforge.net/projects/kidbasic/) and is at version 2.0.0.11

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

I asked ChatGPT to do an Audit and show a way forward for this. This is what it answered:

# BASIC256 Build System Audit and Modernization Plan

Repository: [uglymike17/basic256](https://github.com/uglymike17/basic256?utm_source=chatgpt.com)

Based on your description, this repository is derived from the BASIC256 2.0.0.11 SourceForge branch and still uses a classic Qt/qmake-based build structure.

This document focuses on:

1. A unified GitHub Actions CI/CD pipeline for:

   * Windows
   * Linux x86_64
   * Linux ARM64
   * macOS
2. Migration from qmake to modern CMake
3. Qt5/Qt6 compatibility strategy
4. Packaging recommendations
5. Long-term modernization roadmap

---

# 1. High-Level Audit

## Current Situation (likely based on 2.0.x)

Typical BASIC256 repositories from this era usually contain:

* `.pro` qmake project files
* manual include/lib paths
* platform-specific hacks
* bundled/generated UI files
* weak separation between:

  * interpreter
  * IDE/editor
  * graphics subsystem
  * sound/multimedia
* direct Qt Widgets coupling
* no proper dependency abstraction
* no CI or fragmented CI

This worked well in:

* Qt4
* early Qt5
* desktop-only environments

But becomes increasingly difficult for:

* Apple Silicon
* ARM Linux
* Qt6
* reproducible builds
* GitHub Actions
* package managers
* cross compilation

---

# 2. Recommended Target Architecture

You should aim for:

```text
basic256/
├── CMakeLists.txt
├── cmake/
│   ├── modules/
│   └── toolchains/
├── src/
│   ├── core/
│   ├── interpreter/
│   ├── ide/
│   ├── graphics/
│   ├── audio/
│   └── main/
├── resources/
├── translations/
├── tests/
├── packaging/
└── .github/workflows/
```

The key architectural change:

## Separate "core" from "GUI"

You eventually want:

| Component | Responsibility          |
| --------- | ----------------------- |
| core      | interpreter/runtime     |
| ide       | editor/debugger UI      |
| graphics  | rendering/window system |
| audio     | multimedia              |
| platform  | OS integration          |

This makes:

* testing easier
* ARM support easier
* Qt6 migration easier
* future scripting support easier

---

# 3. Unified GitHub Actions Strategy

## Recommended Platforms

| Platform            | Runner           | Architecture |
| ------------------- | ---------------- | ------------ |
| Windows             | windows-latest   | x64          |
| Ubuntu              | ubuntu-24.04     | x64          |
| Ubuntu ARM          | ubuntu-24.04-arm | ARM64        |
| macOS Intel         | macos-13         | x64          |
| macOS Apple Silicon | macos-14         | ARM64        |

You can later add:

* AppImage
* Flatpak
* Homebrew
* MSIX
* dmg notarization

---

# 4. Recommended Qt Strategy

## Do NOT stay locked to Qt5 forever

Recommended transition:

| Phase   | Goal                       |
| ------- | -------------------------- |
| Phase 1 | Build with Qt5 and Qt6     |
| Phase 2 | Remove deprecated Qt5 APIs |
| Phase 3 | Qt6 primary                |
| Phase 4 | Optional Qt5 compatibility |

Recommended minimum:

```cmake
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Widgets
    Gui
    Core
    Multimedia
    Network
    PrintSupport
)
```

This is the canonical dual Qt5/Qt6 approach.

---

# 5. Proposed CMake Migration

## Phase 1 — Parallel Build System

Do NOT immediately remove qmake.

Instead:

* keep `.pro` files working
* introduce CMake in parallel
* verify all platforms
* switch CI to CMake
* remove qmake later

This dramatically lowers migration risk.

---

# 6. Minimal Root CMakeLists.txt

## Recommended Starting Point

```cmake
cmake_minimum_required(VERSION 3.21)

project(BASIC256 VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    Network
    PrintSupport
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    Network
    PrintSupport
)

add_subdirectory(src)
```

---

# 7. Recommended src/CMakeLists.txt

```cmake
set(BASIC256_SOURCES
    main.cpp
    mainwindow.cpp
    interpreter.cpp
    graphics.cpp
)

set(BASIC256_HEADERS
    mainwindow.h
    interpreter.h
    graphics.h
)

set(BASIC256_RESOURCES
    resources/basic256.qrc
)

qt_add_executable(basic256
    ${BASIC256_SOURCES}
    ${BASIC256_HEADERS}
    ${BASIC256_RESOURCES}
)

target_link_libraries(basic256 PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::PrintSupport
)
```

---

# 8. Biggest qmake → CMake Migration Problems

These are the usual migration pain points for old Qt projects.

## A. UI Files

qmake:

```pro
FORMS += mainwindow.ui
```

CMake:

Handled automatically using:

```cmake
set(CMAKE_AUTOUIC ON)
```

---

## B. MOC Generation

qmake handled this magically.

CMake equivalent:

```cmake
set(CMAKE_AUTOMOC ON)
```

---

## C. Resources

qmake:

```pro
RESOURCES += basic256.qrc
```

CMake:

```cmake
qt_add_executable(... basic256.qrc)
```

---

## D. Platform Defines

Old projects often contain:

```cpp
#ifdef WIN32
```

Prefer:

```cpp
#ifdef Q_OS_WIN
```

because Qt abstracts platform detection.

---

## E. Manual Include Paths

qmake often contains:

```pro
INCLUDEPATH += /usr/include/foo
```

Modern CMake should use:

```cmake
target_include_directories(...)
```

Never use global include directories unless unavoidable.

---

# 9. Recommended GitHub Actions Workflow

## Single Unified build.yml

```yaml
name: build

on:
  push:
  pull_request:
  workflow_dispatch:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-latest
            qt_arch: win64_msvc2022_64

          - os: ubuntu-24.04
            qt_arch: gcc_64

          - os: ubuntu-24.04-arm
            qt_arch: linux_gcc_arm64

          - os: macos-13
            qt_arch: clang_64

          - os: macos-14
            qt_arch: clang_64

    runs-on: ${{ matrix.os }}

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Qt
        uses: jurplel/install-qt-action@v4
        with:
          version: '6.7.2'
          arch: ${{ matrix.qt_arch }}
          modules: qtmultimedia qtserialport qtspeech

      - name: Configure
        run: |
          cmake -B build \
            -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: |
          cmake --build build --config Release --parallel

      - name: Package
        run: |
          cmake --install build --prefix install

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: basic256-${{ matrix.os }}
          path: install/
```

---

# 10. Linux ARM Strategy

## IMPORTANT

GitHub now supports ARM runners.

You no longer need:

* QEMU emulation
* Docker emulation
* cross-compilation hacks

Use:

```yaml
runs-on: ubuntu-24.04-arm
```

This is vastly more reliable.

---

# 11. Windows Strategy

## Strong Recommendation

Use:

* MSVC
* NOT MinGW

Why:

| MSVC                          | MinGW             |
| ----------------------------- | ----------------- |
| official Qt support           | partial           |
| fewer DLL problems            | many DLL problems |
| better GitHub Actions support | weaker            |
| better debugging              | weaker            |
| modern CMake support          | inconsistent      |

Your previous workflow issues involving:

```text
libstdc++-6.dll missing
```

are classic MinGW deployment problems.

MSVC removes most of these.

---

# 12. macOS Strategy

## Recommended

Build both:

* Intel
* Apple Silicon

Initially separately.

Later:

* universal binaries
* notarization
* signed DMGs

can be added.

---

# 13. Packaging Recommendations

## Windows

Use:

```text
windeployqt
```

inside CI.

Example:

```yaml
- name: Deploy Qt
  run: |
    windeployqt install/basic256.exe
```

---

## Linux

Recommended:

* AppImage
* .deb

Eventually:

* Flatpak

---

## macOS

Use:

```text
macdeployqt
```

Later:

* notarization
* dmgbuild

---

# 14. Recommended Dependency Cleanup

## Remove obsolete Qt APIs

You will likely encounter:

| Old                | New                        |
| ------------------ | -------------------------- |
| QRegExp            | QRegularExpression         |
| SIGNAL/SLOT macros | function-pointer connect   |
| QtScript           | QJSEngine or custom parser |
| QString::null      | QString()                  |
| foreach            | range-for                  |

This cleanup should happen BEFORE full Qt6 migration.

---

# 15. Recommended Compiler Baseline

## Suggested

```cmake
set(CMAKE_CXX_STANDARD 17)
```

Do not remain on C++98/11.

Qt6 is much happier with C++17.

---

# 16. Recommended CI Expansion

Once the main build works:

## Add

### Static Analysis

```yaml
- clang-tidy
- cppcheck
```

### Formatting

```yaml
- clang-format
```

### Unit Tests

```yaml
- ctest
```

### Release Automation

```yaml
on:
  release:
```

---

# 17. Recommended Migration Timeline

## Stage 1

* Add root CMakeLists.txt
* Build Windows
* Build Ubuntu

## Stage 2

* Add ARM
* Add macOS
* Add packaging

## Stage 3

* Clean deprecated Qt APIs
* Add Qt6 support

## Stage 4

* Remove qmake
* Remove legacy hacks
* Add tests

---

# 18. Most Important Recommendation

## Do NOT try to modernize everything simultaneously.

This should be done in layers:

1. Build reproducibility
2. CI stability
3. CMake migration
4. Qt modernization
5. Architecture cleanup
6. Packaging

Trying to combine all six at once usually creates a multi-month unstable branch.

---

# 19. Strongly Recommended Immediate Actions

## First Priority

### Create:

```text
.github/workflows/build.yml
```

with:

* Windows
* Ubuntu x64

ONLY.

Do not add all platforms on day one.

---

## Second Priority

Create:

```text
CMakeLists.txt
```

that:

* builds the current app
* changes nothing else

---

## Third Priority

Refactor platform-specific code:

```cpp
#ifdef WIN32
```

into:

```cpp
#ifdef Q_OS_WIN
```

---

## Fourth Priority

Switch CI to CMake.

Only then:

* add ARM
* add macOS
* add packaging

---

# 20. Long-Term Vision

Once CMake exists, BASIC256 becomes dramatically easier to:

* package
* maintain
* modernize
* port
* test
* distribute
* integrate with Linux distros
* support Apple Silicon
* support future Qt versions

The migration is absolutely worth doing.

---

# 21. Final Recommended Target Stack

| Component        | Recommendation                       |
| ---------------- | ------------------------------------ |
| Build System     | CMake                                |
| CI               | GitHub Actions                       |
| Windows Compiler | MSVC                                 |
| Linux Compiler   | GCC/Clang                            |
| macOS Compiler   | AppleClang                           |
| Qt Version       | Qt6 primary, Qt5 compatible          |
| C++ Standard     | C++17                                |
| Packaging        | AppImage + windeployqt + macdeployqt |
| Testing          | CTest                                |
| Formatting       | clang-format                         |
| Static Analysis  | clang-tidy                           |

---

# 22. Suggested Next Deliverables

The next practical files to create would be:

1. Root `CMakeLists.txt`
2. `src/CMakeLists.txt`
3. `.github/workflows/build.yml`
4. `cmake/FindDependencies.cmake`
5. `packaging/windows/`
6. `packaging/linux/`
7. `packaging/macos/`

Those seven items form the foundation for a modern cross-platform BASIC256 codebase.

---

Additional reference:

* [Qt CMake Manual](https://doc.qt.io/qt-6/cmake-manual.html?utm_source=chatgpt.com)
* [install-qt-action](https://github.com/jurplel/install-qt-action?utm_source=chatgpt.com)
* [CMake Documentation](https://cmake.org/documentation/?utm_source=chatgpt.com)
* [GitHub Actions Documentation](https://docs.github.com/actions?utm_source=chatgpt.com)


All his might make it possible to relaunch Basic256 on a new, modern website like this A.I.-generated one:

<img width="1024" height="1536" alt="image" src="https://github.com/user-attachments/assets/32f4c601-2637-4f97-927e-7afc5226a163" />
