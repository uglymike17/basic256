#!/usr/bin/env bash
set -euo pipefail

        cp README.md Basic256/ || true
        cp Basic256-IDE.png Basic256-CLI.png Basic256-Web.png Basic256-Web_GraphicsOnly.png Basic256/ || true

        # ICU, for Unicode/locale support. Qt's own ICU, when its build ships
        # one, was bundled with the Qt libraries in build_Linux_RPi_Trixie.sh;
        # these are the system's, for anything else that links them.
        cp /usr/lib/aarch64-linux-gnu/libicuuc.so.*   Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libicudata.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libicui18n.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libflite.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libflite_cmu_us_kal.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libflite_usenglish.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libspeechd.so.* Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/speech-dispatcher/*.so Basic256/lib/ || true
        # GStreamer runtime (needed by libqtmedia_gstreamer.so)
        cp /usr/lib/aarch64-linux-gnu/libgstreamer-1.0.so.*        Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgstbase-1.0.so.*          Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgstaudio-1.0.so.*         Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgstvideo-1.0.so.*         Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgstpbutils-1.0.so.*       Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libpipewire-0.3.so.*         Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libasound.so.*         Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgstapp-1.0.so.*           Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/liborc-0.4.so.*              Basic256/lib/ || true

        # GStreamer plugins (audioconvert, autoaudiosink, alsa/pulse)
        GSTPLUG="/usr/lib/aarch64-linux-gnu/gstreamer-1.0"
        mkdir -p Basic256/gstreamer-1.0
        for p in libgstautodetect libgstaudioconvert libgstaudioresample \
          libgstplayback libgsttypefindfunctions libgstaudiotestsrc \
          libgstalsa libgstpulseaudio libgstvolume libgstcoreelements \
          libgstapp libgstpipewire; do
          cp "$GSTPLUG/${p}.so" Basic256/gstreamer-1.0/ 2>/dev/null || true
        done

        # Patch ALL bundled libs so they resolve dependencies within Basic256/lib only
        for lib in Basic256/lib/*.so.* ; do
          [ -f "$lib" ] && patchelf --set-rpath '$ORIGIN' "$lib" 2>/dev/null || true
        done

        # Re-patch all plugins (run last, after all plugins are copied)
        find Basic256/plugins -name "*.so" -o -name "*.so.*" | while read f; do
          patchelf --set-rpath '$ORIGIN/../../../lib:$ORIGIN/../../lib' "$f" 2>/dev/null || true
        done
        find Basic256/gstreamer-1.0 -name "*.so" | while read f; do
          patchelf --set-rpath '$ORIGIN/../lib:$ORIGIN' "$f" 2>/dev/null || true
        done

        # DBOPEN needs the SQLite driver and everything it links against;
        # a missing one only shows on a Pi, as "Unable to open SQLITE
        # database", so stop the build here instead
        SQLDRV=Basic256/plugins/sqldrivers/libqsqlite.so
        if [ ! -f "$SQLDRV" ]; then
          echo "ERROR: $SQLDRV is missing from the package"; exit 1
        fi
        if LD_LIBRARY_PATH=Basic256/lib ldd "$SQLDRV" 2>&1 | grep -i "not found"; then
          echo "ERROR: the SQLite driver has unresolved dependencies (above)"; exit 1
        fi

        # The GTK platform theme is how the IDE follows a dark desktop. GTK
        # must come from the user's system, never from the package.
        if [ ! -f Basic256/plugins/platformthemes/libqgtk3.so ]; then
          echo "ERROR: plugins/platformthemes/libqgtk3.so is missing from the package"; exit 1
        fi
        if ls Basic256/lib/libgtk-3.so* 1>/dev/null 2>&1; then
          echo "ERROR: GTK was bundled; the theme plugin must use the system's"; exit 1
        fi
        # GLib likewise: GTK, and the GIO modules it loads such as gvfs, are
        # built against the Pi's own newer GLib and fail against an older one.
        if ls Basic256/lib/libglib-2.0.so* Basic256/lib/libgio-2.0.so* 1>/dev/null 2>&1; then
          echo "ERROR: GLib was bundled; the package must use the system's"; exit 1
        fi

        # Diagnostics: confirm texttospeech plugins made it into the final
        # package and that their dependencies resolve against the bundled
        # Basic256/lib (this is what the shipped artifact actually sees).
        echo "=== Final packaged texttospeech plugins ==="
        ls -la Basic256/plugins/texttospeech/ 2>&1 || echo "(directory empty or missing)"
        for so in Basic256/plugins/texttospeech/*.so; do
          [ -f "$so" ] || continue
          echo "--- ldd $so (post-patchelf) ---"
          ldd "$so" 2>&1 | grep -i "not found" && echo "  ^^^ MISSING DEPENDENCY ABOVE" || echo "  (all dependencies resolved)"
        done

        # Compiled translations
        mkdir -p Basic256/Translations
        if ls build/*.qm 1>/dev/null 2>&1; then
          cp build/*.qm Basic256/Translations/
        elif ls Translations/*.qm 1>/dev/null 2>&1; then
          cp Translations/*.qm Basic256/Translations/
        fi

        # Launcher script
        cat > Basic256/run.sh << 'EOF'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"

export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$DIR/plugins/platforms"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_TEXTTOSPEECH_PLUGINS="$DIR/plugins/texttospeech"
# Qt reads the desktop's colours, light or dark, through a platform theme
# plugin. It picks the bundled GTK one by itself on GNOME-like desktops but
# not on others, such as labwc on the Raspberry Pi, so ask for it. A theme
# already set is kept only if this package carries its plugin: anything else
# cannot load here, and Raspberry Pi OS sets qt5ct, a Qt 5 plugin, for every
# session. Qt-based desktops (KDE, LXQt) are left to Qt's own choice.
case "${XDG_CURRENT_DESKTOP:-}" in
    *KDE*|*LXQt*|*LXQT*) ;;
    *) case "${QT_QPA_PLATFORMTHEME:-}" in
           gtk3) ;;
           *) export QT_QPA_PLATFORMTHEME=gtk3 ;;
       esac ;;
esac
export PIPEWIRE_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export GST_PLUGIN_PATH="$DIR/gstreamer-1.0"
export GST_REGISTRY="$DIR/gstreamer-1.0/registry.bin"

exec "$DIR/basic256" "$@"
EOF
        chmod +x Basic256/run.sh

        tar -czf "${ARTIFACT_NAME}.tar.gz" Basic256/