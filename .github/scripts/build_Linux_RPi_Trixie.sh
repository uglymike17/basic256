#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update && sudo apt-get install -y \
build-essential cmake flex bison \
flite libflite1 \
gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-alsa \
gstreamer1.0-libav gstreamer1.0-pipewire pipewire pipewire-pulse \
libpipewire-0.3-0 libpipewire-0.3-dev libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 \
libasound2-dev espeak-ng libespeak-ng-dev libspeechd-dev speech-dispatcher \
qtbase5-dev qttools5-dev-tools qttools5-dev qtmultimedia5-dev libqt5multimedia5 libqt5multimedia5-plugins \
 libqt5texttospeech5 libqt5texttospeech5-dev libqt5serialport5 libqt5serialport5-dev \
libgl1-mesa-dev libx11-dev patchelf

#libqt5multimedia libqt5multimedia-plugins libqt5multimedia5 libqt5multimedia5-plugins 

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
        QT_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/qt5/plugins"
        echo "QT_PLUGIN_DIR=$QT_PLUGIN_DIR"
        echo "=== Available Qt plugin directories ==="
        find "$QT_PLUGIN_DIR" -maxdepth 1 -type d | sort
        QT_LIB_DIR="/usr/lib/aarch64-linux-gnu"

        # 4. Copy Qt runtime libraries
        cp $QT_LIB_DIR/libQt5Core.so.*          Basic256/lib/
        cp $QT_LIB_DIR/libQt5Gui.so.*           Basic256/lib/
        cp $QT_LIB_DIR/libQt5Widgets.so.*       Basic256/lib/
        cp $QT_LIB_DIR/libQt5Multimedia.so.*    Basic256/lib/
        cp $QT_LIB_DIR/libQt5MultimediaGstTools.so.*    Basic256/lib/
        cp $QT_LIB_DIR/libQt5SerialPort.so.*    Basic256/lib/
        cp $QT_LIB_DIR/libQt5TextToSpeech.so.*  Basic256/lib/
        cp $QT_LIB_DIR/libQt5PrintSupport.so.*  Basic256/lib/
        cp $QT_LIB_DIR/libQt5Sql.so.*           Basic256/lib/
        cp $QT_LIB_DIR/libQt5XcbQpa.so.*        Basic256/lib/
        cp $QT_LIB_DIR/libQt5DBus.so.*          Basic256/lib/
        cp $QT_LIB_DIR/libQt5Network.so.*       Basic256/lib/
        cp $QT_LIB_DIR/libssl.so.*              Basic256/lib/
        cp $QT_LIB_DIR/libcrypto.so.*           Basic256/lib/
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

        # 8. Media service and image format plugins
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