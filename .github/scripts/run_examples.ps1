# ─────────────────────────────────────────────────────────────────────────────
# run_examples.ps1 <path-to-basic256.exe>
#
# Runs every example listed in TestSuite/testsuite_examples_ci.txt headlessly
# via `basic256 -s`, one process each, and fails the CI step if any of them
# crashes, stops on an error, prints ERROR or is still running after
# $TimeLimit seconds.
#
# The examples run from a temporary copy of Examples/Original_Examples, not
# from the checkout: several of them write files into their own folder
# (databases, binary.dat, foo.txt), and the packaging step that follows copies
# Examples/ straight into the release, so running them in place would ship
# whatever they leave behind.
# ─────────────────────────────────────────────────────────────────────────────
param(
    [Parameter(Mandatory = $true)]
    [string]$Basic256Exe,
    [int]$TimeLimit = 60
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = (Resolve-Path (Join-Path $scriptDir "..\..")).Path
$listFile = Join-Path $repoDir "TestSuite\testsuite_examples_ci.txt"
$basic256FullPath = (Resolve-Path $Basic256Exe).Path

# Same environment as run_testsuite.ps1: Qt is not staged next to the exe
# until packaging, so put the aqtinstall Qt6 on PATH and point at its plugins.
if ($env:QT_DIR) {
    $env:PATH = "$env:QT_DIR\bin;$env:PATH"
    $env:QT_PLUGIN_PATH = "$env:QT_DIR\plugins"
}

$workDir = Join-Path ([IO.Path]::GetTempPath()) ("basic256-examples-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $workDir | Out-Null
Copy-Item -Path (Join-Path $repoDir "Examples\Original_Examples") -Destination (Join-Path $workDir "examples") -Recurse
$outFile = Join-Path $workDir "out.txt"
$errFile = Join-Path $workDir "err.txt"
$inFile = Join-Path $workDir "in.txt"
Set-Content -Path $inFile -Value "" -NoNewline

$passed = 0
$failedNames = @()

Write-Host "==> Running the original examples listed in TestSuite/testsuite_examples_ci.txt"
try {
    foreach ($line in Get-Content $listFile) {
        $example = $line.Trim()
        if ($example -eq "" -or $example.StartsWith("#")) { continue }

        $examplePath = Join-Path (Join-Path $workDir "examples") ($example -replace "/", "\")
        if (-not (Test-Path $examplePath)) {
            Write-Host "FAIL  $example  (listed but not found)"
            $failedNames += $example
            continue
        }

        $timer = [Diagnostics.Stopwatch]::StartNew()
        $p = Start-Process -FilePath $basic256FullPath `
            -ArgumentList "-s", ('"' + (Split-Path -Leaf $examplePath) + '"') `
            -WorkingDirectory (Split-Path -Parent $examplePath) `
            -RedirectStandardOutput $outFile -RedirectStandardError $errFile `
            -RedirectStandardInput $inFile -NoNewWindow -PassThru
        # Touching Handle straight away is what lets ExitCode be read later;
        # without it Start-Process hands back a process whose ExitCode is null.
        $null = $p.Handle
        $finished = $p.WaitForExit($TimeLimit * 1000)
        if (-not $finished) {
            $p.Kill()
            $p.WaitForExit()
        }
        $seconds = [int]$timer.Elapsed.TotalSeconds
        $stderr = Get-Content -Raw $errFile
        if ($null -eq $stderr) { $stderr = "" }

        $reason = ""
        if (-not $finished) {
            $reason = "still running after ${TimeLimit}s"
        } elseif ($p.ExitCode -ne 0) {
            $reason = "exit code $($p.ExitCode)"
        } elseif ($stderr -match "ERROR") {
            $reason = "printed an ERROR"
        }

        if ($reason -eq "") {
            Write-Host "pass  $example  (${seconds}s)"
            $passed++
        } else {
            Write-Host "FAIL  $example  ($reason)"
            Write-Host "----- stdout (last 20 lines)"
            Get-Content $outFile -Tail 20 | ForEach-Object { Write-Host $_ }
            Write-Host "----- stderr"
            Write-Host $stderr
            Write-Host "-----"
            $failedNames += $example
        }
    }
} finally {
    Remove-Item -Recurse -Force $workDir -ErrorAction SilentlyContinue
}

Write-Host "==> $passed examples passed, $($failedNames.Count) failed"
if ($failedNames.Count -ne 0) {
    Write-Host "==> FAIL: $($failedNames -join ' ')"
    exit 1
}
if ($passed -eq 0) {
    Write-Host "==> FAIL: no examples were run - is $listFile empty?"
    exit 1
}
Write-Host "==> Original examples passed."
