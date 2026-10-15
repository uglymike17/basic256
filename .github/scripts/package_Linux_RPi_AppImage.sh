#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# package_Linux_RPi_AppImage.sh
# Produces a self-contained BASIC256-RaspberryPi-ARM64.AppImage from the
# CMake build output.  Must be run from the repository root after
# build_Linux_RPi_Trixie.sh.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
ARCH="aarch64"
QT_LIB="/usr/lib/aarch64-linux-gnu"
# Qt6 itself comes from aqtinstall (build_Linux_RPi_Trixie.sh exports QT_DIR
# via $GITHUB_ENV), not from a system apt package -- only non-Qt system libs
# (gstreamer, glib, etc.) still live under QT_LIB.
QT_PLUGIN_DIR="${QT_DIR:?QT_DIR not set - build_Linux_RPi_Trixie.sh must run first}/plugins"
GSTPLUG="${QT_LIB}/gstreamer-1.0"
APPDIR="$(pwd)/AppDir"
ARTIFACT_NAME="${ARTIFACT_NAME:-BASIC256-RaspberryPi-ARM64}"

# Qt's official builds carry a sqldrivers plugin for every backend, and
# linuxdeploy's Qt plugin aborts on any bundled plugin with an unresolved
# dependency (see package_Linux_x86_AppImage.sh). BASIC256 only uses SQLite.
find "${QT_PLUGIN_DIR}/sqldrivers" -name '*.so' ! -name 'libqsqlite.so' -delete 2>/dev/null || true

export APPIMAGE_EXTRACT_AND_RUN=1

echo "==> Building AppDir at ${APPDIR}"

# ── AppDir skeleton ───────────────────────────────────────────────────────────
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib/gstreamer-1.0"
mkdir -p "${APPDIR}/usr/lib/speech-dispatcher"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/64x64/apps"
mkdir -p "${APPDIR}/usr/share/basic256"

# ── Binary & assets ───────────────────────────────────────────────────────────
cp build/basic256   "${APPDIR}/usr/bin/"
cp -r Examples      "${APPDIR}/usr/share/basic256/"
# Bundled module library, beside the binary so include "math.kbs" resolves
# (the interpreter searches <exe dir>/Modules, and the exe lives in usr/bin).
cp -r Modules       "${APPDIR}/usr/bin/"
cp README.md        "${APPDIR}/usr/share/basic256/" 2>/dev/null || true
cp Basic256-IDE.png Basic256-CLI.png Basic256-Web.png Basic256-Web_GraphicsOnly.png \
                    "${APPDIR}/usr/share/basic256/" 2>/dev/null || true
# Desktop/launcher icon: the real 64x64 BASIC256 app logo (same image the
# running app uses as its window icon), not a screenshot.
cp resources/icons/basic256.png "${APPDIR}/usr/share/icons/hicolor/64x64/apps/basic256.png"

ln -sf "usr/share/icons/hicolor/64x64/apps/basic256.png" "${APPDIR}/basic256.png"

# ── Desktop file ──────────────────────────────────────────────────────────────
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
cat > "${APPDIR}/AppRun" << 'APPRUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"

export LD_LIBRARY_PATH="${HERE}/usr/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

export GST_PLUGIN_PATH="${HERE}/usr/lib/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH_1_0=""
export GST_REGISTRY_REUSE_PLUGIN_SCANNER="no"
export GST_REGISTRY="${XDG_CACHE_HOME:-${HOME}/.cache}/basic256/gst-registry.bin"

# speech-dispatcher modules live inside the AppImage on ARM
export SPEECHD_MODULE_DIR="${HERE}/usr/lib/speech-dispatcher"

# Qt reads the desktop's colours, light or dark, through a platform theme
# plugin. It picks the bundled GTK one by itself on GNOME-like desktops but
# not on others, such as labwc on the Raspberry Pi, so ask for it -- unless
# the user chose a theme, or the desktop is Qt-based (KDE, LXQt), where Qt's
# own choice is the right one.
if [ -z "${QT_QPA_PLATFORMTHEME:-}" ]; then
    case "${XDG_CURRENT_DESKTOP:-}" in
        *KDE*|*LXQt*|*LXQT*) ;;
        *) export QT_QPA_PLATFORMTHEME=gtk3 ;;
    esac
fi

exec "${HERE}/usr/bin/basic256" "$@"
APPRUN
chmod +x "${APPDIR}/AppRun"

# ── qt.conf: tells Qt to look for plugins relative to the binary ──────────────
cat > "${APPDIR}/usr/bin/qt.conf" << 'QTCONF'
[Paths]
Plugins = ../plugins
QTCONF

# ── Qt plugins ────────────────────────────────────────────────────────────────
# Qt 6 keeps its multimedia backends under "multimedia"; Qt 5's
# "mediaservice" and "audio" no longer exist.
for subdir in texttospeech multimedia imageformats platforms; do
    mkdir -p "${APPDIR}/usr/plugins/${subdir}"
    cp "${QT_PLUGIN_DIR}/${subdir}/"*.so \
       "${APPDIR}/usr/plugins/${subdir}/" 2>/dev/null || true
done
# SQLite driver for DBOPEN - only SQLite, the other drivers pull in client
# libraries that are not bundled
mkdir -p "${APPDIR}/usr/plugins/sqldrivers"
cp "${QT_PLUGIN_DIR}/sqldrivers/libqsqlite.so" "${APPDIR}/usr/plugins/sqldrivers/"

# ── Extra runtime libs ────────────────────────────────────────────────────────
# On ARM, linuxdeploy-plugin-qt is less reliable for finding all Qt modules,
# so we bundle them explicitly (mirrors build_Linux_RPi_Trixie.sh).
cplib() { cp "${QT_LIB}/${1}" "${APPDIR}/usr/lib/" 2>/dev/null || true; }
cpqt() { cp "${QT_DIR}/lib/${1}" "${APPDIR}/usr/lib/" 2>/dev/null || true; }

# Qt modules, from the aqtinstall tree
cpqt "libQt6Core.so.*"
cpqt "libQt6Gui.so.*"
cpqt "libQt6Widgets.so.*"
cpqt "libQt6Multimedia.so.*"
cpqt "libQt6SerialPort.so.*"
cpqt "libQt6TextToSpeech.so.*"
cpqt "libQt6PrintSupport.so.*"
cpqt "libQt6Sql.so.*"
cpqt "libQt6XcbQpa.so.*"
cpqt "libQt6DBus.so.*"
cpqt "libQt6Network.so.*"

# Display / input
cplib "libxcb.so.*"
cplib "libxcb-*.so.*"
cplib "libxkbcommon*.so.*"

# SSL
cplib "libssl.so.*"
cplib "libcrypto.so.*"

# Image format support
cplib "libjpeg.so.*"

# ICU: required by Qt5Core for Unicode/locale support
cplib "libicuuc.so.*"
cplib "libicudata.so.*"
cplib "libicui18n.so.*"

# Speech / TTS
cplib "libespeak-ng.so.*"
cplib "libspeechd.so.*"

# Flite (optional TTS engine present on Trixie)
cplib "libflite.so.*"
cplib "libflite_cmu_us_kal.so.*"
cplib "libflite_usenglish.so.*"

# Audio
cplib "libasound.so.*"
cplib "libpipewire-0.3.so.*"

# GStreamer core
cplib "libgstreamer-1.0.so.*"
cplib "libgstbase-1.0.so.*"
cplib "libgstaudio-1.0.so.*"
cplib "libgstvideo-1.0.so.*"
cplib "libgstpbutils-1.0.so.*"
cplib "libgstapp-1.0.so.*"

# GLib / GObject (dlopen'd transitively)
cplib "libglib-2.0.so.*"
cplib "libgobject-2.0.so.*"
cplib "libgmodule-2.0.so.*"
cplib "libgio-2.0.so.*"
cplib "libffi.so.*"
cplib "libpcre2-8.so.*"
cplib "liborc-0.4.so.*"

# Speech-dispatcher output modules
cp /usr/lib/aarch64-linux-gnu/speech-dispatcher/*.so \
   "${APPDIR}/usr/lib/speech-dispatcher/" 2>/dev/null || true

# ── GStreamer plugins ─────────────────────────────────────────────────────────
echo "==> Copying GStreamer plugins"
for p in libgstautodetect      libgstaudioconvert    libgstaudioresample \
          libgstplayback        libgsttypefindfunctions libgstaudiotestsrc \
          libgstalsa            libgstpulseaudio      libgstvolume        \
          libgstcoreelements    libgstapp             libgstpipewire; do
    cp "${GSTPLUG}/${p}.so" "${APPDIR}/usr/lib/gstreamer-1.0/" 2>/dev/null || true
done

# ── Fix plugin RPATHs so they resolve libs inside the AppImage ────────────────
# AppRun sets LD_LIBRARY_PATH for the main binary; plugins loaded by Qt or
# GStreamer at runtime need their own RPATH to find bundled libs.
echo "==> Patching plugin RPATHs with patchelf"
find "${APPDIR}/usr/plugins" -name "*.so" | while read -r f; do
    patchelf --set-rpath '$ORIGIN/../../../lib:$ORIGIN/../../lib:$ORIGIN/../lib' \
             "${f}" 2>/dev/null || true
done
find "${APPDIR}/usr/lib/gstreamer-1.0" -name "*.so" | while read -r f; do
    patchelf --set-rpath '$ORIGIN/..:$ORIGIN' \
             "${f}" 2>/dev/null || true
done
for lib in "${APPDIR}/usr/lib/"*.so.*; do
    [ -f "${lib}" ] && \
        patchelf --set-rpath '$ORIGIN' "${lib}" 2>/dev/null || true
done

# ── Translations ──────────────────────────────────────────────────────────────
mkdir -p "${APPDIR}/usr/share/basic256/Translations"
cp build/*.qm "${APPDIR}/usr/share/basic256/Translations/" 2>/dev/null || true

# ── Download linuxdeploy tools into /tmp ─────────────────────────────────────
TOOLS_DIR="/tmp/linuxdeploy_tools"
mkdir -p "${TOOLS_DIR}"
echo "==> Downloading linuxdeploy tools (aarch64) to ${TOOLS_DIR}"
wget -q -P "${TOOLS_DIR}" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
wget -q -P "${TOOLS_DIR}" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
chmod +x "${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
         "${TOOLS_DIR}/linuxdeploy-plugin-qt-${ARCH}.AppImage"

# ── Deploy Qt dependencies and produce .AppImage ──────────────────────────────
export QMAKE="${QT_DIR}/bin/qmake"
echo "==> Using QMAKE=${QMAKE}"
export QML_SOURCES_PATHS="."
export EXTRA_QT_PLUGINS="texttospeech;multimedia;imageformats"
export VERSION
VERSION="$(git describe --tags --always 2>/dev/null || echo 'dev')"

# linuxdeploy's own dependency walk knows nothing of $QMAKE, and the aqtinstall
# Qt is not registered with ldconfig, so point it at the Qt libraries
# (same as package_Linux_x86_AppImage.sh).
export LD_LIBRARY_PATH="${QT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

echo "==> Running linuxdeploy (VERSION=${VERSION})"
"${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
    --appdir "${APPDIR}"                     \
    --plugin qt

# GTK platform theme, so the IDE follows the desktop's light or dark theme
# (AppRun asks for it). Added only after the Qt plugin has deployed: given the
# plugin, it would bundle GTK too, and GTK must come from the user's system.
# Qt's plugins already look for the Qt libraries in ../../lib.
mkdir -p "${APPDIR}/usr/plugins/platformthemes"
cp "${QT_PLUGIN_DIR}/platformthemes/libqgtk3.so" "${APPDIR}/usr/plugins/platformthemes/"

"${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage" \
    --appdir "${APPDIR}"                     \
    --output appimage

# ── Rename output ─────────────────────────────────────────────────────────────
# With the tools in /tmp, the only *.AppImage in the workspace root is the
# one linuxdeploy just built.
for f in ./*.AppImage; do
    mv "${f}" "${ARTIFACT_NAME}.AppImage"
    echo "==> Created ${ARTIFACT_NAME}.AppImage"
    break
done

# ── Verify bundled Examples/ and Modules/math.kbs shipped in the AppImage ────
# linuxdeploy packages the staged AppDir as-is, but a silent loss here would
# ship an AppImage with no example programs or standard-library module
# (Examples was reported missing on ARM). Extract the finished image and fail
# the build if either is absent.
echo "==> Verifying Examples/ and Modules/math.kbs inside ${ARTIFACT_NAME}.AppImage"
rm -rf squashfs-root
./"${ARTIFACT_NAME}.AppImage" --appimage-extract >/dev/null
if [ -z "$(find squashfs-root/usr/share/basic256/Examples -type f -name '*.kbs' 2>/dev/null | head -n1)" ]; then
    echo "ERROR: Examples/ missing or empty in ${ARTIFACT_NAME}.AppImage" >&2
    exit 1
fi
if [ ! -f squashfs-root/usr/bin/Modules/math.kbs ]; then
    echo "ERROR: Modules/math.kbs missing in ${ARTIFACT_NAME}.AppImage" >&2
    exit 1
fi
echo "==> Examples/ and Modules/math.kbs verified in ${ARTIFACT_NAME}.AppImage"
if [ ! -f squashfs-root/usr/plugins/platformthemes/libqgtk3.so ]; then
    echo "ERROR: usr/plugins/platformthemes/libqgtk3.so missing in ${ARTIFACT_NAME}.AppImage" >&2
    exit 1
fi
if ls squashfs-root/usr/lib/libgtk-3.so* 1>/dev/null 2>&1; then
    echo "ERROR: GTK was bundled in ${ARTIFACT_NAME}.AppImage; the theme plugin must use the system's" >&2
    exit 1
fi
echo "==> GTK platform theme verified in ${ARTIFACT_NAME}.AppImage"
rm -rf squashfs-root
