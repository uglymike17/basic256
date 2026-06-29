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
QT_PLUGIN_DIR="${QT_LIB}/qt5/plugins"
GSTPLUG="${QT_LIB}/gstreamer-1.0"
APPDIR="$(pwd)/AppDir"
ARTIFACT_NAME="${ARTIFACT_NAME:-BASIC256-Linux-x86_64}"

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
# linuxdeploy-plugin-qt can miss TTS and mediaservice unless they are already
# present and listed in EXTRA_QT_PLUGINS.
for subdir in texttospeech mediaservice audio imageformats; do
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

# Qt Multimedia GStreamer bridge
cplib "libQt5MultimediaGstTools.so*"

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

# ── Download linuxdeploy + Qt plugin ─────────────────────────────────────────
echo "==> Downloading linuxdeploy tools"
wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
chmod +x "linuxdeploy-${ARCH}.AppImage" "linuxdeploy-plugin-qt-${ARCH}.AppImage"

# ── Deploy Qt dependencies and produce .AppImage ──────────────────────────────
# QMAKE: tells the Qt plugin where to find Qt's own tools and plugin dirs.
# EXTRA_QT_PLUGINS: forces the Qt plugin to deploy TTS / media / audio even
#   when they are not directly linked (they are dlopen'd at runtime).
export QMAKE="${QT_LIB}/qt5/bin/qmake"
export QML_SOURCES_PATHS="."
export EXTRA_QT_PLUGINS="texttospeech;mediaservice;audio;imageformats"
export VERSION
VERSION="$(git describe --tags --always 2>/dev/null || echo 'dev')"

echo "==> Running linuxdeploy (VERSION=${VERSION})"
"./linuxdeploy-${ARCH}.AppImage"  \
    --appdir  "${APPDIR}"          \
    --plugin  qt                   \
    --output  appimage

# ── Rename output to expected artifact name ───────────────────────────────────
# linuxdeploy names the AppImage after the desktop Name + version + arch,
# e.g. "BASIC-256-2.1.0-x86_64.AppImage".  Rename it to what the CI upload
# step expects (value of ARTIFACT_NAME env var).
for f in ./*.AppImage; do
    case "${f##*/}" in linuxdeploy*) continue ;; esac   # skip the tools
    mv "${f}" "${ARTIFACT_NAME}.AppImage"
    echo "==> Created ${ARTIFACT_NAME}.AppImage"
    break
done
