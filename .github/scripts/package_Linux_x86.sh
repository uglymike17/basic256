#!/usr/bin/env bash
set -euo pipefail


        mkdir -p Basic256
        mkdir -p Basic256/lib
        mkdir -p Basic256/plugins/texttospeech
        mkdir -p Basic256/plugins/mediaservice
        mkdir -p Basic256/plugins/audio
        mkdir -p Basic256/plugins/imageformats
        
        cp build/basic256 Basic256/
        cp -r Examples Basic256/
        cp -r TestSuite Basic256/ || true
        cp README.md Basic256/ || true
        cp Basic256.png Basic256/ || true

        # Point PATH to the exact folder containing the jurplel-installed Qt binaries
        export QT_PLUGIN_DIR="${Qt5_DIR:-/usr/lib/x86_64-linux-gnu/qt5}/plugins"

        # Manually grab the dynamic plugins that linuxdeployqt misses statically
        cp -r $QT_PLUGIN_DIR/texttospeech/*.so Basic256/plugins/texttospeech/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/mediaservice/*.so Basic256/plugins/mediaservice/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/audio/*.so Basic256/plugins/audio/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/imageformats/*.so Basic256/plugins/imageformats/ 2>/dev/null || true
        cp /usr/lib/x86_64-linux-gnu/libespeak-ng.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libspeechd.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libasound.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse-simple.so.*  Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse-mainloop-glib.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpipewire-0.3.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libQt5MultimediaGstTools.so* Basic256/lib/ || true

        # GStreamer runtime libs
        GLIB="/usr/lib/x86_64-linux-gnu"
        cp $GLIB/libgstreamer-1.0.so.*        Basic256/lib/ || true
        cp $GLIB/libgstbase-1.0.so.*          Basic256/lib/ || true
        cp $GLIB/libgstaudio-1.0.so.*         Basic256/lib/ || true
        cp $GLIB/libgstpbutils-1.0.so.*       Basic256/lib/ || true
        cp $GLIB/libglib-2.0.so.*             Basic256/lib/ || true
        cp $GLIB/libgobject-2.0.so.*          Basic256/lib/ || true
        cp $GLIB/libgio-2.0.so.*              Basic256/lib/ || true
        mkdir -p Basic256/gstreamer-1.0
        GSTPLUG="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
        for p in libgstautodetect libgstaudioconvert libgstaudioresample \
          libgstplayback libgstalsa libgstpulseaudio libgstpipewire \
          libgstvolume libgstcoreelements \
          libgstwavparse libgstaudioparsers libgstmpegaudioparse \
          libgstmpg123 libgstlibav libgstogg libgstvorbis libgstisomp4; do
          cp "$GSTPLUG/${p}.so" Basic256/gstreamer-1.0/ 2>/dev/null || true
        done

        # Download linuxdeployqt asset into /tmp — NOT the workspace root.
        # This is a build tool, not a deliverable; downloading it to the
        # workspace would cause it to be swept up by the CI upload step's
        # "*.AppImage" glob alongside the actual BASIC256 AppImage.
        LDQT="/tmp/linuxdeployqt-continuous-x86_64.AppImage"
        wget -c -nv -O "${LDQT}" "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        chmod a+x "${LDQT}"
        
        # Force linuxdeployqt to trace dependencies for our manually copied plugins
        EXTRA_ARGS=""
        for plugin in Basic256/plugins/*/*.so; do
          if [ -f "$plugin" ]; then
            EXTRA_ARGS="$EXTRA_ARGS -executable=$plugin"
          fi
        done

        # Force linuxdeployqt to look at the system Qt5 path
        export PATH="/usr/lib/qt5/bin:$PATH"
        
        # Run linuxdeployqt with the plugin arguments
        "${LDQT}" Basic256/basic256 \
          -bundle-non-qt-libs \
          -extra-plugins=texttospeech,mediaservice,audio,imageformats \
          --appimage-extract-and-run

        # Ensure the binary actually lives in Basic256/ alongside the libraries
        if [ ! -f "Basic256/basic256" ] && [ -f "build/basic256" ]; then
            cp build/basic256 Basic256/
        fi

        #Launcher script
        cat > Basic256/run.sh << 'EOF'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"
export GST_PLUGIN_PATH="$DIR/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH=""
export GST_REGISTRY="$DIR/gstreamer-1.0/registry.bin"
exec "$DIR/basic256" "$@"
EOF
        chmod +x Basic256/run.sh

        # Compress everything into your final delivery asset
        tar -czf "${ARTIFACT_NAME}.tar.gz" Basic256/