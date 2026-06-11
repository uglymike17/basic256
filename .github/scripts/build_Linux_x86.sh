#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt-get remove -y libunwind-14-dev || true
sudo apt install -y build-essential cmake libgl1-mesa-dev libx11-dev libxext-dev \
  libxrender-dev libxi-dev libxkbcommon-x11-0 flex bison \
  qtbase5-dev qttools5-dev-tools qttools5-dev qtmultimedia5-dev libpipewire-0.3-dev \
  libqt5multimedia5 libqt5multimedia5-plugins libqt5texttospeech5-dev libqt5serialport5-dev \
  libspeechd-dev libspeechd2 speech-dispatcher libespeak-ng-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

#cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="${Qt5_DIR:-$Qt5_Dir}"
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="${Qt5_DIR:-/usr/lib/x86_64-linux-gnu/qt5}"
cmake --build build --config Release -j"$(nproc)"
