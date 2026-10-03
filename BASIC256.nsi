;   BASIC256.nsi


 ;   Modification History
 ;   date......   programmer...   description...
 ;   2008-09-01   j.m.reneau         original coding
 ;   2012-12-02   j.m.reneau         changed to require the unsis version for unicode support
 ;                                   More information at http://www.scratchpaper.com/
 ;   2013-11-12   j.m.reneau         major rewrite for 1.0.0.0 and change to QT5.1
 ;   2014-06-03   j.m.reneau         updated to qt5.3 and new media plugins
 ;   2015-12-30   j.m.reneau         updated to QT 5.7
 #   2016-10-31   j.m.reneau         updated to qt 5.7
 ;   2020-04-26   j.m.reneau         updated to qt 5.14.2
 ;   2020-09-05   j.m.reneau         Updated to Qt 5.15.0
 ;   2026-06-18   m. vilain          Updated to Qt 5.15.2, _Win64_Install
 ;   2026-06-24   m. vilain          Add dist/Examples/TestSuite uninstall; VC++ redist check
 ;   2026-10-03   m. vilain          Ask to close a running BASIC256; build date from makensis

 ;   On Windows use notepad or notepad++.

 !include nsDialogs.nsh

 ; VERSION is passed in from the CI build via "makensis /DVERSION=...",
 ; The hardcoded fallback below is only used for ad-hoc local makensis runs.
 !ifndef VERSION
     !define VERSION "2.1.Alpha04Qt6"
 !endif
 ; the date makensis is run, so every build carries its own
 !define /date VERSIONDATE "%Y-%m-%d"
 !define SDK_BIN "$%QT_DIR%\bin"
 !define SDK_LIB "$%QT_DIR%\lib"
 !define SDK_PLUGINS "$%QT_DIR%\plugins"

 ; Path to the Visual C++ 2015-2022 Redistributable (x64).
 ; In CI this is downloaded by build_installer_Windows.ps1 and placed next to the .nsi.
 !define VCREDIST "vc_redist.x64.exe"

 var customDialog
 var customLabel0
 var customLabel1
 var customImage
 var customImageHandle

 !include "x64.nsh"

 Function .onInit
     ; $PLUGINSDIR is the one directory the installer can count on existing
     ; on the machine it runs on.  NSD_SetImage LoadImage()s its argument at
     ; run time, so the welcome bitmap has to be unpacked to a real path
     ; first -- the old resources\images\... argument named a path in the
     ; source tree, which exists on no user's machine, so the welcome page
     ; had always come up with no picture on it at all.
     InitPluginsDir
     File /oname=$PLUGINSDIR\welcome.bmp "resources\images\basic256.bmp"
     ${IfNot} ${RunningX64}
         MessageBox MB_OK "This installer requires 64-bit Windows."
         Abort
     ${EndIf}
     SetRegView 64
 FunctionEnd

 Function customPage

 	 nsDialogs::Create /NOUNLOAD 1018
 	 Pop $customDialog

 	 ${If} $customDialog == error
 	 	 Abort
 	 ${EndIf}

 	 ${NSD_CreateBitmap} 10 10 128 128 ""
 	 Pop $customImage
 	 ${NSD_SetImage} $customImage $PLUGINSDIR\welcome.bmp $customImageHandle	; drawn at its native size and clipped, never scaled, so the .bmp is exactly the 128x128 of the control above, on the dialog face colour

 	 ${NSD_CreateLabel} 152 12 -162 32 "BASIC256 ${VERSION} (${VERSIONDATE})"
 	 Pop $customLabel0
 	 ${NSD_CreateLabel} 152 46 -162 110 "This installer will load BASIC256.  Previous versions will be overwritten and any saved files in the program folder may or may not be preserved."
 	 Pop $customLabel1

 	 nsDialogs::Show
 FunctionEnd

 Function customPageLeave
 	 ${NSD_FreeImage} $customImageHandle
 FunctionEnd

 ; Windows will not let a running program's .exe be replaced or deleted, so
 ; a BASIC256 still open from the install folder made the copy stop with a
 ; bare "Error opening file for writing".  Opening the exe for writing fails
 ; the same way while it runs; checking that up front lets the page ask the
 ; user to close it.  Retry checks again, Cancel stays on the page.
 !macro CheckNotRunning
     ${If} ${FileExists} "$INSTDIR\basic256.exe"
         ${Do}
             ClearErrors
             FileOpen $0 "$INSTDIR\basic256.exe" a
             ${IfNot} ${Errors}
                 FileClose $0
                 ${Break}
             ${EndIf}
             ${If} ${Cmd} `MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "BASIC256 is running from $INSTDIR.$\n$\nClose every BASIC256 window, then click Retry." /SD IDCANCEL IDCANCEL`
                 Abort
             ${EndIf}
         ${Loop}
     ${EndIf}
 !macroend

 Function directoryLeave
     !insertmacro CheckNotRunning
 FunctionEnd

 Function un.confirmLeave
     !insertmacro CheckNotRunning
 FunctionEnd


 ;   The name of the installer
 Name "BASIC256 ${VERSION} (${VERSIONDATE})"

 ;   The file to write
 OutFile BASIC256-${VERSION}_Win64_Install.exe

 ;   The default installation directory
 InstallDir $PROGRAMFILES64\BASIC256

 ;   Registry key to check for directory (so if you install again, it will
 ;   overwrite the old one automatically)
 InstallDirRegKey HKLM "Software\BASIC256" "Install_Dir"

 ;   Request application privileges for Windows Vista
 RequestExecutionLevel admin

 InstType "Full"
 InstType "Minimal"
 ;---------------------------------

 ;   Pages

 Page custom customPage customPageLeave ": BASIC256 Welcome"
 Page license
 LicenseData "license.txt"
 Page components
 Page directory "" "" directoryLeave
 Page instfiles

 UninstPage uninstConfirm "" "" un.confirmLeave
 UninstPage instfiles

 ;---------------------------------

 ; ── Visual C++ 2015-2022 Redistributable (x64) ──────────────────────────────
 ; Hidden section — always runs first, before any other section.
 ; Checks whether the VC++ runtime is already installed; if not, installs it
 ; silently so BASIC256 (built with MSVC 2022) runs on any Windows 10/11 machine.
 Section "-VCRuntime"

     SetRegView 64

     ; The 2015-2022 redist sets Installed=1 under this key on all VS versions
     ; from 2015 through 2022 (all share the same v14.x runtime).
     ReadRegDWORD $0 HKLM \
         "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"

     ${If} $0 != 1
         DetailPrint "Visual C++ 2015-2022 Redistributable (x64) not found — installing..."
         SetOutPath $TEMP
         File "${VCREDIST}"
         ; /install   = full install (not repair)
         ; /passive   = progress bar, no user interaction
         ; /norestart = never force a reboot (we handle code 3010 below)
         ExecWait '"$TEMP\${VCREDIST}" /install /passive /norestart' $0
         Delete "$TEMP\${VCREDIST}"
         ${If} $0 != 0
         ${AndIf} $0 != 3010   ; 3010 = success, reboot required — still OK
             MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "The Visual C++ 2015-2022 Redistributable could not be installed \
(error code: $0).$\n$\nBASIC256 may not start without it. You can download \
it manually from:$\nhttps://aka.ms/vs/17/release/vc_redist.x64.exe"
         ${EndIf}
         ${If} $0 == 3010
             ; Flag that a reboot will be needed — NSIS will offer it at the end
             SetRebootFlag true
         ${EndIf}
     ${Else}
         DetailPrint "Visual C++ 2015-2022 Redistributable (x64) already installed — skipping."
     ${EndIf}

 SectionEnd
 ; ────────────────────────────────────────────────────────────────────────────

 ;   The stuff to install
 Section "BASIC256"

     SectionIn 1 RO
     
     SetOutPath $INSTDIR\Translations
     SetFileAttributes $INSTDIR\Translations HIDDEN
     File .\Translations\*.qm

     SetOutPath $INSTDIR\Modules
     File .\Modules\*

     SetOutPath $INSTDIR

     File build\Release\basic256.exe
     File ChangeLog
     File CONTRIBUTORS
     File license.txt
     File README.md
     File Basic256-IDE.png
     File Basic256-CLI.png
     File Basic256-Web.png

     ; Pull in the already-deployed Qt6 runtime (every Qt DLL and plugin
     ; subfolder windeployqt determined this exe actually needs) from the
     ; Basic256\ folder that package_windows.ps1 built via windeployqt in the
     ; previous CI step, instead of hand-listing DLL/plugin filenames here.
     ; The old itemized Qt5 list rotted badly across the Qt6 migration --
     ; several Qt5 plugin categories (mediaservice, playlistformats) don't
     ; exist in Qt6 at all, and the exact Qt6 replacements depend on which
     ; Multimedia backend Qt6 picked, which windeployqt already resolved
     ; correctly for us.
     File /r /x "Examples" /x "TestSuite" /x "Translations" /x "basic256.bat" /x "basic256.exe" /x "README.md" /x "Basic256-IDE.png" /x "Basic256-CLI.png" /x "Basic256-Web.png" "Basic256\*.*"

   ;   Write the installation path into the registry
     WriteRegStr HKLM SOFTWARE\BASIC256 "Install_Dir" "$INSTDIR"
     
     ;   Write the uninstall keys for Windows
     WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "DisplayName" "BASIC-256"
     WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "UninstallString" '"$INSTDIR\uninstall.exe"'
     WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "NoModify" 1
     WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "NoRepair" 1
     WriteUninstaller "uninstall.exe"

     ;   .kbs file association
     WriteRegStr HKCR ".kbs" "" "BASIC256.kbs"
     WriteRegStr HKCR "BASIC256.kbs" "" "BASIC-256 Script"
     WriteRegStr HKCR "BASIC256.kbs\DefaultIcon" "" "$INSTDIR\basic256.exe,0"
     WriteRegStr HKCR "BASIC256.kbs\shell\open\command" "" '"$INSTDIR\basic256.exe" "%1"'

     ;   Application registration  fixes generic icon and broken launch in "Open with" picker
     WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\basic256.exe" "" "$INSTDIR\basic256.exe"
     WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\basic256.exe" "Path" "$INSTDIR"
     WriteRegStr HKLM "SOFTWARE\Classes\Applications\basic256.exe" "FriendlyAppName" "BASIC-256 IDE"
     WriteRegStr HKLM "SOFTWARE\Classes\Applications\basic256.exe\DefaultIcon" "" "$INSTDIR\basic256.exe,0"
     WriteRegStr HKLM "SOFTWARE\Classes\Applications\basic256.exe\shell\open\command" "" '"$INSTDIR\basic256.exe" "%1"'
     WriteRegStr HKLM "SOFTWARE\Classes\Applications\basic256.exe\SupportedTypes" ".kbs" ""

     ;   Notify Explorer to refresh file associations immediately
     System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0x1000, i 0, i 0)'

 SectionEnd

 ;   Start Menu Shortcuts (can be disabled by the user)
 Section "Start Menu Shortcuts"
     SectionIn 1
     SetOutPath $INSTDIR   
     CreateDirectory "$SMPROGRAMS\BASIC256"
     CreateShortCut "$SMPROGRAMS\BASIC256\BASIC256.lnk" "$INSTDIR\BASIC256.exe" "" "$INSTDIR\BASIC256.exe" 0
     CreateShortCut "$SMPROGRAMS\BASIC256\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
 SectionEnd

 ;   Examples (can be disabled by the user)
 ;   /x "Basic256" keeps this off the staging folder package_windows.ps1
 ;   builds for the zip. "File /r <name>" does not copy just .\<name>: it
 ;   searches every subdirectory for a directory of that name and copies
 ;   each one under its own relative path, so the Examples staged at
 ;   Basic256\Examples used to land a second time in $INSTDIR\Basic256.
 Section "Example Programs"
     SectionIn 1
     SetOutPath $INSTDIR
     File /r /x ".svn" /x "Basic256" Examples
 SectionEnd

 ;   Test Suite (can be disabled by the user)
 ;   Excludes the staging folder for the same reason as Examples above.
 Section "Test Suite"
     SectionIn 1
     SetOutPath $INSTDIR
     File /r /x ".svn" /x "Basic256" /x "filegatesub" TestSuite
 SectionEnd

 ;---------------------------------

 ;   Uninstaller

 Section "Uninstall"
     
     ;   Remove registry keys
     DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256"
     DeleteRegKey HKLM SOFTWARE\BASIC256

     ;   Remove .kbs file association
     DeleteRegKey HKCR ".kbs"
     DeleteRegKey HKCR "BASIC256.kbs"
     DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\basic256.exe"
     DeleteRegKey HKLM "SOFTWARE\Classes\Applications\basic256.exe"

     ;   Remove files and uninstaller
     Delete $INSTDIR\*.exe
     Delete $INSTDIR\*.dll
     Delete $INSTDIR\ChangeLog
     Delete $INSTDIR\CONTRIBUTORS
     Delete $INSTDIR\license.txt
     Delete $INSTDIR\README.md
     Delete $INSTDIR\Basic256-IDE.png
     Delete $INSTDIR\Basic256-CLI.png
     Delete $INSTDIR\Basic256-Web.png
     RMDir /r $INSTDIR\espeak-data
     RMDir /r $INSTDIR\dist
     RMDir /r $INSTDIR\Examples
     RMDir /r $INSTDIR\TestSuite
     ; Cleanup for installs made before the stray CMake "share/basic256"
     ; install() rule (containing a duplicate Examples/TestSuite) was removed.
     RMDir /r $INSTDIR\share
     RMDir /r $INSTDIR\basic256
     RMDir /r $INSTDIR\help
     RMDir /r $INSTDIR\Translations
     RMDir /r $INSTDIR\Modules
     RMDir /r $INSTDIR\accessible
     RMDir /r $INSTDIR\audio
     RMDir /r $INSTDIR\imageformats
     RMDir /r $INSTDIR\platforms
     RMDir /r $INSTDIR\printsupport
     RMDir /r $INSTDIR\sqldrivers
     RMDir /r $INSTDIR\mediaservice
     RMDir /r $INSTDIR\playlistformats
     RMDir /r $INSTDIR\texttospeech
     ; Qt6 plugin categories (added post-migration; harmless no-ops if absent)
     RMDir /r $INSTDIR\multimedia
     RMDir /r $INSTDIR\generic
     RMDir /r $INSTDIR\styles
     RMDir /r $INSTDIR\iconengines
     RMDir /r $INSTDIR\networkinformation
     RMDir /r $INSTDIR\tls

     Delete $INSTDIR\uninstall.exe

     ;   Remove shortcuts, if any
     Delete "$SMPROGRAMS\BASIC256\*.*"

     ;   Remove directories used
     RMDir "$SMPROGRAMS\BASIC256"
     RMDir "$INSTDIR"

 SectionEnd
