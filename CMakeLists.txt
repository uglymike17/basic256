cmake_minimum_required(VERSION 3.21)
project(basic256 VERSION 2.1.0 LANGUAGES C CXX)

# Force C++17 standard 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Automatic handling of Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)

# IDE/Tooling supportsupport for compile_commands.json (e.g. for clangd, VSCode C++ extensions) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------------------------------------
# Browser-unavailable feature flags (WASM port plan, Phase 3)
# -------------------------------------------------------------------
# All ON by default -- the desktop build keeps its current feature set
# byte-for-byte. The Phase 4 WASM build configures all six OFF: the
# underlying Qt module or OS facility either doesn't exist or can't work
# in a browser sandbox (see WASM.md's decisions and Phase 3 section).
# When a flag is off, the corresponding opcode(s) raise ERROR_NOTAVAILABLE
# through the normal Error mechanism instead of silently no-opping.
option(BASIC256_ENABLE_PROCESS "Enable SYSTEM (QProcess)" ON)
option(BASIC256_ENABLE_SERIAL  "Enable SERIALOPEN et al. (QSerialPort)" ON)
option(BASIC256_ENABLE_SQL     "Enable DBOPEN et al. (QtSql)" ON)
option(BASIC256_ENABLE_PRINTER "Enable PRINTERON et al. (QtPrintSupport)" ON)
option(BASIC256_ENABLE_TCP     "Enable NETLISTEN/NETCONNECT et al. (QTcpServer/QTcpSocket)" ON)
option(BASIC256_ENABLE_TTS     "Enable SAY (QTextToSpeech)" ON)

# Find Qt6 components -- Sql/SerialPort/TextToSpeech/PrintSupport are only
# required when their BASIC256_ENABLE_* flag is on: the Phase 4 WASM Qt SDK
# doesn't ship any of these, and the flags-OFF desktop dress rehearsal
# (Phase 3 gate) proves the build doesn't need them either.
# PrintSupport correction (Phase 4): Phase 3 kept PrintSupport unconditional
# because src/gui's own Print... menu actions (BasicEdit/BasicGraph/
# BasicOutput/PreferencesWin) use QPrinter/QPrintDialog directly, independent
# of the BASIC-language PRINTERON/etc opcodes -- true on desktop, where the
# module always exists. But Qt for WebAssembly's own docs say flatly
# "Printing is not supported" -- the wasm Qt SDK does not ship a
# QtPrintSupport module at all, so find_package(... PrintSupport) would fail
# outright at configure time regardless of BASIC256_ENABLE_PRINTER's value.
# Fixed by extending BASIC256_ENABLE_PRINTER to also gate those GUI menu
# actions (see src/gui/BasicEdit.cpp, BasicGraph.cpp, BasicOutput.cpp,
# PreferencesWin.cpp -- each already had an #ifdef ANDROID "printing not
# supported" precedent for slotPrint(), extended the same way Phase 3
# extended it for OP_OPENSERIAL/SERIAL).
set(BASIC256_QT_COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    LinguistTools
    Network
)
if(BASIC256_ENABLE_SQL)
    list(APPEND BASIC256_QT_COMPONENTS Sql)
endif()
if(BASIC256_ENABLE_SERIAL)
    list(APPEND BASIC256_QT_COMPONENTS SerialPort)
endif()
if(BASIC256_ENABLE_PRINTER)
    list(APPEND BASIC256_QT_COMPONENTS PrintSupport)
endif()
if(BASIC256_ENABLE_TTS)
    list(APPEND BASIC256_QT_COMPONENTS TextToSpeech)
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS ${BASIC256_QT_COMPONENTS})
find_package(Qt6 REQUIRED COMPONENTS ${BASIC256_QT_COMPONENTS})



# -------------------------------------------------------------------
# Source Files & Wildcards
# -------------------------------------------------------------------

# Note: Explicitly listing files is best practice in CMake,
# src/core = interpreter + language runtime (QtCore/QtGui-painting only,
# plus the not-yet-flagged QtSql/QtSerialPort/QtPrintSupport/QtNetwork/
# QtMultimedia usage -- Phase 3 gates those behind BASIC256_ENABLE_* flags).
set(SOURCES_CORE
    src/core/BasicDownloader.cpp
    src/core/BasicKeyboard.cpp
    src/core/BasicMediaPlayer.cpp
    src/core/Convert.cpp
    src/core/DataElement.cpp
    src/core/Error.cpp
    src/core/GraphicsBuffer.cpp
    src/core/Interpreter.cpp
    src/core/md5.cpp
    src/core/Sleeper.cpp
    src/core/Sound.cpp
    src/core/Stack.cpp
    src/core/Variables.cpp
    src/core/WasmAudioSink.cpp
    src/core/WasmLaunch.cpp
    src/core/WasmSettings.cpp
)

set(HEADERS_CORE
    src/core/BasicDownloader.h
    src/core/BasicKeyboard.h
    src/core/BasicMediaPlayer.h
    src/core/BasicTypes.h
    src/core/CompileErrors.h
    src/core/Constants.h
    src/core/Convert.h
    src/core/DataElement.h
    src/core/Error.h
    src/core/ErrorCodes.h
    src/core/GraphicsBuffer.h
    src/core/Interpreter.h
    src/core/md5.h
    src/core/Sleeper.h
    src/core/Sound.h
    src/core/Stack.h
    src/core/Variables.h
    src/core/Version.h
    src/core/WasmAudioSink.h
    src/core/WasmLaunch.h
    src/core/WasmSettings.h
    src/core/WordCodes.h
)

# src/gui = everything QtWidgets
set(SOURCES_GUI
    src/gui/BasicDock.cpp
    src/gui/BasicEdit.cpp
    src/gui/BasicGraph.cpp
    src/gui/BasicIcons.cpp
    src/gui/BasicOutput.cpp
    src/gui/BasicWidget.cpp
    src/gui/EditSyntaxHighlighter.cpp
    src/gui/LineNumberArea.cpp
    src/gui/MainWindow.cpp
    src/gui/PreferencesWin.cpp
    src/gui/ReplaceWin.cpp
    src/gui/VariableWin.cpp
    src/gui/ViewWidgetIFace.cpp
)

set(HEADERS_GUI
    src/gui/BasicDock.h
    src/gui/BasicEdit.h
    src/gui/BasicGraph.h
    src/gui/BasicIcons.h
    src/gui/BasicOutput.h
    src/gui/BasicWidget.h
    src/gui/EditSyntaxHighlighter.h
    src/gui/LineNumberArea.h
    src/gui/MainWindow.h
    src/gui/PreferencesWin.h
    src/gui/ReplaceWin.h
    src/gui/Settings.h
    src/gui/VariableWin.h
    src/gui/ViewWidgetIFace.h
)

# src/app = Main.cpp, RunController glue, resources wiring
set(SOURCES_APP
    src/app/Main.cpp
    src/app/RunController.cpp
)

set(HEADERS_APP
    src/app/RunController.h
)

# -------------------------------------------------------------------
# Dynamic Lexer/Parser Generation (Flex & Bison)
# -------------------------------------------------------------------

# Find the standard Flex and Bison executables on the system
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Point to your original grammar and scanner files
set(BISON_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/LEX/basicParse.y)
set(FLEX_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/LEX/basicParse.l)

# Direct CMake to generate the C files dynamically inside the build directory
BISON_TARGET(BasicParser ${BISON_INPUT} ${CMAKE_CURRENT_BINARY_DIR}/basicParse.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/basicParse.tab.h)
FLEX_TARGET(BasicLexer ${FLEX_INPUT} ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

# Ensure the lexer always waits for the parser's header file to be ready
ADD_FLEX_BISON_DEPENDENCY(BasicLexer BasicParser)

# The generated parser/lexer are tightly coupled to the interpreter (they
# call back into it) and belong in basic256core, not the app executable.
set(SOURCES_CORE
    ${BISON_BasicParser_OUTPUTS}
    ${FLEX_BasicLexer_OUTPUTS}
    ${SOURCES_CORE}
)
set(HEADERS_CORE
    ${BISON_BasicParser_OUTPUTS_H}
    ${HEADERS_CORE}
)

# Qt Resources
set(RESOURCES
    resources/resource.qrc
)
# WASM Phase 5: bundle a curated subset of Examples/ (self-contained .kbs
# files only -- no external image/sound assets, none using a
# BASIC256_ENABLE_* flag that's off for wasm) as a Qt resource so
# File->Open Example works without a real filesystem. Lives in DemoWASM/,
# separate from Examples/, so the WASM curation can be maintained
# independently of the full example set. Desktop builds don't need this:
# Examples/ already ships as loose files next to the binary via the
# packaging scripts, and the normal Open dialog can browse to them.
if(EMSCRIPTEN)
    list(APPEND RESOURCES DemoWASM/examples.qrc)
endif()

# Translations 
set(TS_FILES
    Translations/basic256_en.ts
    Translations/basic256_de.ts
    Translations/basic256_ru.ts
    Translations/basic256_es.ts
    Translations/basic256_fr.ts
    Translations/basic256_pt.ts
    Translations/basic256_nl.ts
    Translations/basic256_it.ts
)

# -------------------------------------------------------------------
# Platform Specific Settings
# -------------------------------------------------------------------

if(WIN32)
    set(PLATFORM_DEFS WIN32 USE_QT_TEXTTOSPEECH)
    set(PLATFORM_LIBS ole32 ws2_32 winmm)
    list(APPEND SOURCES_APP
        resources/windows.rc
)

elseif(APPLE)
    set(PLATFORM_DEFS MACX MACX_SAY USE_QT_TEXTTOSPEECH)
    set(PLATFORM_LIBS "")
    set(PLATFORM_INCLUDE_DIRS /opt/local/include)  # ← store, don't apply yet
    set(PLATFORM_LINK_DIRS /opt/local/lib)

    # App icon
    list(APPEND SOURCES_APP resources/basic256.icns)
    set_source_files_properties(resources/basic256.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE basic256.icns)

elseif(EMSCRIPTEN)
    # WASM Phase 4: must come before "elseif(UNIX AND NOT APPLE)" below --
    # Emscripten's CMake toolchain sets UNIX to true (it's a POSIX-like
    # target), so without this branch WASM builds would silently fall into
    # the Linux branch and define LINUX. That pulled in a real, Linux-only
    # OSS sound header (RunController.cpp's <sys/soundcard.h>, the first
    # compile break this phase actually hit) and would make OP_OSTYPE
    # misreport the browser as Linux. No platform defs/libs needed for wasm.
    set(PLATFORM_DEFS "")
    set(PLATFORM_LIBS "")

elseif(UNIX AND NOT APPLE)
    set(PLATFORM_DEFS LINUX USE_QT_TEXTTOSPEECH)
    set(PLATFORM_LIBS m)
endif()

# -------------------------------------------------------------------
# basic256core -- interpreter + language runtime static library
# -------------------------------------------------------------------

add_library(basic256core STATIC
    ${SOURCES_CORE}
    ${HEADERS_CORE}
)

# Old flat #include "X.h" lines (core <-> gui cross-references, e.g.
# Interpreter.cpp including BasicGraph.h/BasicEdit.h/Settings.h, or
# MainWindow.cpp including Interpreter.h) keep resolving by exposing both
# src/core and src/gui on the same public include path. Tighten later.
target_include_directories(basic256core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)
# Only the generated parser/lexer (compiled into this target) and
# Interpreter.cpp's "basicParse.tab.h" include need these.
target_include_directories(basic256core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(basic256core PRIVATE ${PLATFORM_DEFS})
target_compile_definitions(basic256core PRIVATE $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT>)

# Phase 3 feature flags -- see the BASIC256_ENABLE_* option() block above.
# PUBLIC (not PRIVATE): Interpreter.h is a shared header that src/gui files
# also include transitively (e.g. PreferencesWin.cpp -> MainWindow.h ->
# RunController.h -> Interpreter.h) to reach QPrinter/QPrinterInfo for their
# own Print... dialogs. A PRIVATE define would only be visible while
# compiling basic256core's own sources, so Interpreter.h's #ifdef
# BASIC256_ENABLE_PRINTER would see it as *undefined* everywhere else,
# always taking the forward-declared-only branch regardless of the flag's
# real value -- exactly the bug that broke the default (flags-ON) build.
# TTS is app-layer (RunController.cpp only, never included from core), so
# it's defined directly on the basic256 exe target further down instead.
if(BASIC256_ENABLE_PROCESS)
    target_compile_definitions(basic256core PUBLIC BASIC256_ENABLE_PROCESS)
endif()
if(BASIC256_ENABLE_SERIAL)
    target_compile_definitions(basic256core PUBLIC BASIC256_ENABLE_SERIAL)
endif()
if(BASIC256_ENABLE_SQL)
    target_compile_definitions(basic256core PUBLIC BASIC256_ENABLE_SQL)
endif()
if(BASIC256_ENABLE_PRINTER)
    target_compile_definitions(basic256core PUBLIC BASIC256_ENABLE_PRINTER)
endif()
if(BASIC256_ENABLE_TCP)
    target_compile_definitions(basic256core PUBLIC BASIC256_ENABLE_TCP)
endif()

if(PLATFORM_INCLUDE_DIRS)
    target_include_directories(basic256core PRIVATE ${PLATFORM_INCLUDE_DIRS})
endif()
if(PLATFORM_LINK_DIRS)
    target_link_directories(basic256core PRIVATE ${PLATFORM_LINK_DIRS})
endif()

# Same warning/compat settings as the app target below. strcasecmp and
# _CRT_SECURE_NO_WARNINGS specifically matter here: the Bison/Flex-generated
# C sources (which use strcasecmp) are compiled into this target.
if(MSVC)
    target_compile_options(basic256core PRIVATE /W4 /permissive-)
    target_compile_definitions(basic256core PRIVATE strcasecmp=_stricmp)
    target_compile_definitions(basic256core PRIVATE $<$<COMPILE_LANGUAGE:C>:_CRT_SECURE_NO_WARNINGS>)
else()
    target_compile_options(basic256core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_options(basic256core PRIVATE -mstackrealign)
    endif()
endif()

# Qt components basic256core's sources actually #include directly: QtSql
# (DBOPEN et al.), QtSerialPort (SERIALOPEN et al.), QtPrintSupport
# (PRINTER... via QPrinter/QPrinterInfo in Interpreter.h), QtNetwork
# (NETSERVER/TCP + BasicDownloader -- Network itself is never gated, since
# BasicDownloader's NETREAD needs it regardless of BASIC256_ENABLE_TCP),
# QtMultimedia (Sound/BasicMediaPlayer). Widgets and TextToSpeech are
# GUI/app-only -- see Phase 1 log in WASM.md. Sql/SerialPort/PrintSupport
# are only linked when their BASIC256_ENABLE_* flag is on.
target_link_libraries(basic256core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Multimedia
)
if(BASIC256_ENABLE_SQL)
    target_link_libraries(basic256core PUBLIC Qt${QT_VERSION_MAJOR}::Sql)
endif()
if(BASIC256_ENABLE_SERIAL)
    target_link_libraries(basic256core PUBLIC Qt${QT_VERSION_MAJOR}::SerialPort)
endif()
if(BASIC256_ENABLE_PRINTER)
    target_link_libraries(basic256core PUBLIC Qt${QT_VERSION_MAJOR}::PrintSupport)
endif()

# -------------------------------------------------------------------
# basic256 -- app executable (src/gui + src/app), links basic256core
# -------------------------------------------------------------------

# Define the executable target
if(WIN32)
    # WIN32 = /SUBSYSTEM:WINDOWS → no console window ever allocated by Windows.
    # stdout/stderr are reconnected to the parent console in Main.cpp via AttachConsole.
    add_executable(basic256 WIN32
    ${SOURCES_GUI}
    ${HEADERS_GUI}
    ${SOURCES_APP}
    ${HEADERS_APP}
    ${RESOURCES}
    ${QM_FILES}
)
else()
    add_executable(basic256
    ${SOURCES_GUI}
    ${HEADERS_GUI}
    ${SOURCES_APP}
    ${HEADERS_APP}
    ${RESOURCES}
    ${QM_FILES}
    )
endif()

# Process TS files into QM files
qt_add_translations(basic256 TS_FILES ${TS_FILES})

if(APPLE)
    set_target_properties(basic256 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "basic256"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.basic256.basic256"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
#        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/macos/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "basic256.icns"
    )
endif()

if(UNIX AND NOT APPLE)
    set_target_properties(basic256 PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# WASM Phase 4: Emscripten link settings. EMSCRIPTEN is set by Qt's
# qt-cmake / the Emscripten CMake toolchain file when cross-compiling.
# - QT_WASM_INITIAL_MEMORY: heap growth is unsupported with pthreads, so the
#   heap must be sized up front. 512MB covers sprite-heavy fractal/demo
#   programs (the memory hogs) with headroom; revisit if real programs OOM.
# - QT_WASM_PTHREAD_POOL_SIZE: number of web workers created at startup.
#   Qt's own default is 4; a RUN click needs at least: the interpreter
#   thread, the sound system, and BasicDownloader, and exceeding the pool
#   size requires returning to the event loop first (which a RUN click
#   can't guarantee) -- 8 gives headroom over the observed minimum.
# Both are Qt CMake target properties (technology preview since Qt 6.2.4),
# translated by Qt's own build system into the equivalent Emscripten
# linker settings -- not raw -sINITIAL_MEMORY/-sPTHREAD_POOL_SIZE flags.
if(EMSCRIPTEN)
    set_target_properties(basic256 PROPERTIES
        QT_WASM_INITIAL_MEMORY "512MB"
        QT_WASM_PTHREAD_POOL_SIZE "8"
    )

    # WASM Phase 6: size pass. CMAKE_BUILD_TYPE=Release (set by
    # build_WASM.sh) gives Emscripten's default Release flags
    # (-O3 -DNDEBUG), which favors speed over size. -Oz (the last -O flag
    # on the compile/link command line wins for clang/emcc) trades some
    # interpreter speed for a materially smaller .wasm download -- an
    # explicit v1 tradeoff, acceptable since a page-load-blocking
    # multi-MB download matters more for a browser demo than raw
    # interpreter throughput. Applied to both the compile step (per-TU,
    # including basic256core's object files that get linked into
    # basic256) and the link step (affects binaryen's wasm-opt pass).
    # -flto was considered and deliberately NOT applied this pass: LTO
    # across Qt's static libraries under Emscripten's wasm-ld risks a
    # materially longer/heavier CI link, and there is no local toolchain
    # in this environment to first measure that cost -- revisit with real
    # numbers from a maintainer's machine.
    target_compile_options(basic256core PRIVATE -Oz)
    target_compile_options(basic256 PRIVATE -Oz)
    target_link_options(basic256 PRIVATE -Oz)

    # WASM Phase 7: persistence. wasm-deploy/idbfs.js (--pre-js) mounts IDBFS
    # (IndexedDB-backed) at /persist and loads it before main() runs; Main.cpp
    # points NativeFormat QSettings there so preferences / SETSETTING data
    # survive reloads (see src/core/WasmSettings.{h,cpp}). -lidbfs.js links
    # Emscripten's IDBFS backend, which is not included by default. The
    # "SHELL:" prefix keeps "--pre-js <file>" together as one two-token option
    # (CMake would otherwise be free to de-duplicate/reorder it); LINK_DEPENDS
    # relinks if the bootstrap script changes.
    target_link_options(basic256 PRIVATE
        "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/wasm-deploy/idbfs.js"
        -lidbfs.js)
    set_target_properties(basic256 PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wasm-deploy/idbfs.js")
endif()

# Derive the build's VERSION display string (About box, applicationVersion)
# from the branch being built, the same way build_installer_Windows.ps1
# derives the installer's version -- so the two never disagree again.
# - GITHUB_HEAD_REF is set for pull_request events (source branch name).
# - GITHUB_REF_NAME is set for push/release events (branch or tag name).
# - Fall back to git directly for local/non-CI runs.
if(DEFINED ENV{GITHUB_HEAD_REF} AND NOT "$ENV{GITHUB_HEAD_REF}" STREQUAL "")
    set(BASIC256_BUILD_BRANCH "$ENV{GITHUB_HEAD_REF}")
elseif(DEFINED ENV{GITHUB_REF_NAME} AND NOT "$ENV{GITHUB_REF_NAME}" STREQUAL "")
    set(BASIC256_BUILD_BRANCH "$ENV{GITHUB_REF_NAME}")
else()
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE BASIC256_BUILD_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
endif()

# Branches are named like "v2.1.Alpha04Qt6" -- strip the leading "v" to get
# the "2.1.Alpha04Qt6" version string. Anything that doesn't look like a
# version branch (main, a detached-HEAD SHA, git not found, ...) leaves
# VERSION at Version.h's hardcoded fallback instead of showing garbage.
if(BASIC256_BUILD_BRANCH MATCHES "^[vV][0-9]")
    string(REGEX REPLACE "^[vV]" "" BASIC256_BUILD_VERSION "${BASIC256_BUILD_BRANCH}")
endif()

# All target_* commands AFTER target creation
target_compile_definitions(basic256 PRIVATE ${PLATFORM_DEFS})
target_compile_definitions(basic256 PRIVATE $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT>)
if(BASIC256_BUILD_VERSION)
    target_compile_definitions(basic256 PRIVATE "VERSION=\"${BASIC256_BUILD_VERSION}\"")
endif()
if(BASIC256_ENABLE_TTS)
    target_compile_definitions(basic256 PRIVATE BASIC256_ENABLE_TTS)
endif()

# src/app on the path for cross-directory includes such as
# src/gui/MainWindow.h's "#include \"RunController.h\"". src/core and
# src/gui themselves already arrive transitively via basic256core's
# PUBLIC include dirs above.
target_include_directories(basic256 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

# NOTE: Examples/TestSuite/translations are deliberately NOT install()'d here.
# None of the CI packaging scripts (package_Linux_x86.sh,
# package_Linux_x86_AppImage.sh, package_Linux_RPi_*.sh) run `cmake
# --install` -- they copy those trees directly into the distribution folder
# themselves. A `DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256` rule here was
# already added and removed once before (see the "share/basic256" cleanup in
# BASIC256.nsi's uninstaller) because it produces a stray extra
# "basic256/" directory containing a duplicate Examples/TestSuite for anyone
# who does run `cmake --install` -- don't re-add it.
if(UNIX AND NOT APPLE)
    install(TARGETS basic256 DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(PLATFORM_INCLUDE_DIRS)
    target_include_directories(basic256 PRIVATE ${PLATFORM_INCLUDE_DIRS})
endif()
if(PLATFORM_LINK_DIRS)
    target_link_directories(basic256 PRIVATE ${PLATFORM_LINK_DIRS})
endif()

# Global compiler definitions
if(MSVC)
    target_compile_options(basic256 PRIVATE /W4 /permissive-)
    target_compile_definitions(basic256 PRIVATE strcasecmp=_stricmp)
    target_compile_definitions(basic256 PRIVATE $<$<COMPILE_LANGUAGE:C>:_CRT_SECURE_NO_WARNINGS>)
else()
    target_compile_options(basic256 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_options(basic256 PRIVATE -mstackrealign)
    endif()
endif()

# Link basic256core (which itself PUBLIC-links Core/Gui/Network/Multimedia,
# plus Sql/SerialPort/PrintSupport when their flags are on -- all propagate
# transitively) plus the GUI/app-only Qt components and OS-specific
# libraries. PrintSupport and TextToSpeech are only linked when their
# BASIC256_ENABLE_* flag is on -- see the BASIC256_QT_COMPONENTS PrintSupport
# correction above (Phase 4): src/gui's own Print... menu actions are now
# gated the same as the BASIC-language PRINTERON et al. opcodes, since the
# WASM Qt SDK doesn't ship QtPrintSupport at all.
target_link_libraries(basic256
    PRIVATE
        basic256core
        Qt${QT_VERSION_MAJOR}::Widgets
        ${PLATFORM_LIBS}
)
if(BASIC256_ENABLE_PRINTER)
    target_link_libraries(basic256 PRIVATE Qt${QT_VERSION_MAJOR}::PrintSupport)
endif()
if(BASIC256_ENABLE_TTS)
    target_link_libraries(basic256 PRIVATE Qt${QT_VERSION_MAJOR}::TextToSpeech)
endif()

# WASM Phase 4: plain add_executable() (used above, not qt_add_executable())
# does not get automatic Qt target finalization -- qt_finalize_target() is
# "ordinarily invoked as part of a call to qt_add_executable()", and without
# it a WASM build silently produces only basic256.js/basic256.wasm, missing
# the basic256.html/qtloader.js/qtlogo.svg files the browser actually needs
# to load the app. Safe to call on all platforms per Qt's own docs, but
# scoped to EMSCRIPTEN only here to keep the already-green desktop targets'
# build untouched.
if(EMSCRIPTEN)
    qt_finalize_target(basic256)
endif()

