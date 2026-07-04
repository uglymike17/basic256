$ErrorActionPreference = "Stop"

#Install flex and bison
choco install winflexbison3 -y

#install Qt6
# qtmultimedia/qtserialport/qtspeech are separate addon modules in Qt6.
# qt5compat provides Qt6::Core5Compat, the temporary QRegExp shim (see
# QT6_MIGRATION_CHECKLIST.md Phase 2) so the rest of the codebase still
# compiles until the QRegularExpression migration is done.
python -m pip install aqtinstall
aqt install-qt windows desktop 6.7.3 win64_msvc2019_64 -m qtmultimedia qtserialport qtspeech qt5compat

#set Qt6 dir
"QT_DIR=$env:GITHUB_WORKSPACE\6.7.3\msvc2019_64" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append

#Build Windows
$bisonBin = "C:\ProgramData\chocolatey\lib\winflexbison3\tools"
$env:BISON_PKGDATADIR = "C:\ProgramData\chocolatey\lib\winflexbison3\tools\data"

# Mirror flex/bison executables so names match standard conventions
Copy-Item "$bisonBin\win_flex.exe" "$bisonBin\flex.exe" -Force
Copy-Item "$bisonBin\win_bison.exe" "$bisonBin\bison.exe" -Force

# Add bison tools to path for the lexer build step
$env:Path = "$bisonBin;" + $env:Path

# Generate LEX parser files manually using flex/bison direct execution
Push-Location LEX
& "$bisonBin\bison.exe" -d -v -l basicParse.y
& "$bisonBin\flex.exe" -L basicParse.l
Pop-Location

# Configure using MSVC (Omitting -G defaults to the installed Visual Studio version)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$env:QT_DIR"
# Build using MSVC compiler pipeline natively via CMake
cmake --build build --config Release