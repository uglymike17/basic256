#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# package_Linux_x86_AppImage.sh
# Produces a self-contained BASIC256-Linux-x86_64.AppImage from the CMake
# build output.  Must be run from the repository root after build_Linux_x86.sh.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
ARCH="x86_64"
QT_LIB="/usr/lib/x86_64-linux-gnu"
# Qt6 itself comes from aqtinstall (build_Linux_x86.sh exports QT_DIR via
# $GITHUB_ENV), not from a system apt package -- only non-Qt system libs
# (gstreamer, glib, etc.) still live under QT_LIB.
QT_PLUGIN_DIR="${QT_DIR:?QT_DIR not set - build_Linux_x86.sh must run first}/plugins"
GSTPLUG="${QT_LIB}/gstreamer-1.0"
APPDIR="$(pwd)/AppDir"
ARTIFACT_NAME="${ARTIFACT_NAME:-BASIC256-Linux-x86_64}"

# Qt's official builds bundle a sqldrivers plugin for every backend (Mimer,
# DB2, Firebird/ibase, MySQL, ODBC, PostgreSQL, SQLite) regardless of
# whether that backend's proprietary/optional client library is installed
# on this runner. BASIC256 only uses SQLite, and linuxdeploy's Qt plugin
# aborts on any bundled plugin with an unresolved dependency (this exact Qt
# tree already hit libqsqlmimer.so -> libmimerapi.so in
# package_Linux_x86.sh) -- so strip every driver but SQLite before
# deploying.
find "${QT_PLUGIN_DIR}/sqldrivers" -name '*.so' ! -name 'libqsqlite.so' -delete 2>/dev/null || true

# Avoid FUSE requirement on GitHub Actions runners; inherited by sub-processes
# (linuxdeploy-plugin-qt is also an AppImage and respects this flag)
export APPIMAGE_EXTRACT_AND_RUN=1

echo "==> Building AppDir at ${APPDIR}"

# ── AppDir skeleton ───────────────────────────────────────────────────────────
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib/gstreamer-1.0"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APPDIR}/usr/share/basic256"

# ── Binary & assets ───────────────────────────────────────────────────────────
cp build/basic256   "${APPDIR}/usr/bin/"
cp -r Examples      "${APPDIR}/usr/share/basic256/"
cp README.md        "${APPDIR}/usr/share/basic256/" 2>/dev/null || true
cp Basic256.png     "${APPDIR}/usr/share/icons/hicolor/256x256/apps/basic256.png"

# Root-level symlinks required by the AppImage specification
ln -sf "usr/share/icons/hicolor/256x256/apps/basic256.png" "${APPDIR}/basic256.png"

# ── Desktop file (AppImage spec: must exist, drives Name / Icon / Exec) ───────
cat > "${APPDIR}/usr/share/applications/basic256.desktop" << 'DESKTOP'
[Desktop Entry]
Version=1.0
Type=Application
Name=BASIC-256
Exec=basic256
Icon=basic256
Categories=Education;Science;ComputerScience;
Comment=Easy BASIC programming environment for children
Terminal=false
StartupWMClass=basic256
DESKTOP
ln -sf "usr/share/applications/basic256.desktop" "${APPDIR}/basic256.desktop"

# ── Custom AppRun ─────────────────────────────────────────────────────────────
# Written BEFORE linuxdeploy runs.  linuxdeploy preserves an existing AppRun,
# so this script is used as-is inside the final AppImage.
# It sets every env var that the generic linuxdeploy AppRun would miss:
# GStreamer plugin path, speech-dispatcher socket dir, and the XCB platform.
cat > "${APPDIR}/AppRun" << 'APPRUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"

export LD_LIBRARY_PATH="${HERE}/usr/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

# GStreamer: point to the bundled plugin set; disable system scanner
export GST_PLUGIN_PATH="${HERE}/usr/lib/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH_1_0=""
export GST_REGISTRY_REUSE_PLUGIN_SCANNER="no"
# Write the plugin registry cache to user's cache dir, not the read-only AppImage
export GST_REGISTRY="${XDG_CACHE_HOME:-${HOME}/.cache}/basic256/gst-registry.bin"

exec "${HERE}/usr/bin/basic256" "$@"
APPRUN
chmod +x "${APPDIR}/AppRun"

# ── Qt plugins (pre-copy before linuxdeploy so TTS + media are guaranteed) ───
# linuxdeploy-plugin-qt can miss TTS/multimedia unless they are already
# present and listed in EXTRA_QT_PLUGINS.
# NOTE: Qt6 Multimedia plugins live under a "multimedia" plugin dir (e.g.
# libffmpegmediaplugin.so) -- Qt5's "mediaservice"/"audio" categories no
# longer exist (confirmed via linuxdeployqt's "plugin could not be found"
# warnings against this same aqt-installed Qt6 tree in package_Linux_x86.sh).
for subdir in texttospeech multimedia imageformats; do
    mkdir -p "${APPDIR}/usr/plugins/${subdir}"
    cp "${QT_PLUGIN_DIR}/${subdir}/"*.so \
       "${APPDIR}/usr/plugins/${subdir}/" 2>/dev/null || true
done

# ── Extra runtime libs not auto-detected by linuxdeploy ──────────────────────
cplib() { cp "${QT_LIB}/${1}" "${APPDIR}/usr/lib/" 2>/dev/null || true; }

# Speech / audio
cplib "libespeak-ng.so.*"
cplib "libspeechd.so.*"
cplib "libasound.so.*"
cplib "libpulse.so.*"
cplib "libpulse-simple.so.*"
cplib "libpulse-mainloop-glib.so.*"
cplib "libpipewire-0.3.so.*"

# GStreamer core (linuxdeploy only traces direct ELF deps; these are dlopen'd)
cplib "libgstreamer-1.0.so.*"
cplib "libgstbase-1.0.so.*"
cplib "libgstaudio-1.0.so.*"
cplib "libgstpbutils-1.0.so.*"
cplib "libglib-2.0.so.*"
cplib "libgobject-2.0.so.*"
cplib "libgio-2.0.so.*"

# ── GStreamer plugins → usr/lib/gstreamer-1.0 ─────────────────────────────────
echo "==> Copying GStreamer plugins"
for p in libgstautodetect  libgstaudioconvert  libgstaudioresample \
          libgstplayback   libgstalsa          libgstpulseaudio    \
          libgstpipewire   libgstvolume        libgstcoreelements  \
          libgstwavparse   libgstaudioparsers  libgstmpegaudioparse \
          libgstmpg123     libgstlibav         libgstogg           \
          libgstvorbis     libgstisomp4; do
    cp "${GSTPLUG}/${p}.so" "${APPDIR}/usr/lib/gstreamer-1.0/" 2>/dev/null || true
done

# ── Translations ──────────────────────────────────────────────────────────────
mkdir -p "${APPDIR}/usr/share/basic256/Translations"
cp build/*.qm "${APPDIR}/usr/share/basic256/Translations/" 2>/dev/null || true

# ── Download linuxdeploy tools into /tmp ─────────────────────────────────────
# Downloading to /tmp (not the workspace root) keeps the workspace clean so
# the CI upload glob "*.AppImage" only picks up the final BASIC256 AppImage.
# linuxdeploy searches for the plugin in the same directory as itself, so
# both files must share the same directory.
TOOLS_DIR="/tmp/linuxdeploy_tools"
mkdir -p "${TOOLS_DIR}"
echo "==> Downloading linuxdeploy tools to ${TOOLS_DIR}"
wget -q -P "${TOOLS_DIR}" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
wget -q -P "${TOOLS_DIR}" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
chmod +x "${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
         "${TOOLS_DIR}/linuxdeploy-plugin-qt-${ARCH}.AppImage"

# ── Deploy Qt dependencies and produce .AppImage ──────────────────────────────
# QMAKE: tells the Qt plugin where to find Qt's own tools and plugin dirs.
# EXTRA_QT_PLUGINS: forces the Qt plugin to deploy TTS / media / audio even
#   when they are not directly linked (they are dlopen'd at runtime).
# --output appimage always writes to the current working directory (workspace
#   root), regardless of where the tool binary lives.
export QMAKE="${QT_DIR}/bin/qmake"
export QML_SOURCES_PATHS="."
export EXTRA_QT_PLUGINS="texttospeech;multimedia;imageformats"
export VERSION
VERSION="$(git describe --tags --always 2>/dev/null || echo 'dev')"

# The base linuxdeploy tool (before its "qt" plugin even runs) does its own
# ELF dependency walk of usr/bin/basic256 to decide what to bundle -- unlike
# linuxdeploy-plugin-qt, it has no notion of $QMAKE, and Qt6 isn't
# system-installed/ldconfig-registered here (it's an aqtinstall tree), so
# without this it can't resolve libQt6*.so.6 at all (hit
# "Could not find dependency: libQt6Sql.so.6" before the qt plugin ever ran).
export LD_LIBRARY_PATH="${QT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

echo "==> Running linuxdeploy (VERSION=${VERSION})"
"${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
    --appdir "${APPDIR}"                     \
    --plugin qt                              \
    --output appimage

# ── Rename output to expected artifact name ───────────────────────────────────
# linuxdeploy names the AppImage after the desktop Name + version + arch,
# e.g. "BASIC-256-2.1.0-x86_64.AppImage".  With the tools now in /tmp, the
# only *.AppImage in the workspace root is the one we just built.
for f in ./*.AppImage; do
    mv "${f}" "${ARTIFACT_NAME}.AppImage"
    echo "==> Created ${ARTIFACT_NAME}.AppImage"
    break
done
