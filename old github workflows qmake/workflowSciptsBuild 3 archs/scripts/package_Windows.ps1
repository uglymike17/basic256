$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path "dist" | Out-Null
Set-Content -Path "dist\basic256.bat" -Value "@echo off`r`nset QT_OPENGL=desktop`r`nstart basic256.exe"

Copy-Item build\Release\basic256.exe dist\

Copy-Item -Path "Examples" -Destination "dist\Examples" -Recurse -Force

& "$env:Qt5_Dir\bin\windeployqt.exe" `
  --dir dist `
  --multimedia `
  --no-translations `
  dist\basic256.exe

if (Test-Path "dist\translations") {
    Move-Item "dist\translations" "dist\Translations_Temp"
    Move-Item "dist\Translations_Temp" "dist\Translations" -Force
}

if (-not (Test-Path "dist\Translations")) {
    New-Item -ItemType Directory -Path "dist\Translations" | Out-Null
}

if (Test-Path "build\basic256_en.qm") {
    Copy-Item "build\*.qm" "dist\Translations\" -Force
} elseif (Test-Path "Translations") {
    Copy-Item "Translations\*.qm" "dist\Translations\" -Force
}

Remove-Item "dist\libGLESv2.dll", "dist\libEGL.dll", "dist\opengl32sw.dll" -ErrorAction SilentlyContinue
Copy-Item "$env:Qt5_Dir\bin\Qt5OpenGL.dll" dist\ -Force
Copy-Item "$env:Qt5_Dir\bin\Qt5MultimediaWidgets.dll" dist\ -Force
Copy-Item "$env:Qt5_Dir\bin\Qt5PrintSupport.dll" dist\ -Force

Compress-Archive -Path dist -DestinationPath $env:ARTIFACT_NAME
