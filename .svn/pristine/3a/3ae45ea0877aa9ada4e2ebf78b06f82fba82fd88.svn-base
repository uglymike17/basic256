REM # BATCH FILE TO COPY THE DLL AND SUPPORT FILES INTO
REM # THE BASIC256Portable folder SO THAT THEY MAY BE
REM # INCLUDED IN THE PORTABLE DISTRIBUTION

REM # DATE...... PROGRAMMER... VERSION....	DESCRIPTION...
REM # 2013-11-11 j.m.reneau    1.0.0		original coding
REM # 2014-01-05 j.m.reneau    1.0.7		added qt sql
REM # 2014-04-19 j.m.reneau    1.1.1.3		moved development to qt 5.2.1
REM # 2014-06-01 j.m.reneau    1.1.2.4      changed paths to qt 5.3
REM # 2014-10-26 j.m.reneau    1.1.4.0      added serialport
REM # 2016-01-01 j.m.reneau    1.99.99.08	moved to 5.5
REM # 2016-10-31 j.m.reneau    1.99.99.72	moved to qt 5.7
REM # 2020-04-28 j.m.reneau    2.0.0.1		moved to QT 5.14.2

REM # Iterate through development and propduction folders
FOR %%F IN (BASIC256Portable BASIC256PortableDebug) DO (call :foldercreate %%F)
goto :eof

REM # ACTUALLY Create and Fill
:foldercreate
set SDK_BIN=C:\Qt\5.14.2\mingw73_32\bin
set SDK_LIB=C:\Qt\5.14.2\mingw73_32\lib
set SDK_PLUGINS=C:\Qt\5.14.2\mingw73_32\plugins

set INSTDIR=%1\App\BASIC256
echo %INSTDIR%

rmdir /s /q %INSTDIR%
mkdir %INSTDIR%

mkdir %INSTDIR%\Translations
xcopy Translations\*.qm %INSTDIR%\Translations

mkdir %INSTDIR%\Modules
xcopy Modules\* %INSTDIR%\Modules

mkdir %INSTDIR%\espeak-data
xcopy /e release\espeak-data %INSTDIR%\espeak-data

mkdir %INSTDIR%\audio
xcopy %SDK_PLUGINS%\audio\qtaudio_windows.dll %INSTDIR%\audio

mkdir %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qgif.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qico.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qjpeg.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qsvg.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qtga.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qtiff.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qwbmp.dll %INSTDIR%\imageformats

mkdir %INSTDIR%\platforms
xcopy %SDK_PLUGINS%\platforms\qwindows.dll %INSTDIR%\platforms

mkdir %INSTDIR%\printsupport
xcopy %SDK_PLUGINS%\printsupport\windowsprintersupport.dll %INSTDIR%\printsupport

mkdir %INSTDIR%\sqldrivers
xcopy %SDK_PLUGINS%\sqldrivers\qsqlite.dll %INSTDIR%\sqldrivers

mkdir %INSTDIR%\mediaservice
xcopy %SDK_PLUGINS%\mediaservice\dsengine.dll %INSTDIR%\mediaservice
xcopy %SDK_PLUGINS%\mediaservice\qtmedia_audioengine.dll %INSTDIR%\mediaservice

mkdir %INSTDIR%\playlistformats
xcopy %SDK_PLUGINS%\playlistformats\qtmultimedia_m3u.dll %INSTDIR%\playlistformats

xcopy ChangeLog %INSTDIR%
xcopy CONTRIBUTORS %INSTDIR%
xcopy license.txt %INSTDIR%

xcopy %SDK_BIN%\libgcc_s_dw2-1.dll %INSTDIR%
xcopy %SDK_BIN%\libstdc++-6.dll %INSTDIR%
xcopy %SDK_BIN%\libwinpthread-1.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Core.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Gui.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Multimedia.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5MultimediaWidgets.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Network.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5OpenGL.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5PrintSupport.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5SerialPort.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Sql.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5WebKit.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Widgets.dll %INSTDIR%

xcopy %SDK_LIB%\libespeak.dll %INSTDIR%
xcopy %SDK_LIB%\libportaudio-2.dll %INSTDIR%
xcopy %SDK_LIB%\libpthread-2.dll %INSTDIR%

goto :eof
