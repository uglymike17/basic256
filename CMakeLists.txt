cmake_minimum_required(VERSION 3.16)
project(basic256 VERSION 1.0 LANGUAGES C CXX)

# Force C++17 standard [cite: 2]
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatic handling of Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 5 components [cite: 1, 2]
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Sql 
    PrintSupport 
    SerialPort 
    Multimedia 
    TextToSpeech 
    LinguistTools 
    REQUIRED
)

# Global compiler definitions
# (Uncomment the line below if you need the interpreter debug messages)
# add_compile_definitions(DEBUG)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
    add_compile_definitions(QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT)
endif()

# Include paths [cite: 2]
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/LEX
)

# -------------------------------------------------------------------
# Source Files & Wildcards 
# -------------------------------------------------------------------

# Note: Explicitly listing files is best practice in CMake, 
# but file(GLOB) mimics the behavior of the original *.cpp / *.h masks 
file(GLOB SOURCES_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB HEADERS_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

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

# Qt Resources [cite: 3]
set(RESOURCES resources/resource.qrc)

# Translations [cite: 3]
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
# Process TS files into QM files
qt5_add_translation(QM_FILES ${TS_FILES})

# -------------------------------------------------------------------
# Platform Specific Settings 
# -------------------------------------------------------------------

if(WIN32)
    add_compile_definitions(WIN32 USEQMEDIAPLAYER USE_QT_TEXTTOSPEECH)
    
    # Windows Resource File [cite: 3]
    list(APPEND SOURCES resources/windows.rc)
    
    # Platform Libraries [cite: 3]
    set(PLATFORM_LIBS ole32 ws2_32 winmm)
    
    # 32-bit stack realignment flags 
    # Remove these or wrap in a check if you are targeting 64-bit only 
    add_compile_options(-mstackrealign) 

elseif(APPLE)
    add_compile_definitions(MACX MACX_SAY USE_QT_TEXTTOSPEECH)
    
    # App icon 
    list(APPEND SOURCES resources/basic256.icns)
    set_source_files_properties(resources/basic256.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    
    # Mac include paths & libraries 
    include_directories(/opt/local/include)
    link_directories(/opt/local/lib)

elseif(UNIX AND NOT APPLE)
    add_compile_definitions(LINUX USE_QT_TEXTTOSPEECH)
    
    set(PLATFORM_LIBS m)
    
    # Installation rules 
    install(DIRECTORY ./Examples DESTINATION /usr/share/basic256)
    install(DIRECTORY ./wikihelp/help DESTINATION /usr/share/basic256)
    
    # Install generated .qm translation files 
    install(FILES ${QM_FILES} DESTINATION /usr/share/basic256)
endif()

# -------------------------------------------------------------------
# Target Configuration [cite: 2]
# -------------------------------------------------------------------

# Define the executable target [cite: 2]
add_executable(basic256 
    ${SOURCES} 
    ${HEADERS} 
    ${RESOURCES}
    ${QM_FILES}
)

# Link Qt libraries and OS specific libraries [cite: 2]
target_link_libraries(basic256
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
        Qt5::PrintSupport
        Qt5::SerialPort
        Qt5::Multimedia
        Qt5::TextToSpeech
        ${PLATFORM_LIBS}
)

# Linux target installation path 
if(UNIX AND NOT APPLE)
    install(TARGETS basic256 DESTINATION /usr/bin)
endif()