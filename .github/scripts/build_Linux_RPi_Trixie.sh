#!/usr/bin/env bash
set -euo pipefail

# Tolerate transient failures on repos we don't even use (e.g.
# packages.microsoft.com's azure-cli repo intermittently fails GPG
# verification on GitHub's hosted runners) -- apt update failing on one
# unrelated repo shouldn't block installing from the repos that did sync.
# Previously chained with && (set -e made this fatal on ANY repo failure,
# including this unrelated one -- caused a real, confirmed CI failure).
sudo apt-get update || true

# Non-Qt build/runtime dependencies only. Qt itself comes from aqtinstall
# below, as on Linux x86: the runner's own Qt packages are Ubuntu 24.04's
# Qt 6.4, older than the 6.5 that View/Theme needs to follow the desktop's
# light or dark setting (Follow System was greyed out on the Pi). The libxcb-*
# set is Qt's documented runtime dependency list for the xcb platform plugin.
sudo apt-get install -y \
build-essential cmake flex bison patchelf \
libgl1-mesa-dev libegl1 libx11-dev libxext-dev libxrender-dev libxi-dev \
libfontconfig1 libxkbcommon0 libxkbcommon-x11-0 libxcb-cursor0 \
libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 \
libcups2 libpulse-dev \
flite libflite1 \
gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-alsa \
gstreamer1.0-libav gstreamer1.0-pipewire pipewire pipewire-pulse \
libpipewire-0.3-0 libpipewire-0.3-dev libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 \
libasound2-dev espeak-ng libespeak-ng-dev libspeechd-dev speech-dispatcher

# Official Qt for arm64 Linux via aqtinstall (host linux_arm64, available
# from Qt 6.7). Same version and modules as build_Linux_x86.sh.
python3 -m pip install aqtinstall
aqt install-qt linux_arm64 desktop 6.11.1 linux_gcc_arm64 -m qtmultimedia qtserialport qtspeech

# Discover the installed subdir rather than assuming its name (aqt strips
# host prefixes from the arch when naming it), and export QT_DIR for the
# packaging steps -- $GITHUB_ENV only takes effect from the next step.
QT_DIR="$(find "$GITHUB_WORKSPACE/6.11.1" -mindepth 1 -maxdepth 1 -type d | head -n1)"
export QT_DIR
echo "QT_DIR=$QT_DIR" >> "$GITHUB_ENV"
echo "Resolved QT_DIR=$QT_DIR"

# 1. Compile using native ARM64 tools
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT_DIR"
        cmake --build build --config Release -j$(nproc)

        # 2. Create standalone relocatable distribution folder
        mkdir -p Basic256/lib Basic256/plugins
        cp build/basic256 Basic256/
        patchelf --set-rpath '$ORIGIN/lib' Basic256/basic256
        printf '[Paths]\nPlugins=plugins\n' > Basic256/qt.conf

        # 3. Qt comes from the aqtinstall tree; everything else is the system's
        QT_PLUGIN_DIR="$QT_DIR/plugins"
        echo "QT_PLUGIN_DIR=$QT_PLUGIN_DIR"
        echo "=== Available Qt plugin directories ==="
        find "$QT_PLUGIN_DIR" -maxdepth 1 -type d | sort
        SYS_LIB_DIR="/usr/lib/aarch64-linux-gnu"

        # 4. Non-Qt libraries the Qt libraries and plugins link against. The Qt
        # libraries themselves are collected in step 9, once every plugin is in.
        cp $SYS_LIB_DIR/libssl.so.*              Basic256/lib/ || true
        cp $SYS_LIB_DIR/libcrypto.so.*           Basic256/lib/ || true
        cp $SYS_LIB_DIR/libxcb.so.*              Basic256/lib/ 2>/dev/null || true
        cp $SYS_LIB_DIR/libxcb-*.so.*            Basic256/lib/ 2>/dev/null || true
        cp $SYS_LIB_DIR/libxkbcommon*.so.*       Basic256/lib/ 2>/dev/null || true

        # 5. Platform plugins
        mkdir -p Basic256/plugins/platforms
        cp $QT_PLUGIN_DIR/platforms/libqxcb.so              Basic256/plugins/platforms/
        cp $QT_PLUGIN_DIR/platforms/libqwayland*.so         Basic256/plugins/platforms/ 2>/dev/null || true
        cp $QT_PLUGIN_DIR/platforms/libqoffscreen.so        Basic256/plugins/platforms/ || true

        # 5a. GTK platform theme, so the IDE follows the desktop's light or dark
        # theme (run.sh asks for it). GTK itself is deliberately not bundled:
        # the plugin uses the system's, which every desktop with a GTK theme has.
        mkdir -p Basic256/plugins/platformthemes
        cp $QT_PLUGIN_DIR/platformthemes/libqgtk3.so Basic256/plugins/platformthemes/

        # 5b. SQLite driver for DBOPEN. run.sh points Qt at the bundled plugins
        # only, so without this QSQLITE is "not loaded" and every DBOPEN fails.
        # Only SQLite: the other drivers pull in client libraries this package
        # does not ship.
        mkdir -p Basic256/plugins/sqldrivers
        cp $QT_PLUGIN_DIR/sqldrivers/libqsqlite.so Basic256/plugins/sqldrivers/

        # 6. Text-to-speech plugins
        echo "=== texttospeech plugins available ($QT_PLUGIN_DIR/texttospeech) ==="
        ls -la "$QT_PLUGIN_DIR/texttospeech" 2>&1 || echo "(directory does not exist)"
        mkdir -p Basic256/plugins/texttospeech
        cp $QT_PLUGIN_DIR/texttospeech/libqtexttospeech_*.so Basic256/plugins/texttospeech/ || true
        echo "=== texttospeech plugins copied into package ==="
        ls -la Basic256/plugins/texttospeech/ 2>&1 || echo "(directory empty or missing)"

        # 7. espeak-ng and libjpeg runtime libs (outside any if-block)
        cp $SYS_LIB_DIR/libespeak-ng.so.*  Basic256/lib/ || true
        cp $SYS_LIB_DIR/libjpeg.so.*       Basic256/lib/ || true

        # 8. Multimedia backend and image format plugins. Qt 6 keeps its
        # multimedia backends (FFmpeg, GStreamer) under "multimedia".
        if [ -d $QT_PLUGIN_DIR/multimedia ]; then
          mkdir -p Basic256/plugins/multimedia
          cp $QT_PLUGIN_DIR/multimedia/* Basic256/plugins/multimedia/
        fi

        if [ -d $QT_PLUGIN_DIR/imageformats ]; then
          mkdir -p Basic256/plugins/imageformats
          cp $QT_PLUGIN_DIR/imageformats/* Basic256/plugins/imageformats/
        fi

        # 9. Every library from the Qt tree that the binary or a bundled plugin
        # needs -- the Qt modules, and whatever Qt ships beside them (FFmpeg for
        # the multimedia backend, ICU). Followed through ldd and repeated until
        # nothing new turns up, so the list never has to be kept by hand.
        while :; do
          before=$(ls Basic256/lib | wc -l)
          for f in Basic256/basic256 $(find Basic256/plugins Basic256/lib -name '*.so*' -type f); do
            LD_LIBRARY_PATH="$QT_DIR/lib" ldd "$f" 2>/dev/null | awk '{print $3}' | grep "^$QT_DIR/lib/" || true
          done | sort -u | while read -r dep; do
            [ -e "Basic256/lib/$(basename "$dep")" ] || cp -L "$dep" Basic256/lib/
          done
          [ "$(ls Basic256/lib | wc -l)" -eq "$before" ] && break
        done
        echo "=== Qt libraries bundled from $QT_DIR/lib ==="
        ls Basic256/lib | grep -v '^libxcb' || true

        cp -r Examples Basic256/
        # the MKDIR test leaves an empty filegatesub behind whenever the
        # TestSuite has been run in this checkout
        rm -rf TestSuite/filegatesub
        cp -r TestSuite Basic256/
        # Bundled module library, beside the binary so include "math.kbs" resolves
        cp -r Modules Basic256/
