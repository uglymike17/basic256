# ─────────────────────────────────────────────────────────────────────────────
# run_testsuite.ps1 <path-to-basic256.exe>
#
# Runs TestSuite/testsuite_ci.kbs headlessly via `basic256 -s` and fails the
# CI step if the interpreter crashes/errors OR if any test assertion failed.
#
# A failed assertion does NOT change basic256's exit code -- BASIC-256's
# `end` statement is a normal, deliberate stop, used both when
# testsuite_common_include.kbs's same()/different()/etc. helpers detect a
# failure (they print "fail" then call `end`) and on the success path at the
# end of testsuite_ci.kbs. So this script checks the captured output for the
# literal word "fail" and for the final "BASIC256_TESTSUITE_CI_PASSED"
# marker, in addition to the exit code (which still catches a genuine
# interpreter crash/script-load error).
# ─────────────────────────────────────────────────────────────────────────────
param(
    [Parameter(Mandatory = $true)]
    [string]$Basic256Exe
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testSuiteDir = Resolve-Path (Join-Path $scriptDir "..\..\TestSuite")
$basic256FullPath = (Resolve-Path $Basic256Exe).Path

Write-Host "==> Running TestSuite (unattended subset) via: $basic256FullPath -s testsuite_ci.kbs"

Push-Location $testSuiteDir
try {
    $output = & $basic256FullPath "-s" "testsuite_ci.kbs" 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
} finally {
    Pop-Location
}

Write-Host $output

$failed = $false

if ($exitCode -ne 0) {
    Write-Host "==> FAIL: basic256 exited with code $exitCode (interpreter error/crash)"
    $failed = $true
}

if ($output -match "(?m)\bfail\b") {
    Write-Host "==> FAIL: at least one test assertion printed 'fail'"
    $failed = $true
}

if ($output -notmatch "BASIC256_TESTSUITE_CI_PASSED") {
    Write-Host "==> FAIL: final 'BASIC256_TESTSUITE_CI_PASSED' marker not found (script stopped early)"
    $failed = $true
}

if ($failed) {
    exit 1
}

Write-Host "==> TestSuite passed."
