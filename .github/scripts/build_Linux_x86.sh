#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt-get remove -y libunwind-14-dev || true

# Non-Qt build/runtime dependencies only. Qt6 itself comes from aqtinstall
# below: Ubuntu 22.04 (jammy)'s apt repos don't ship a complete official
# Qt6 (no qt6-speech-dev / Qt6TextToSpeech at all, and serialport is named
# libqt6serialport6-dev, not qt6-serialport-dev) -- confirmed via
# packages.ubuntu.com, not just found missing at apt-install time.
sudo apt install -y build-essential cmake libgl1-mesa-dev libx11-dev libxext-dev \
  libxrender-dev libxi-dev libxkbcommon-x11-0 libxcb-cursor0 flex bison libpulse-dev \
  libpipewire-0.3-dev libcups2-dev \
  libspeechd-dev libspeechd2 speech-dispatcher libespeak-ng-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-libav \
  libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
  libxcb-shape0 libxcb-xinerama0 libxcb-xkb1

# NOTE: the libxcb-* set above is Qt's own documented runtime dependency
# list for the xcb platform plugin (qxcb) on Linux -- added after
# linuxdeployqt's ldd trace flagged libxcb-icccm.so.4 as missing; the rest
# are included proactively rather than fixing them one CI round-trip at a
# time.

# Install official Qt6 via aqtinstall, mirroring build_Windows.ps1, instead
# of relying on Ubuntu's Qt6 packaging.
python3 -m pip install aqtinstall
aqt install-qt linux desktop 6.7.3 linux_gcc_64 -m qtmultimedia qtserialport qtspeech

# set Qt6 dir (also export for later steps, e.g. packaging - $GITHUB_ENV only
# takes effect starting from the *next* step, not later in this same script).
# Discover the actual installed subdir rather than assuming it matches the
# "linux_gcc_64" arch argument verbatim -- aqt has a history of stripping
# host-redundant prefixes from the arch name when naming the install dir
# (e.g. on Windows, arch "win64_msvc2019_64" installs into "msvc2019_64").
QT_DIR="$(find "$GITHUB_WORKSPACE/6.7.3" -mindepth 1 -maxdepth 1 -type d | head -n1)"
export QT_DIR
echo "QT_DIR=$QT_DIR" >> "$GITHUB_ENV"
echo "Resolved QT_DIR=$QT_DIR"

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT_DIR"
cmake --build build -j"$(nproc)"
