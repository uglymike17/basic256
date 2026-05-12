/** Copyright (C) 2010, J.M.Reneau.
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



#include <QSettings>

#ifndef SETTINGSH
	#define SETTINGSH
	#define SETTINGSORG "BASIC-256 Consortium"
	#define SETTINGSAPP "BASIC-256 IDE"
	#define SETTINGSPORTABLEINI	"BASIC256_IDE.ini"

	// error reporting options
	#define SETTINGSERRORNONE 0
	#define SETTINGSERRORWARN 1
	#define SETTINGSERROR 2
	
	// ALLOW options
	#define SETTINGSALLOWNO 0
	#define SETTINGSALLOWASK 1
	#define SETTINGSALLOWYES 2
	
	// main window
    #define SETTINGSMAINGEOMETRY "Main/Geometry/"
    #define SETTINGSMAINSTATE "Main/State/"

    #define SETTINGSFONT "Main/Font/"
    #define SETTINGSTOOLBARVISIBLE "Main/ToolbarVisible/"
    #define SETTINGSTOOLBARVISIBLEDEFAULT true
    #define SETTINGSFONTDEFAULT "DejaVu Sans Mono,11,-1,5,50,0,0,0,0,0"

    
    #define SETTINGSEDITVISIBLE "Edit/Visible/"
    #define SETTINGSEDITVISIBLEDEFAULT true
    #define SETTINGSEDITWHITESPACE "Edit/Whitespace/"
    #define SETTINGSEDITWHITESPACEDEFAULT false

    #define SETTINGSOUTVISIBLE "Out/Visible/"
    #define SETTINGSOUTVISIBLEDEFAULT true
    #define SETTINGSOUTTOOLBARVISIBLE "Out/ToolbarVisible/"
    #define SETTINGSOUTTOOLBARVISIBLEDEFAULT true

    #define SETTINGSGRAPHVISIBLE "Graph/Visible/"
    #define SETTINGSGRAPHVISIBLEDEFAULT true
    #define SETTINGSGRAPHTOOLBARVISIBLE "Graph/ToolbarVisible/"
    #define SETTINGSGRAPHTOOLBARVISIBLEDEFAULT true
    #define SETTINGSGRAPHGRIDLINES "Graph/GridLines/"
    #define SETTINGSGRAPHGRIDLINESDEFAUT false
    
    #define SETTINGSVARVISIBLE "Var/Visible/"
    #define SETTINGSVARVISIBLEDEFAULT false
    
	// other IDE preferences
	#define SETTINGSIDESAVEONRUN "IDE/SaveOnRun"
	#define SETTINGSIDESAVEONRUNDEFAULT false

    // startup
    #define SETTINGSWINDOWSRESTORE "Startup/Restore"
    #define SETTINGSWINDOWSRESTOREDEFAULT true
    #define SETTINGSCHECKFORUPDATE "Startup/CheckForUpdate"
    #define SETTINGSCHECKFORUPDATEDEFAULT true

	// documentation window
	#define SETTINGSDOCSIZE "Doc/Size"
	#define SETTINGSDOCPOS "Doc/Pos"
	
	// preferences window
	#define SETTINGSPREFPOS "Pref/Pos"
	#define SETTINGSPREFPASSWORD "Pref/Password"

	// Replace window
	#define SETTINGSREPLACEPOS "Replace/Pos"

    // permissions (SYSTEM and PORTIN/PORTOUT)
	#define SETTINGSALLOWSYSTEM "Allow/System"
    #define SETTINGSALLOWSYSTEMDEFAULT SETTINGSALLOWNO
	#define SETTINGSALLOWPORT "Allow/Port"
    #define SETTINGSALLOWPORTDEFAULT SETTINGSALLOWNO
    //permissions (setsetting/getsetting)
    #define SETTINGSSETTINGSACCESS "Settings/AccesLevel"
    #define SETTINGSSETTINGSACCESSDEFAULT 0
    #define SETTINGSSETTINGSMAX "Settings/MaxKeys"
    #define SETTINGSSETTINGSMAXDEFAULT 100
    #define SETTINGSALLOWSETTING "Settings/Allow"
    #define SETTINGSALLOWSETTINGDEFAULT true
    // store user settings (setsetting/getsetting) in seperate group
    #define SETTINGSGROUPUSER "UserSettings"

	// user settings
	#define SETTINGSTYPECONV "Runtime/TypeConv"
	#define SETTINGSTYPECONVDEFAULT SETTINGSERRORNONE
	#define SETTINGSVARNOTASSIGNED "Runtime/VNA"
	#define SETTINGSVARNOTASSIGNEDDEFAULT SETTINGSERROR
	#define SETTINGSDEBUGSPEED "Runtime/DebugSpeed"
	#define SETTINGSDEBUGSPEEDDEFAULT 10
	#define SETTINGSDEBUGSPEEDMIN 1
	#define SETTINGSDEBUGSPEEDMAX 2000
	#define SETTINGSDECDIGS "Runtime/DecDigs"
	#define SETTINGSDECDIGSDEFAULT 12
    #define SETTINGSDECDIGSMIN 10
    #define SETTINGSDECDIGSMAX 16
	#define SETTINGSFLOATTAIL "Runtime/FloatTail"
	#define SETTINGSFLOATTAILDEFAULT true
    #define SETTINGSFLOATLOCALE "Runtime/FloatLocale"
    #define SETTINGSFLOATLOCALEDEFAULT false

    // sound settings
    #define SETTINGSSOUNDVOLUME "Sound/Volume"
    #define SETTINGSSOUNDVOLUMEDEFAULT 5
    #define SETTINGSSOUNDVOLUMEMIN 0
    #define SETTINGSSOUNDVOLUMEMAX 10
    #define SETTINGSSOUNDNORMALIZE "Sound/Normalize"
    #define SETTINGSSOUNDNORMALIZEDEFAULT 1000
    #define SETTINGSSOUNDNORMALIZEMIN 200
    #define SETTINGSSOUNDNORMALIZEMAX 2000
    #define SETTINGSSOUNDVOLUMERESTORE "Sound/VolumeRestore"
    #define SETTINGSSOUNDVOLUMERESTOREDEFAULT 1000
    #define SETTINGSSOUNDVOLUMERESTOREMIN 100
    #define SETTINGSSOUNDVOLUMERESTOREMAX 2000
    #define SETTINGSSOUNDSAMPLERATE "Sound/SampleRate"
    #define SETTINGSSOUNDSAMPLERATEDEFAULT 22050

	
	// espeak - library
	#define SETTINGSESPEAKVOICE "eSpeak/Voice"
	#define SETTINGSESPEAKVOICEDEFAULT "default"
	
	// espeak - statement
	#define SETTINGSESPEAKSTATEMENT "eSpeak/Statement"
	#define SETTINGSESPEAKSTATEMENTDEFAULT "espeak \"WORD\""
	
	// printersettings
	#define SETTINGSPRINTERPRINTER "Printer/Printer"
	#define SETTINGSPRINTERPDFFILE "Printer/PDFFile"
	#define SETTINGSPRINTERPAPER "Printer/Paper"
	#define SETTINGSPRINTERPAPERDEFAULT 2
	#define SETTINGSPRINTERRESOLUTION "Printer/Resolution"
	#define SETTINGSPRINTERRESOLUTIONDEFAULT 0
	#define SETTINGSPRINTERORIENT "Printer/Orient"
	#define SETTINGSPRINTERORIENTDEFAULT 0

	// store history of files as SaveHistory/0 ... SaveHistory/8 
	#define SETTINGSGROUPHIST "SaveHistory"
    #define SETTINGSGROUPHISTN 10

    // Graphics window zoom
    #define SETTINGSZOOM "View/Zoom"
    #define SETTINGSZOOMDEFAULT 1.0



    // You need an SETTINGS; statement when you are using settings in a function
    // this defines a QSettings variable named "setings" for your use
    #ifdef WIN32PORTABLE
		#include <QCoreApplication>
        #define SETTINGS QSettings settings( QCoreApplication::applicationDirPath() + "/../../Data/settings/"  + SETTINGSPORTABLEINI, QSettings::IniFormat );
    #else
        #define SETTINGS QSettings settings(SETTINGSORG, SETTINGSAPP);
    #endif


#endif
