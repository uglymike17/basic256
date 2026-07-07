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

# Find Qt6 components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    PrintSupport
    SerialPort
    Multimedia
    TextToSpeech
    LinguistTools
    Network
)

set(BASIC256_QT_COMPONENTS
    Core
    Gui
    Widgets
    Sql
    PrintSupport
    SerialPort
    Multimedia
    TextToSpeech
    LinguistTools
    Network
)

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
    src/core/Interpreter.cpp
    src/core/md5.cpp
    src/core/Sleeper.cpp
    src/core/Sound.cpp
    src/core/Stack.cpp
    src/core/Variables.cpp
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
    src/core/Interpreter.h
    src/core/md5.h
    src/core/Sleeper.h
    src/core/Sound.h
    src/core/Stack.h
    src/core/Variables.h
    src/core/Version.h
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
# (NETSERVER/TCP + BasicDownloader), QtMultimedia (Sound/BasicMediaPlayer).
# Widgets and TextToSpeech are GUI/app-only -- see Phase 1 log in WASM.md.
# Phase 3 gates Sql/SerialPort/PrintSupport/Network(server)/Multimedia
# (in-memory)/TTS behind BASIC256_ENABLE_* flags for the browser build.
target_link_libraries(basic256core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::SerialPort
        Qt${QT_VERSION_MAJOR}::PrintSupport
        Qt${QT_VERSION_MAJOR}::Multimedia
)

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

# Link basic256core (which itself PUBLIC-links Core/Gui/Network/Sql/
# SerialPort/PrintSupport/Multimedia -- those propagate transitively) plus
# the GUI/app-only Qt components and OS-specific libraries.
target_link_libraries(basic256
    PRIVATE
        basic256core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::TextToSpeech
        ${PLATFORM_LIBS}
)

