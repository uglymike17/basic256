cmake_minimum_required(VERSION 3.21)
project(basic256 VERSION 1.0 LANGUAGES C CXX)

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

# Find Qt 5/6 components 
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    PrintSupport
    SerialPort
    Multimedia
    TextToSpeech
    LinguistTools
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    PrintSupport
    SerialPort
    Multimedia
    TextToSpeech
    LinguistTools
)



# -------------------------------------------------------------------
# Source Files & Wildcards 
# -------------------------------------------------------------------

# Note: Explicitly listing files is best practice in CMake, 
# but file(GLOB) mimics the behavior of the original *.cpp / *.h masks 
file(GLOB SOURCES_MAIN CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB HEADERS_MAIN CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# Add the explicit LEX files 
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX/lex.yy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX/basicParse.tab.c
    ${SOURCES_MAIN}
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX/basicParse.tab.h
    ${HEADERS_MAIN}
)

# Qt Resources 
set(RESOURCES resources/resource.qrc)

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
# Process TS files into QM files When switching to Qt6, use qt_add_translations() instead of qt_add_translation() 
qt_add_translation(QM_FILES ${TS_FILES})

# -------------------------------------------------------------------
# Platform Specific Settings 
# -------------------------------------------------------------------

if(WIN32)
    set(PLATFORM_DEFS WIN32 USEQMEDIAPLAYER USE_QT_TEXTTOSPEECH)
    
    # Windows Resource File
    list(APPEND SOURCES resources/windows.rc)
    
    # Platform Libraries
    set(PLATFORM_LIBS ole32 ws2_32 winmm)
    
elseif(APPLE)
    set(PLATFORM_DEFS MACX MACX_SAY USE_QT_TEXTTOSPEECH)
    set(PLATFORM_LIBS "")
    set(PLATFORM_INCLUDE_DIRS /opt/local/include)  # ← store, don't apply yet
    set(PLATFORM_LINK_DIRS /opt/local/lib)
    
    # App icon 
    list(APPEND SOURCES resources/basic256.icns)
    set_source_files_properties(resources/basic256.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

elseif(UNIX AND NOT APPLE)
    set(PLATFORM_DEFS LINUX USE_QT_TEXTTOSPEECH)
    set(PLATFORM_LIBS m)
endif()

# -------------------------------------------------------------------
# Target Configuration 
# -------------------------------------------------------------------

# Define the executable target 
qt_add_executable(basic256 
    ${SOURCES} 
    ${HEADERS} 
    ${RESOURCES}
    ${QM_FILES}
)

# All target_* commands AFTER target creation
target_compile_definitions(basic256 PRIVATE ${PLATFORM_DEFS})
target_compile_definitions(basic256 PRIVATE $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT>)

target_include_directories(basic256 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX
)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS basic256 DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY ./Examples DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256)
    install(DIRECTORY ./wikihelp/help DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256)
    install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/basic256)
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

# Link Qt libraries and OS specific libraries 
target_link_libraries(basic256
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::PrintSupport
        Qt${QT_VERSION_MAJOR}::SerialPort
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::TextToSpeech
        ${PLATFORM_LIBS}
)

