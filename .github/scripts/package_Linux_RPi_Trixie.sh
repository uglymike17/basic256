#!/usr/bin/env bash
set -euo pipefail

        cp README.md Basic256/ || true
        cp Basic256.png Basic256/ || true

        # ICU libraries - required by Qt5Core at runtime for Unicode/locale support
        cp /usr/lib/aarch64-linux-gnu/libicuuc.so.*   Basic256/lib/
        cp /usr/lib/aarch64-linux-gnu/libicudata.so.* Basic256/lib/
        cp /usr/lib/aarch64-linux-gnu/libicui18n.so.* Basic256/lib/
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
        cp /usr/lib/aarch64-linux-gnu/libglib-2.0.so.*             Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgobject-2.0.so.*          Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgmodule-2.0.so.*          Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libgio-2.0.so.*              Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libffi.so.*                  Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/libpcre2-8.so.*              Basic256/lib/ || true
        cp /usr/lib/aarch64-linux-gnu/liborc-0.4.so.*              Basic256/lib/ || true

        QT_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/qt6/plugins"
        
        if [ -d $QT_PLUGIN_DIR/audio ]; then
          mkdir -p Basic256/plugins/audio
          cp $QT_PLUGIN_DIR/audio/* Basic256/plugins/audio/
        fi

        mkdir -p Basic256/plugins/mediaservice
        if [ -d $QT_PLUGIN_DIR/mediaservice ]; then
          cp $QT_PLUGIN_DIR/mediaservice/* Basic256/plugins/mediaservice/
        fi

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
        export PIPEWIRE_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
        export GST_PLUGIN_PATH="$DIR/gstreamer-1.0"
        export GST_REGISTRY="$DIR/gstreamer-1.0/registry.bin"
        export QT_DEBUG_PLUGINS=1

        exec "$DIR/basic256" "$@"
EOF
        chmod +x Basic256/run.sh

        tar -czf "${ARTIFACT_NAME}.tar.gz" Basic256/