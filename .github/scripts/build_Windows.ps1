$ErrorActionPreference = "Stop"

#Install flex and bison
choco install winflexbison3 -y

#install Qt6
# qtmultimedia/qtserialport/qtspeech are separate addon modules in Qt6.
# aqtinstall 3.3.0 (latest on PyPI) cannot parse the Qt 6.11 Windows
# repository layout (per-arch subdirs replaced the old nested
# qt6_6111/qt6_6111/Updates.xml path -- aqtinstall issue #1007). The fix
# (PR #1000) merged upstream 2026-03-24 but has not shipped in a PyPI
# release yet, so install aqtinstall pinned to that merge commit instead.
# Revert to a plain `pip install aqtinstall` once a release contains it.
python -m pip install "git+https://github.com/miurahr/aqtinstall.git@8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b"
aqt install-qt windows desktop 6.11.1 win64_msvc2022_64 -m qtmultimedia qtserialport qtspeech

#set Qt6 dir (also export for later steps, e.g. packaging - $GITHUB_ENV only
#takes effect starting from the *next* step, not later in this same script)
$qtDir = "$env:GITHUB_WORKSPACE\6.11.1\msvc2022_64"
$env:QT_DIR = $qtDir
"QT_DIR=$qtDir" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append

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
#cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$env:QT_DIR"
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="$env:QT_DIR"
# Build using MSVC compiler pipeline natively via CMake
#cmake --build build --config Release
cmake --build build --config RelWithDebInfo