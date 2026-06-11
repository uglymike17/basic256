#!/usr/bin/env bash
set -euo pipefail

cd build

# 1. Fix the Qt dependencies inside the .app bundle so it runs on other Macs
# -qmldir points to where your source QML files are (if you use QML)
macdeployqt BASIC256.app -qmldir=../

# 2. Bundle the translation (.qm) files into the standard macOS location
mkdir -p BASIC256.app/Contents/Resources/Translations
cp ../Translations/*.qm BASIC256.app/Contents/Resources/Translations/

# 3. Create a clean distribution folder to hold the app and extra documents
DIST_DIR="BASIC256-Distribution"
mkdir -p "$DIST_DIR"

# Move the app bundle and copy your documentation/tests next to it
mv BASIC256.app "$DIST_DIR/"
cp -r ../Examples "$DIST_DIR/"
cp -r ../TestSuite "$DIST_DIR/"
cp ../README.md "$DIST_DIR/"

# 4. Zip the entire distribution folder
zip -r "../${ARTIFACT_NAME}.zip" "$DIST_DIR"