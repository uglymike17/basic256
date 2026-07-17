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
mkdir -p Basic256/Translations
if ls build/*.qm 1>/dev/null 2>&1; then
  cp build/*.qm Basic256/Translations/
elif ls Translations/*.qm 1>/dev/null 2>&1; then
  cp Translations/*.qm Basic256/Translations/
fi


# 3. Create a clean distribution folder to hold the app and extra documents
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}"

# Bundled module library, beside the executable inside the .app so that
# include "math.kbs" resolves (the interpreter searches <exe dir>/Modules, and
# applicationDirPath() inside a bundle is Contents/MacOS). Added before the
# move so it travels with the bundle and is covered by the codesign below.
mkdir -p "${APP_BUNDLE}/Contents/MacOS/Modules"
cp -r Modules/* "${APP_BUNDLE}/Contents/MacOS/Modules/"

# Bundled example programs, inside the .app beside the executable so the in-app
# "File > Open Example..." resolves them via applicationDirPath()/Examples even
# after the .app is dragged out of this distribution folder. Also copied next to
# the .app below for plain Finder browsing. Added before the move/codesign so it
# travels with the bundle and is covered by the signature.
mkdir -p "${APP_BUNDLE}/Contents/MacOS/Examples"
cp -r Examples/* "${APP_BUNDLE}/Contents/MacOS/Examples/"

# Move the app bundle and copy your documentation/tests next to it
mv "${APP_BUNDLE}" "${DIST_DIR}/"
cp -r Examples "${DIST_DIR}/"
cp -r TestSuite "${DIST_DIR}/"
cp README.md "${DIST_DIR}/"
cp Basic256-IDE.png Basic256-CLI.png Basic256-Web.png "${DIST_DIR}/"

#    Code-sign the bundle so macOS Gatekeeper doesn't report it as "damaged".
#    Set CODESIGN_IDENTITY to a "Developer ID Application: Name (TEAMID)"
#    identity in CI once a real Apple Developer certificate is configured;
#    until then this falls back to an ad-hoc signature, which silences the
#    generic "damaged" message but is NOT sufficient for notarization.
echo "==> Code-signing app bundle..."
if [ -n "${CODESIGN_IDENTITY:-}" ]; then
    codesign --force --deep --options runtime \
        --sign "${CODESIGN_IDENTITY}" "${DIST_DIR}/${APP_NAME}.app"
else
    codesign --force --deep --sign - "${DIST_DIR}/${APP_NAME}.app"
fi

echo "==> Creating zip artifact..."
rm -f "${ARTIFACT_NAME}"
ditto -c -k --sequesterRsrc --keepParent "${DIST_DIR}" "${ARTIFACT_NAME}"
