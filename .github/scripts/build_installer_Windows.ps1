$ErrorActionPreference = "Stop"

# Install NSIS
choco install nsis -y --no-progress
if ($LASTEXITCODE -ne 0) { throw "choco install nsis failed (exit $LASTEXITCODE)" }
# Download VC++ 2015-2022 Redistributable for bundling into the installer
Write-Host "Downloading vc_redist.x64.exe..."
Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" `
                  -OutFile "vc_redist.x64.exe"

# Derive the installer version from the branch being built, so the output
# filename (and the in-installer version label) tracks the branch name
# automatically instead of needing a manual edit in BASIC256.nsi every time
# an alpha/beta branch is cut.
# - GITHUB_HEAD_REF is set for pull_request events (source branch name).
# - GITHUB_REF_NAME is set for push/release events (branch or tag name).
# - Fall back to git directly for local/non-CI runs.
$branchName = $env:GITHUB_HEAD_REF
if (-not $branchName) { $branchName = $env:GITHUB_REF_NAME }
if (-not $branchName) { $branchName = (git rev-parse --abbrev-ref HEAD).Trim() }

# Branches are named like "v2.1.Alpha03" - strip the leading "v" to get the
# "2.1.Alpha03" version string used in the installer name. Anything that is
# NOT a version branch (main, a detached-HEAD SHA, git not found, ...) falls
# back to BASIC256_MAIN_DISPLAY_VERSION declared in CMakeLists.txt -- the
# same string CMake's own VERSION derivation uses for such builds -- so the
# installer label can't drift from the About box. This mirrors the
# ^[vV][0-9] guard in CMakeLists.txt; keeping the two in lockstep is exactly
# why the fallback reads CMakeLists.txt instead of hardcoding a name here.
if ($branchName -match '^[vV][0-9]') {
    $version = $branchName -replace '^[vV]', ''
} else {
    $cmakeLists = Join-Path $PSScriptRoot '..\..\CMakeLists.txt'
    $cmakeText = Get-Content -Raw $cmakeLists
    if ($cmakeText -match 'BASIC256_MAIN_DISPLAY_VERSION\s+"([^"]+)"') {
        $version = $Matches[1]
    } else {
        throw "Could not parse BASIC256_MAIN_DISPLAY_VERSION from $cmakeLists (branch '$branchName' is not a version branch, so the fallback is required)"
    }
}

Write-Host "Building installer for version: $version (branch: $branchName)"

# Compile the installer
# QT_DIR is already in the environment, exported by build_Windows.ps1
# makensis reads $%QT_DIR% from the environment at compile time
# /DVERSION overrides the fallback !define VERSION in BASIC256.nsi
& "C:\Program Files (x86)\NSIS\makensis.exe" "/DVERSION=$version" BASIC256.nsi
if ($LASTEXITCODE -ne 0) { throw "makensis failed (exit $LASTEXITCODE)" }

Write-Host "Installer built: BASIC256-${version}_Win64_Install.exe"