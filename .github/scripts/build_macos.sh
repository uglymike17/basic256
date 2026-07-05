#!/usr/bin/env bash
set -euo pipefail

APP_TARGET="basic256"
APP_BUNDLE="build/${APP_TARGET}.app"
ARTIFACT_NAME="${ARTIFACT_NAME:-basic256-MacOS.zip}"

echo "==> Installing Qt6 via Homebrew..."
brew install qt

# FIX: Export directly into the current shell session so CMake can find it right now
export PATH="/opt/homebrew/opt/qt/bin:$PATH"

# Also update GITHUB_PATH for any subsequent steps in your workflow file
echo "/opt/homebrew/opt/qt/bin" >> "$GITHUB_PATH"

echo "==> Configuring CMake for macOS ARM64..."
cmake -B build -S . \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_BUILD_TYPE=Release

echo "==> Building Project..."
cmake --build build --config Release
