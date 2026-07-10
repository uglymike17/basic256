$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path "Basic256" | Out-Null
Set-Content -Path "Basic256\basic256.bat" -Value "@echo off`r`nset QT_OPENGL=desktop`r`nstart basic256.exe"

Copy-Item build\Release\basic256.exe Basic256\
Copy-Item README.md                  Basic256\
Copy-Item Basic256-IDE.png,Basic256-CLI.png,Basic256-Web.png Basic256\

Copy-Item -Path "Examples" -Destination "Basic256\Examples" -Recurse -Force
Copy-Item -Path "TestSuite" -Destination "Basic256\TestSuite" -Recurse -Force

& "$env:QT_DIR\bin\windeployqt.exe" `
  --dir Basic256 `
  --multimedia `
  --no-translations `
  Basic256\basic256.exe

if (-not (Test-Path "Basic256\Translations")) {
    New-Item -ItemType Directory -Path "Basic256\Translations" | Out-Null
}

if (Test-Path "build\basic256_en.qm") {
    Copy-Item "build\*.qm" "Basic256\Translations\" -Force
} elseif (Test-Path "Translations") {
    Copy-Item "Translations\*.qm" "Basic256\Translations\" -Force
}

Remove-Item "Basic256\libGLESv2.dll", "Basic256\libEGL.dll", "Basic256\opengl32sw.dll" -ErrorAction SilentlyContinue
Copy-Item "$env:QT_DIR\bin\Qt6OpenGL.dll" Basic256\ -Force
Copy-Item "$env:QT_DIR\bin\Qt6MultimediaWidgets.dll" Basic256\ -Force
Copy-Item "$env:QT_DIR\bin\Qt6PrintSupport.dll" Basic256\ -Force

Compress-Archive -Path Basic256 -DestinationPath $env:ARTIFACT_NAME
