#!/usr/bin/env bash
set -euo pipefail


        mkdir -p dist
        mkdir -p dist/lib
        mkdir -p dist/plugins/texttospeech
        mkdir -p dist/plugins/mediaservice
        mkdir -p dist/plugins/audio
        mkdir -p dist/plugins/imageformats
        
        cp build/basic256 dist/
        cp -r Examples dist/
        cp -r TestSuite dist/ || true
        cp README.md dist/ || true

        # Point PATH to the exact folder containing the jurplel-installed Qt binaries
        export PATH="$Qt5_Dir/bin:$PATH"
        export QT_PLUGIN_DIR="$Qt5_Dir/plugins"

        # Manually grab the dynamic plugins that linuxdeployqt misses statically
        cp -r $QT_PLUGIN_DIR/texttospeech/*.so dist/plugins/texttospeech/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/mediaservice/*.so dist/plugins/mediaservice/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/audio/*.so dist/plugins/audio/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/imageformats/*.so dist/plugins/imageformats/ 2>/dev/null || true
        cp /usr/lib/x86_64-linux-gnu/libespeak-ng.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libspeechd.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libasound.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse-mainloop-glib.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpipewire-0.3.so.* dist/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libQt5MultimediaGstTools.so* dist/lib/ || true


        # Download linuxdeployqt asset
        wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        chmod a+x linuxdeployqt-continuous-x86_64.AppImage
        
        # Force linuxdeployqt to trace dependencies for our manually copied plugins
        EXTRA_ARGS=""
        for plugin in dist/plugins/*/*.so; do
          if [ -f "$plugin" ]; then
            EXTRA_ARGS="$EXTRA_ARGS -executable=$plugin"
          fi
        done
        
        # Run linuxdeployqt with the plugin arguments
        ./linuxdeployqt-continuous-x86_64.AppImage dist/basic256 \
          -bundle-non-qt-libs \
          -extra-plugins=texttospeech,mediaservice,audio,imageformats \
          --appimage-extract-and-run

        # Compress everything into your final delivery asset
        tar -czf ${{ matrix.artifact_name }}.tar.gz dist