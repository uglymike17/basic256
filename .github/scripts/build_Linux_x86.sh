#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt-get remove -y libunwind-14-dev || true
sudo apt install -y build-essential cmake libgl1-mesa-dev libx11-dev libxext-dev \
  libxrender-dev libxi-dev libxkbcommon-x11-0 flex bison libpulse-dev \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
  qt6-multimedia-dev qt6-serialport-dev qt6-speech-dev libpipewire-0.3-dev \
  libspeechd-dev libspeechd2 speech-dispatcher libespeak-ng-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-libav

# NOTE: package names above are best-effort for Ubuntu 22.04 (jammy)'s Qt6
# repos, not verified against the actual apt index -- if any "Unable to
# locate package" errors show up in the next log, paste it back.
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="${QT_DIR:-/usr/lib/x86_64-linux-gnu/qt6}"
cmake --build build -j"$(nproc)"
