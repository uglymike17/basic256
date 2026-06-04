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
    QString filePath = QCoreApplication::applicationFilePath();
    QString fileName = QFileInfo(filePath).fileName();

    //Application
    QSettings s("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\" + fileName, QSettings::NativeFormat);
    s.setValue("FriendlyAppName", displayName);
    s.beginGroup("SupportedTypes");
    foreach (const QString& fileType, fileTypes)
        s.setValue(fileType, QString());
    s.endGroup();
    s.beginGroup("shell");
    s.beginGroup("open");
    s.setValue("FriendlyAppName", displayName);
    s.beginGroup("Command");
    s.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath) + QString("\" \"%1\""));


    //Associate .kbs files to BASIC-256 (Windows)
    QSettings ss("HKEY_CURRENT_USER\\Software\\Classes\\" , QSettings::NativeFormat);
    ss.beginGroup(".kbs");
    ss.setValue(".",fileName + QString(".kbs"));
    ss.endGroup();
    ss.beginGroup(fileName + QString(".kbs"));
    ss.beginGroup("shell");
    ss.beginGroup("open");
    ss.beginGroup("Command");
    ss.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath) + QString("\" \"%1\""));
    ss.endGroup();
    ss.endGroup();
    ss.beginGroup("run");
    ss.setValue(".", QString("&Run"));
    ss.beginGroup("Command");
    ss.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath) + QString("\" -r \"%1\""));
}
#endif




int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication qapp(argc, argv);
    QFont f = qapp.font();
    f.setPointSize(9);
    qapp.setFont(f);
    qRegisterMetaType<std::vector<std::vector<double>>>("std::vector<std::vector<double>>");
    int guimode = 0;		// 0=normal, 1- r option, 2- app option
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
    QCommandLineOption setRunOption(QStringList() << "r" << "run", QObject::tr("Run specified file"));
    parser.addOption(setRunOption);
    // Application option
    QCommandLineOption setAppOption(QStringList() << "a" << "app" << "application", QObject::tr("Run specified file as an application"));
    parser.addOption(setAppOption);
    // Language option
    QCommandLineOption setLanguageOption(QStringList() << "l" << "lang" << "language", QObject::tr("Set language to <language>."), QObject::tr("language"));
    parser.addOption(setLanguageOption);
    // Process the actual command line arguments given by the user
    parser.process(qapp);
    const QStringList args = parser.positionalArguments();

    // file is args.at(0)
    QString fileName;
    if(args.size()>0)
    if (!args.at(0).isEmpty())
        fileName=args.at(0);

    localecode = parser.value(setLanguageOption);
    if(localecode.isEmpty())
        localecode = QLocale::system().name();

    if (parser.isSet(setRunOption) and !fileName.isEmpty()) {
        guimode=1;
    }

    if (parser.isSet(setAppOption) and !fileName.isEmpty()) {
        guimode=2;
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

    MainWindow mainwin(0, 0, localecode, guimode);
    mainwin.setObjectName( "mainwin" );
    mainwin.statusBar()->showMessage(QObject::tr("Ready."));
    mainwin.show();

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
    return qapp.exec();
}

