# Verify QT version
lessThan(QT_MAJOR_VERSION, 5) {
  error( "BASIC-256 requires QT 5 or better." )
}

CONFIG(release, debug|release):message(Release build!)
CONFIG(debug, debug|release):message(Debug build!)

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG(release, debug|release):DEFINES += QT_NO_WARNING_OUTPUT

TEMPLATE						=	app
TARGET							=	basic256
# DEPENDPATH						+=	.
# QMAKE_CXXFLAGS					+=	-g
# QMAKE_CXXFLAGS					+=	-std=c++11
CONFIG							+=	 qt debug_and_release
CONFIG							+=	 c++17
CONFIG							+=	 console
# Output directories
OBJECTS_DIR						=	tmp/obj
MOC_DIR							=	tmp/moc

# Qt Modules
QT								+=	gui
QT								+=	sql
QT								+=	widgets
QT								+=	printsupport
QT								+=	serialport
QT								+=	multimedia
QT 								+= texttospeech

# Project Structure
INCLUDEPATH						+=	.
RESOURCES						+=	resources/resource.qrc
TRANSLATIONS					=	Translations/basic256_en.ts \
									Translations/basic256_de.ts \
									Translations/basic256_ru.ts \
									Translations/basic256_es.ts \
									Translations/basic256_fr.ts \
									Translations/basic256_pt.ts \
									Translations/basic256_nl.ts \
									Translations/basic256_it.ts

#### uncomment to display fprintf debugging messages
#### shows each pcode statement at the stack - really slow but really good
#### debugging the interpreter
###DEFINES += DEBUG

# -------------------------------------------------
# Install Examples
# -------------------------------------------------

examples.path = $$PREFIX/share/basic256/Examples
examples.files = $$files(Examples/*, true)

INSTALLS += examples


# -------------------------------------------------
# Install TestSuite
# -------------------------------------------------

testsuite.path = $$PREFIX/share/basic256/TestSuite
testsuite.files = $$files(TestSuite/*, true)

INSTALLS += testsuite

# Windows Specific
win32 {
	DEFINES 					+=	WIN32
	DEFINES 					+=	USEQMEDIAPLAYER
	DEFINES 					+=	USE_QT_TEXTTOSPEECH
	RC_FILE						=	resources/windows.rc
	LIBS						+=	-lole32 \
									-lws2_32 \
									-lwinmm

	# Only keep this if building 32-bit; remove for 64-bit
	QMAKE_CXXFLAGS				+=	-mstackrealign
	QMAKE_CXXFLAGS_RELEASE		+=	-mstackrealign

}

# Linux Specific
unix:!macx {
	## this is the LINUX (unix-non-mac)
	DEFINES						+=	LINUX
    DEFINES 					+=  USE_QT_TEXTTOSPEECH	
	LIBS						+=	-lm


	########
	# rules for make install
	########
	exampleFiles.files			=	./Examples
	exampleFiles.path			=	/usr/share/basic256
	INSTALLS					+=	exampleFiles

	helpHTMLFiles.files			=	./wikihelp/help
	helpHTMLFiles.path			=	/usr/share/basic256
	INSTALLS					+=	helpHTMLFiles

	transFiles.files			=	./Translations/*.qm
	transFiles.path				=	/usr/share/basic256
	INSTALLS					+=	transFiles

	# main program executable
	target.path					=	/usr/bin
	INSTALLS					+=	target

}

macx {
	# macintosh
	DEFINES						+=	MACX
	DEFINES						+=	MACX_SAY
    DEFINES 					+=  USE_QT_TEXTTOSPEECH
	ICON						=	resources/basic256.icns

	LIBS						+=	-L/opt/local/lib
	INCLUDEPATH					+=	/opt/local/include

	# Sound - QT Mobility Multimedia AudioOut
	QT							+=  texttospeech
	INCLUDEPATH					+=	QtMultimediaKit
	INCLUDEPATH					+=	QtMobility


}

exists( ./LEX/Makefile ) {
	message( Running make for ./LEX/Makefile )
	system( make -C ./LEX )
} else {
	error( Could not make LEX project - aborting... )
}


# Input
HEADERS 						+=	LEX/basicParse.tab.h
HEADERS 						+=	*.h

SOURCES 						+=	LEX/lex.yy.c
SOURCES 						+=	LEX/basicParse.tab.c
SOURCES 						+=	*.cpp
