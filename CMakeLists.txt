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

option(BASIC256_ENABLE_PROCESS "Enable SYSTEM (QProcess)" ON)
option(BASIC256_ENABLE_SERIAL  "Enable SERIALOPEN et al. (QSerialPort)" ON)
option(BASIC256_ENABLE_SQL     "Enable DBOPEN et al. (QtSql)" ON)
option(BASIC256_ENABLE_PRINTER "Enable PRINTERON et al. (QtPrintSupport)" ON)
option(BASIC256_ENABLE_TCP     "Enable NETLISTEN/NETCONNECT et al. (QTcpServer/QTcpSocket)" ON)
option(BASIC256_ENABLE_TTS     "Enable SAY (QTextToSpeech)" ON)

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
    src/core/MediaPath.cpp
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
    src/core/MediaPath.h
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
    # Must come before "elseif(UNIX AND NOT APPLE)" below -- Emscripten's
    # CMake toolchain sets UNIX to true (it's a POSIX-like target), so without
    # this branch a WASM build silently falls into the Linux branch and
    # defines LINUX. That pulls in a Linux-only OSS sound header
    # (RunController.cpp's <sys/soundcard.h>) and makes OP_OSTYPE misreport the
    # browser as Linux. No platform defs/libs needed for wasm.
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

target_include_directories(basic256core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)

target_include_directories(basic256core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(basic256core PRIVATE ${PLATFORM_DEFS})
target_compile_definitions(basic256core PRIVATE $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT>)

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

if(EMSCRIPTEN)
    set_target_properties(basic256 PROPERTIES
        QT_WASM_INITIAL_MEMORY "512MB"
        QT_WASM_PTHREAD_POOL_SIZE "8"
    )

    target_compile_options(basic256core PRIVATE -Oz)
    target_compile_options(basic256 PRIVATE -Oz)
    target_link_options(basic256 PRIVATE -Oz)
    target_link_options(basic256 PRIVATE
        "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/wasm-deploy/idbfs.js"
        -lidbfs.js)
    set_target_properties(basic256 PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wasm-deploy/idbfs.js")
endif()

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
# version branch (main, a detached-HEAD SHA, git not found, ...) falls back
# to PROJECT_VERSION -- the single source of truth declared in project()
# at the top of this file -- so no build depends on the hand-edited string
# in Version.h, which is exactly the drift this derivation exists to stop.
if(BASIC256_BUILD_BRANCH MATCHES "^[vV][0-9]")
    string(REGEX REPLACE "^[vV]" "" BASIC256_BUILD_VERSION "${BASIC256_BUILD_BRANCH}")
else()
    set(BASIC256_BUILD_VERSION "${PROJECT_VERSION}")
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

target_include_directories(basic256 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

# NOTE: Examples/TestSuite/translations are deliberately NOT install()'d here.
# None of the CI packaging scripts run `cmake --install` -- they copy those
# trees directly into the distribution folder themselves. A
# `DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256` rule here was already added
# and removed once before because it produces a stray extra "basic256/"
# directory containing a duplicate Examples/TestSuite for anyone who does run
# `cmake --install` -- don't re-add it.
if(UNIX AND NOT APPLE)
    install(TARGETS basic256 DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Bundled module library: copy Modules/ next to the built executable so that a
# plain build tree resolves `include "math.kbs"` the same way a shipped build
# does (the interpreter searches <exe dir>/Modules -- see LEX/basicParse.l). The
# packaging scripts place Modules/ beside the executable in each distribution;
# this mirrors that for developers running straight from the build directory.
# Emscripten has no native filesystem for include to read, so skip it there.
if(NOT EMSCRIPTEN)
    add_custom_command(TARGET basic256 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Modules
            $<TARGET_FILE_DIR:basic256>/Modules
        COMMENT "Copying Modules/ next to the executable"
        VERBATIM
    )
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

if(EMSCRIPTEN)
    qt_finalize_target(basic256)
endif()

