/** Copyright (C) 2006, Ian Paul Larsen, Florin Oprea
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/



#include <iostream>
#include <locale.h>
#if !defined(WIN32) || defined(__MINGW32__)
#include <unistd.h>
#endif
#ifdef Q_OS_WIN
#  ifndef NOMINMAX
#  define NOMINMAX
#  endif
#  include <windows.h>
#endif

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QStatusBar>
#include <QtPlugin>
#include <QTranslator>
#include <QMetaType>
#include <QDebug>

#include "Settings.h"
#include "Version.h"
#include "MainWindow.h"
#include "BasicEdit.h"


//Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

extern MainWindow * mainwin;
extern BasicEdit * editwin;


#if defined(WIN32) && !defined(WIN32PORTABLE)
static void associateFileTypes(const QStringList &fileTypes)
{
    QString displayName = QGuiApplication::applicationDisplayName();
    QString filePath    = QCoreApplication::applicationFilePath();
    QString fileName    = QFileInfo(filePath).fileName();

    // Application entry: registers the exe itself and its supported types
    QSettings s("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\" + fileName,
                QSettings::NativeFormat);
    s.setValue("FriendlyAppName", displayName);

    s.beginGroup("SupportedTypes");
        foreach (const QString &fileType, fileTypes)
            s.setValue(fileType, QString());
    s.endGroup();                   // SupportedTypes

    s.beginGroup("shell");
        s.beginGroup("open");
            s.setValue("FriendlyAppName", displayName);
            s.beginGroup("Command");
                s.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath)
                                           + QString("\" \"%1\""));
            s.endGroup();           // Command
        s.endGroup();               // open
    s.endGroup();                   // shell

    // Associate .kbs extension → BASIC-256 (per-user, no admin required)
    QSettings ss("HKEY_CURRENT_USER\\Software\\Classes\\",
                 QSettings::NativeFormat);

    ss.beginGroup(".kbs");
        ss.setValue(".", fileName + QString(".kbs"));
    ss.endGroup();                  // .kbs

    ss.beginGroup(fileName + QString(".kbs"));
        ss.beginGroup("shell");

            ss.beginGroup("open");
                ss.beginGroup("Command");
                    ss.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath)
                                               + QString("\" \"%1\""));
                ss.endGroup();      // Command
            ss.endGroup();          // open

            ss.beginGroup("run");
                ss.setValue(".", QString("&Run"));
                ss.beginGroup("Command");
                    ss.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath)
                                               + QString("\" -r \"%1\""));
                ss.endGroup();      // Command
            ss.endGroup();          // run

        ss.endGroup();              // shell
    ss.endGroup();                  // fileName+".kbs"
}
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    // With /SUBSYSTEM:WINDOWS there is no console window, but if the user
    // launched us from cmd.exe or a batch file we should still be able to
    // write --help / --version / error output to that existing window.
    // AttachConsole fails silently when there is no parent console
    // (e.g. double-click from Explorer or a -g / -t shortcut) — exactly
    // what we want.
    //
    // Skip this entirely if stdout is already redirected to a pipe or file
    // (e.g. `basic256.exe -s script.kbs > out.txt`, or a CI runner capturing
    // output via a pipe): AttachConsole+freopen would reopen stdout onto the
    // parent's console device (CONOUT$), silently discarding that
    // redirection so nothing the process prints ever reaches the file/pipe
    // the caller was trying to capture.
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD stdoutType = (hStdOut != NULL && hStdOut != INVALID_HANDLE_VALUE)
        ? GetFileType(hStdOut) : FILE_TYPE_UNKNOWN;
    bool stdoutAlreadyRedirected = (stdoutType == FILE_TYPE_DISK || stdoutType == FILE_TYPE_PIPE);
    if (!stdoutAlreadyRedirected && AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$",  "r", stdin);
    }
#endif

    // Enable per-monitor DPI awareness so Windows text-size settings are
    // reflected in Qt's system font. Must be set before QApplication is created.
    // QT_ENABLE_HIGHDPI_SCALING is the non-deprecated Qt 5.14+ replacement for
    // the Qt::AA_EnableHighDpiScaling attribute that was removed in commit e673dad.
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QApplication qapp(argc, argv);

#ifdef Q_OS_WIN
    // Qt 5 reads QApplication::font() from SPI_GETICONTITLELOGFONT (icon
    // title font). On some Windows builds this source does not reliably
    // reflect the text-size accessibility setting (TextScaleFactor), while
    // SPI_GETNONCLIENTMETRICS::lfMessageFont always does (Windows 10 RS4+).
    // Explicitly override the default app font so that toolbar buttons and
    // every other widget that inherits the app font scale correctly.
    {
        NONCLIENTMETRICSW ncm = {};
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            HDC hdc = GetDC(nullptr);
            const int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
            if (hdc) ReleaseDC(nullptr, hdc);
            const LOGFONTW &lf = ncm.lfMessageFont;
            if (lf.lfHeight < 0 && dpi > 0) {
                QFont f(QString::fromWCharArray(lf.lfFaceName));
                f.setPointSizeF(-lf.lfHeight * 72.0 / dpi);
                qapp.setFont(f);
            }
        }
    }
#endif

    qRegisterMetaType<std::vector<std::vector<double>>>("std::vector<std::vector<double>>");
    int guimode = 0;		// 0=normal, 1- r option, 2- app option, 3=-g graph-only, 4=-t text-only, 5=-s silent (no GUI)
    bool fullScreen = false;
    QString localecode;		// either lang or the system localle - stored on mainwin for help display

    QCoreApplication::setOrganizationName(SETTINGSORG);
    QCoreApplication::setApplicationName(SETTINGSAPP);
    QCoreApplication::setApplicationVersion(VERSION);

#if defined(WIN32) && !defined(WIN32PORTABLE)
    associateFileTypes(QStringList(".kbs"));
#endif

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("BASIC-256 is an easy to use version of BASIC designed to teach anybody (especially middle and high-school students) the basics of computer programming."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", QObject::tr("BASIC file in format <name.kbs>"));
    // Run option
    QCommandLineOption setRunOption(QStringList() << "r" << "run", QObject::tr("Run specified file in IDE"));
    parser.addOption(setRunOption);
    // Application option
    QCommandLineOption setAppOption(QStringList() << "a" << "app" << "application", QObject::tr("Run specified file as an application (No Edit window)"));
    parser.addOption(setAppOption);
    // Graphics-only option 
    QCommandLineOption setGraphOption(QStringList() << "g" << "graph", QObject::tr("Run specified file showing only the Graphics Output window."));
    parser.addOption(setGraphOption);
    // Text-only option   (NEW)
    QCommandLineOption setTextOption(QStringList() << "t" << "text", QObject::tr("Run specified file showing only the Text Output window."));
    parser.addOption(setTextOption);
    // Silent option (no GUI at all)
    QCommandLineOption setSilentOption(QStringList() << "s" << "silent",
        QObject::tr("Run specified file with no GUI at all: PRINT output goes to stdout, "
                    "errors go to stderr, and the process exit code reflects success/failure. "
                    "Requires a filename argument. Cannot be combined with -r, -a, -g, or -t "
                    "(that combination is a hard error)."));
    parser.addOption(setSilentOption);
    // Full-screen option
    QCommandLineOption setFullOption(QStringList() << "f" << "full",
        QObject::tr("With -r, -a, -g, or -t: resize window to full available screen. "
                    "Ignored when used without -r, -a, -g, or -t."));
    parser.addOption(setFullOption);
    // Language option
    QCommandLineOption setLanguageOption(QStringList() << "l" << "lang" << "language", QObject::tr("Set language to <language>."), QObject::tr("language"));
    parser.addOption(setLanguageOption);
    // Process the actual command line arguments given by the user
    parser.process(qapp);
    fullScreen = parser.isSet(setFullOption);
    const QStringList args = parser.positionalArguments();

    // file is args.at(0)
    QString fileName;
    if(args.size()>0)
    if (!args.at(0).isEmpty())
        fileName=args.at(0);

    localecode = parser.value(setLanguageOption);
    if(localecode.isEmpty())
        localecode = QLocale::system().name();

    // -s/--silent is mutually exclusive with -r, -a, -g, and -t: fail fast,
    // before any window/interpreter setup begins.
    if (parser.isSet(setSilentOption)) {
        if (parser.isSet(setRunOption) || parser.isSet(setAppOption) ||
            parser.isSet(setGraphOption) || parser.isSet(setTextOption)) {
            std::cerr << QObject::tr(
                "Error: -s/--silent cannot be combined with -r, -a, -g, or -t.")
                .toStdString() << std::endl;
            return 1;
        }
        if (fileName.isEmpty()) {
            std::cerr << QObject::tr(
                "Error: -s/--silent requires a filename argument.")
                .toStdString() << std::endl;
            return 1;
        }
    }

    if (parser.isSet(setRunOption) and !fileName.isEmpty()) {
        guimode=1;
    }

    if (parser.isSet(setAppOption) and !fileName.isEmpty()) {
        guimode=2;
    }
    if (parser.isSet(setGraphOption) && !fileName.isEmpty()) {
        guimode = 3;
    }
    if (parser.isSet(setTextOption) && !fileName.isEmpty()) {
        guimode = 4;
    }
    if (parser.isSet(setSilentOption)) {
        guimode = GUISTATESILENT;
        // Suppress Qt's own internal diagnostic output (e.g. "Painter not active"
        // warnings from graphics calls that are intentionally no-ops in this mode)
        // so stderr carries only script/runtime errors for automated test runners.
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) {});
    }

    QTranslator qtTranslator;
#ifdef WIN32
    qtTranslator.load("qt_" + localecode);
#else
    qtTranslator.load("qt_" + localecode, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
    qapp.installTranslator(&qtTranslator);

    QTranslator kbTranslator;
#ifdef WIN32
    kbTranslator.load("basic256_" + localecode, qApp->applicationDirPath() + "/Translations/");
#else
    bool ok;
    ok = kbTranslator.load("basic256_" + localecode, "/usr/share/basic256/");
    if (!ok) ok = kbTranslator.load("basic256_" + localecode, "/usr/local/share/basic256/");  // alternative location
#endif
    qapp.installTranslator(&kbTranslator);

    MainWindow mainwin(nullptr, Qt::WindowFlags(), localecode, guimode, fullScreen);
    mainwin.setObjectName( "mainwin" );
    // --silent: MainWindow and its child windows (Edit/Graphics/Text Output) are
    // still constructed internally (the interpreter is wired to them), but they
    // are never shown, so no window ever appears on screen.
    if (guimode != GUISTATESILENT) {
        mainwin.statusBar()->showMessage(QObject::tr("Ready."));
        mainwin.show();
    }

    bool loaded=false;
 
#ifdef ANDROID
    // android - dont load initial file but set default folder to sdcard if exists
    if (QDir("/storage/sdcard0").exists()) {
        QDir::setCurrent("/storage/sdcard0");
    }
#else
    // load initial file and optionally start
    if (!fileName.isEmpty()) {
            QFileInfo fi(fileName);
        loaded = mainwin.loadFile(fi.absoluteFilePath());
        if (guimode == GUISTATESILENT && !loaded) {
            std::cerr << QObject::tr("Error: unable to load file '%1'.").arg(fileName).toStdString() << std::endl;
            return 1;
        }
        if(guimode!=0 && !loaded){
            return 0;
        }
        mainwin.ifGuiStateRun();
    }else if(guimode!=0){
        return 0;
    }
#endif

    setlocale(LC_ALL,"C");

    if(!loaded) mainwin.newProgram();
    qDebug() << "[close-debug] Main.cpp: entering qapp.exec()";
    int rc = qapp.exec();
    qDebug() << "[close-debug] Main.cpp: qapp.exec() returned" << rc << "-- about to destroy mainwin and return from main()";
    return rc;
}

