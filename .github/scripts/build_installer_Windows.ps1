$ErrorActionPreference = "Stop"

# Install NSIS
choco install nsis -y --no-progress
if ($LASTEXITCODE -ne 0) { throw "choco install nsis failed (exit $LASTEXITCODE)" }

# Compile the installer
# Qt5_Dir is already in the environment, exported by build_Windows.ps1
# makensis reads $%Qt5_Dir% from the environment at compile time
& "C:\Program Files (x86)\NSIS\makensis.exe" BASIC256.nsi
if ($LASTEXITCODE -ne 0) { throw "makensis failed (exit $LASTEXITCODE)" }

Write-Host "Installer built: BASIC256-$env:VERSION_STR`_Win64_Install.exe"