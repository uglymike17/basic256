#!/usr/bin/env bash
set -euo pipefail

cd build

# 1. Fix the Qt dependencies inside the .app bundle so it runs on other Macs
# -qmldir points to where your source QML files are (if you use QML)
macdeployqt Basic256.app -always-overwrite

# 2. Bundle the translation (.qm) files into the standard macOS location
mkdir -p "build/basic256.app/Contents/Resources/Translations"
cp ../Translations/*.qm "build/basic256.app/Contents/Resources/Translations/"

# 3. Create a clean distribution folder to hold the app and extra documents
DIST_DIR="Basic256-Distribution"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Move the app bundle and copy your documentation/tests next to it
mv "build/Basic256.app" "$DIST_DIR/"
cp -r ../Examples "$DIST_DIR/"
cp -r ../TestSuite "$DIST_DIR/"
cp ../README.md "$DIST_DIR/"

echo "==> Creating zip artifact..."
rm -f "${ARTIFACT_NAME}"
ditto -c -k --sequesterRsrc --keepParent "${APP_BUNDLE}" "${ARTIFACT_NAME}"