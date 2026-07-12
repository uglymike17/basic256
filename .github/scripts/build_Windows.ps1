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

$qtVersion = "6.11.1"
$qtArch = "win64_msvc2022_64"
$qtBaseDir = "$env:GITHUB_WORKSPACE\$qtVersion"
$qtDir = "$qtBaseDir\msvc2022_64"

# A Qt mirror intermittently serves a truncated/corrupt archive -- seen as
# "Bad7zFile: Specified path is bad: bin/avutil-59.dll" while unpacking
# qtmultimedia. aqt then dies with a traceback and a non-zero exit code, but
# PowerShell does *not* stop on a native command's exit code ($ErrorActionPreference
# only governs cmdlet errors), so the script used to sail straight on to cmake,
# which failed much later with the misleading "Failed to find required Qt
# component Multimedia". Retry the install from a clean tree, and verify each
# addon module's CMake package config actually landed before continuing.
$requiredConfigs = @(
    "$qtDir\lib\cmake\Qt6\Qt6Config.cmake",
    "$qtDir\lib\cmake\Qt6Multimedia\Qt6MultimediaConfig.cmake",
    "$qtDir\lib\cmake\Qt6SerialPort\Qt6SerialPortConfig.cmake",
    "$qtDir\lib\cmake\Qt6TextToSpeech\Qt6TextToSpeechConfig.cmake"
)

$maxAttempts = 3
$qtInstalled = $false
for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
    Write-Host "aqt install-qt: attempt $attempt of $maxAttempts"

    # Start from a clean tree so a half-extracted module left behind by a failed
    # attempt cannot masquerade as a good install.
    if (Test-Path $qtBaseDir) {
        Remove-Item $qtBaseDir -Recurse -Force
    }

    aqt install-qt windows desktop $qtVersion $qtArch -m qtmultimedia qtserialport qtspeech
    $aqtExit = $LASTEXITCODE

    $missing = @($requiredConfigs | Where-Object { -not (Test-Path $_) })

    if ($aqtExit -eq 0 -and $missing.Count -eq 0) {
        $qtInstalled = $true
        break
    }

    if ($aqtExit -ne 0) {
        Write-Host "::warning::aqt install-qt exited with code $aqtExit (attempt $attempt)"
    }
    foreach ($config in $missing) {
        Write-Host "::warning::Qt module config missing after install: $config"
    }

    if ($attempt -lt $maxAttempts) {
        Start-Sleep -Seconds 15
    }
}

if (-not $qtInstalled) {
    throw "aqt install-qt did not produce a complete Qt $qtVersion installation after $maxAttempts attempts (see the warnings above for the exit code and/or the missing module configs). This is usually a corrupt archive served by a Qt mirror; re-running the job normally clears it."
}

#set Qt6 dir (also export for later steps, e.g. packaging - $GITHUB_ENV only
#takes effect starting from the *next* step, not later in this same script)
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
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "bison failed on basicParse.y (exit $LASTEXITCODE)" }
& "$bisonBin\flex.exe" -L basicParse.l
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "flex failed on basicParse.l (exit $LASTEXITCODE)" }
Pop-Location

# Configure using MSVC (Omitting -G defaults to the installed Visual Studio version)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$env:QT_DIR"
# Stop here on a configure failure: without the exit-code check the script runs
# on to `cmake --build`, whose "MSB1009: Project file does not exist" buries the
# configure error that actually caused it.
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

# Build using MSVC compiler pipeline natively via CMake
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { throw "cmake build failed (exit $LASTEXITCODE)" }