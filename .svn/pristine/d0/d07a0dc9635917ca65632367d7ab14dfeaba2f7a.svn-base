###
### qmake for android
### requires QT 5.2.
###

### KNOWN ISSUES:  2013-01-04 j.m.reneau
### 1) editor is very flaky - this is a reported problem with QT
### at the present version.  Hopefully soon a good fix.
### https://bugreports.qt-project.org/browse/QTBUG-34616
### 2) after long periods of inactivity the UI goes black and will not
### respond - kill the application and restart.  several reports
### of this problem with many other apps have been reported


### ***********************************************************************
### ***********************************************************************
### ***********************************************************************
###
### BE SURE TO COPY %buildtemp%/src/org/qtproject/qt5/android/bindings/QtActivity.java
### to trunk/android/src/org/qtproject/qt5/android/bindings/QtActivity.java
### and make the changes as recommended by ThomasL
### as posted at: https://groups.google.com/forum/#!searchin/android-qt/context/android-qt/rTz8d6IzgEM/7c6YEei47GEJ
###
### add the following to QtActivity.java:
### private static QtActivity QtActivityInstance;
### public static QtActivity getQtActivityInstance()
### {
### return QtActivity.QtActivityInstance;
### }
###
### and in QtActivity.java loadApplication() method, we added this
### QtActivity.QtActivityInstance = this;
###
### Now the "Context can be exposed by
### import org.qtproject.qt5.android.bindings.QtActivity;
### by calling the QtActivity.getQtActivityInstance() method
###
### ***********************************************************************
### ***********************************************************************
### ***********************************************************************

QT       += core gui widgets printsupport sql

###message($$[QT_VERSION])

DEFINES                                         +=	ANDROID

DEFINES                                         +=      DISABLESQLITE
DEFINES                                         +=      USEQSOUND

TEMPLATE					=	app
TARGET						=	BASIC256
DEPENDPATH					+=	.
INCLUDEPATH					+=	.
QMAKE_CXXFLAGS                                  +=	-g
CONFIG						+=	qt debug_and_release
OBJECTS_DIR					=	tmp/obj
MOC_DIR						=	tmp/moc
ANDROID_PACKAGE_SOURCE_DIR                      =       $$PWD/android

RESOURCES					+=	resources/resource.qrc
RC_FILE						=	resources/basic256.rc
TRANSLATIONS                                    =	Translations/basic256_en.ts \
                                                        Translations/basic256_de.ts \
                                                        Translations/basic256_ru.ts \
                                                        Translations/basic256_es.ts \
                                                        Translations/basic256_fr.ts \
                                                        Translations/basic256_pt.ts \
                                                        Translations/basic256_nl.ts

	########
	# TTS control - How Say statement works
        # in Android the ANDROID define handles this setting
	########

	########
	# Sound class - How Sound statement works
	########

	# Sound - Option 1 - QT Mobility Multimedia AudioOut
	DEFINES					+=	SOUND_QMOBILITY
        QT					+=	multimedia
	INCLUDEPATH				+=	QtMultimediaKit
	INCLUDEPATH				+=	QtMobility
	CONFIG					+=	mobility
	MOBILITY				+=	multimedia

exists( ./LEX/Makefile ) {
	message( Running make for ./LEX/Makefile )
	system( make -C ./LEX )
} else { 
        error( Could not make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS						+=	android/AndroidTTS.h

HEADERS 					+= *.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	android/AndroidTTS.cpp

SOURCES 					+= *.cpp

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/version.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java \
    android/AndroidManifest.xml \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/version.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java

