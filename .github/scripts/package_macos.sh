#!/usr/bin/env bash
set -euo pipefail

APP_NAME="basic256"
APP_BUNDLE="build/${APP_NAME}.app"
DIST_DIR="${APP_NAME}-Distribution"
ARTIFACT_NAME="${ARTIFACT_NAME:-Basic256-MacOS.zip}"

# 1. Fix the Qt dependencies inside the .app bundle so it runs on other Macs
macdeployqt "${APP_BUNDLE}" -always-overwrite

# # 2. Bundle the translation (.qm) files into the standard macOS location
# mkdir -p "${APP_BUNDLE}/Contents/Resources/Translations"
# cp Translations/*.qm "${APP_BUNDLE}/Contents/Resources/Translations/"

# Compiled translations
mkdir -p dist/Translations
if ls build/*.qm 1>/dev/null 2>&1; then
  cp build/*.qm dist/Translations/
elif ls Translations/*.qm 1>/dev/null 2>&1; then
  cp Translations/*.qm dist/Translations/
fi


# 3. Create a clean distribution folder to hold the app and extra documents
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}"

# Move the app bundle and copy your documentation/tests next to it
mv "${APP_BUNDLE}" "${DIST_DIR}/"
cp -r Examples "${DIST_DIR}/"
cp -r TestSuite "${DIST_DIR}/"
cp README.md "${DIST_DIR}/"

echo "==> Creating zip artifact..."
rm -f "${ARTIFACT_NAME}"
ditto -c -k --sequesterRsrc --keepParent "${DIST_DIR}" "${ARTIFACT_NAME}"
