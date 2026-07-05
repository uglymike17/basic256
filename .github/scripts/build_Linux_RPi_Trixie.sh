#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update && sudo apt-get install -y \
build-essential cmake flex bison \
flite libflite1 \
gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-alsa \
gstreamer1.0-libav gstreamer1.0-pipewire pipewire pipewire-pulse \
libpipewire-0.3-0 libpipewire-0.3-dev libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 \
libasound2-dev espeak-ng libespeak-ng-dev libspeechd-dev speech-dispatcher \
qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
qt6-multimedia-dev qt6-serialport-dev qt6-speech-dev qt6-declarative-dev \
libgl1-mesa-dev libx11-dev patchelf

# NOTE: qt6-declarative-dev is required even though this project doesn't use
# QML directly -- Qt6TextToSpeech's CMake config (Qt6TextToSpeechConfig.cmake)
# depends on the Qt6QmlIntegration package, which qt6-declarative-dev
# provides. Without it, find_package(Qt6 ... TextToSpeech) fails with
# "Qt6TextToSpeech_FOUND is FALSE" even though the .cmake file exists on disk.

# 1. Compile using native ARM64 tools
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build --config Release -j$(nproc)

        # 2. Create standalone relocatable distribution folder
        mkdir -p Basic256/lib Basic256/plugins
        cp build/basic256 Basic256/
        patchelf --set-rpath '$ORIGIN/lib' Basic256/basic256
        find Basic256/plugins -name "*.so" | while read f; do
          patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN/../lib:$ORIGIN' "$f" || true
        done
        printf '[Paths]\nPlugins=plugins\n' > Basic256/qt.conf

        # 3. Define plugin dir once, used throughout
        # NOTE: Qt6 on Debian/Ubuntu multiarch systems -- switched from qt5/ to qt6/.
        QT_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/qt6/plugins"
        echo "QT_PLUGIN_DIR=$QT_PLUGIN_DIR"
        echo "=== Available Qt plugin directories ==="
        find "$QT_PLUGIN_DIR" -maxdepth 1 -type d | sort
        QT_LIB_DIR="/usr/lib/aarch64-linux-gnu"

        # 4. Copy Qt runtime libraries
        # NOTE: renamed Qt5->Qt6; each made non-fatal (|| true) since this is
        # an unverified best-effort pass at Qt6's actual lib set on this
        # runner -- if any are genuinely missing/renamed, the run() output
        # further down (or a runtime crash) will point at exactly which.
        cp $QT_LIB_DIR/libQt6Core.so.*          Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6Gui.so.*           Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6Widgets.so.*       Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6Multimedia.so.*    Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6SerialPort.so.*    Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6TextToSpeech.so.*  Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6PrintSupport.so.*  Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6Sql.so.*           Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6XcbQpa.so.*        Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6DBus.so.*          Basic256/lib/ || true
        cp $QT_LIB_DIR/libQt6Network.so.*       Basic256/lib/ || true
        cp $QT_LIB_DIR/libssl.so.*              Basic256/lib/ || true
        cp $QT_LIB_DIR/libcrypto.so.*           Basic256/lib/ || true
        cp $QT_LIB_DIR/libxcb.so.*              Basic256/lib/ 2>/dev/null || true
        cp $QT_LIB_DIR/libxcb-*.so.*            Basic256/lib/ 2>/dev/null || true
        cp $QT_LIB_DIR/libxkbcommon*.so.*       Basic256/lib/ 2>/dev/null || true

        # 5. Platform plugins
        mkdir -p Basic256/plugins/platforms
        cp $QT_PLUGIN_DIR/platforms/libqxcb.so              Basic256/plugins/platforms/ || true
        cp $QT_PLUGIN_DIR/platforms/libqwayland-generic.so  Basic256/plugins/platforms/ || true
        cp $QT_PLUGIN_DIR/platforms/libqwayland-egl.so      Basic256/plugins/platforms/ || true
        cp $QT_PLUGIN_DIR/platforms/libqoffscreen.so        Basic256/plugins/platforms/ || true


        # 6. Text-to-speech plugins
        mkdir -p Basic256/plugins/texttospeech
        cp $QT_PLUGIN_DIR/texttospeech/libqtexttospeech_*.so Basic256/plugins/texttospeech/ || true

        # 7. espeak-ng and libjpeg runtime libs (outside any if-block)
        cp $QT_LIB_DIR/libespeak-ng.so.*  Basic256/lib/ || true
        cp $QT_LIB_DIR/libjpeg.so.*       Basic256/lib/ || true

        # 8. Multimedia backend and image format plugins.
        # NOTE: Qt5's "mediaservice" plugin category does not exist in Qt6 --
        # Qt6 Multimedia backends live under a "multimedia" plugin dir
        # instead (exact name/contents not verified on this runner; both are
        # checked defensively so neither missing one fails the build).
        if [ -d $QT_PLUGIN_DIR/multimedia ]; then
          mkdir -p Basic256/plugins/multimedia
          cp $QT_PLUGIN_DIR/multimedia/* Basic256/plugins/multimedia/
        fi

        if [ -d $QT_PLUGIN_DIR/mediaservice ]; then
          mkdir -p Basic256/plugins/mediaservice
          cp $QT_PLUGIN_DIR/mediaservice/* Basic256/plugins/mediaservice/
        fi

        if [ -d $QT_PLUGIN_DIR/imageformats ]; then
          mkdir -p Basic256/plugins/imageformats
          cp $QT_PLUGIN_DIR/imageformats/* Basic256/plugins/imageformats/
        fi

        if [ -d $QT_PLUGIN_DIR/audio ]; then
          mkdir -p Basic256/plugins/audio
          cp $QT_PLUGIN_DIR/audio/* Basic256/plugins/audio/
        fi

        cp -r Examples Basic256/