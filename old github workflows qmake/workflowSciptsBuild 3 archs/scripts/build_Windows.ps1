$ErrorActionPreference = "Stop"

#Install flex and bison
choco install winflexbison3 -y

#install Qt
python -m pip install aqtinstall
aqt install-qt windows desktop 5.15.2 win64_msvc2019_64

#set Qt5 dir
"Qt5_Dir=$env:GITHUB_WORKSPACE\5.15.2\msvc2019_64" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append

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
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$env:GITHUB_WORKSPACE\5.15.2\msvc2019_64"
# Build using MSVC compiler pipeline natively via CMake
cmake --build build --config Release