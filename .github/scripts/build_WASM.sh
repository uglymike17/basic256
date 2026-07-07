#!/usr/bin/env bash
set -euo pipefail

# Native build tools only: flex/bison generate LEX/basicParse.{y,l} into C
# on the *host*, regardless of target -- WASM cross-compiles the app, not
# these host-side codegen tools. cmake/build-essential are needed to build
# and to run the FLEX_TARGET/BISON_TARGET custom commands.
# Tolerate transient failures on repos we don't even use (e.g.
# packages.microsoft.com's azure-cli repo intermittently fails GPG
# verification on GitHub's hosted runners) -- apt update failing on one
# unrelated repo shouldn't block installing from the repos that did sync.
sudo apt update || true
sudo apt install -y build-essential cmake flex bison

# -------------------------------------------------------------------
# Emscripten SDK -- pinned to 4.0.7, the exact version Qt 6.11.1's
# WebAssembly binaries were built with. Qt does not guarantee ABI
# compatibility across emsdk versions, so this must track WASM.md's
# "Decisions already made" pin, not just "latest".
# -------------------------------------------------------------------
if [ ! -d "$HOME/emsdk" ]; then
    git clone https://github.com/emscripten-core/emsdk.git "$HOME/emsdk"
fi
pushd "$HOME/emsdk"
./emsdk install 4.0.7
./emsdk activate 4.0.7
# shellcheck disable=SC1091
source ./emsdk_env.sh
popd

# -------------------------------------------------------------------
# Qt 6.11.1: the WASM target SDK (wasm_multithread -- the multithreaded
# build is required, see RULE 2/the interpreter's QThread) plus a
# same-version desktop host Qt (QT_HOST_PATH -- moc/rcc/uic/lrelease run
# on the host, not in the browser). --autodesktop installs the host Qt
# automatically alongside the wasm one.
# qtmultimedia is the only extra module needed on the wasm side: Sql/
# SerialPort/PrintSupport/TextToSpeech are unnecessary (and PrintSupport
# isn't offered for wasm at all -- see CMakeLists.txt's Phase 4 PrintSupport
# correction) since every BASIC256_ENABLE_* flag below is OFF.
# -------------------------------------------------------------------
python3 -m pip install aqtinstall
aqt install-qt all_os wasm 6.11.1 wasm_multithread -m qtmultimedia --autodesktop

# Discover the actual installed subdirs rather than assuming exact names --
# aqt has a history of adjusting directory names per-arch (see
# build_Linux_x86.sh's own note on this), and this is the *first* time this
# repo has installed a wasm + host Qt pair, so don't hardcode "gcc_64" /
# "wasm_multithread" as literal directory names.
QT_WASM_DIR="$(find "$GITHUB_WORKSPACE/6.11.1" -mindepth 1 -maxdepth 1 -type d -iname '*wasm*' | head -n1)"
QT_HOST_DIR="$(find "$GITHUB_WORKSPACE/6.11.1" -mindepth 1 -maxdepth 1 -type d -not -iname '*wasm*' | head -n1)"
echo "Resolved QT_WASM_DIR=$QT_WASM_DIR"
echo "Resolved QT_HOST_DIR=$QT_HOST_DIR"
if [ -z "$QT_WASM_DIR" ] || [ -z "$QT_HOST_DIR" ]; then
    echo "Failed to resolve Qt wasm/host install directories under $GITHUB_WORKSPACE/6.11.1" >&2
    find "$GITHUB_WORKSPACE/6.11.1" -mindepth 1 -maxdepth 1 -type d >&2
    exit 1
fi

# -------------------------------------------------------------------
# Configure + build with Qt's own wasm toolchain wrapper (qt-cmake).
# All six BASIC256_ENABLE_* flags OFF: none of those Qt modules exist in
# (or make sense for) the WASM SDK -- see WASM.md Phase 3/4.
# -------------------------------------------------------------------
"$QT_WASM_DIR/bin/qt-cmake" -S . -B build-wasm \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_HOST_PATH="$QT_HOST_DIR" \
    -DBASIC256_ENABLE_PROCESS=OFF \
    -DBASIC256_ENABLE_SERIAL=OFF \
    -DBASIC256_ENABLE_SQL=OFF \
    -DBASIC256_ENABLE_PRINTER=OFF \
    -DBASIC256_ENABLE_TCP=OFF \
    -DBASIC256_ENABLE_TTS=OFF

cmake --build build-wasm -j"$(nproc)"

echo "== build-wasm output =="
ls -la build-wasm/basic256.* build-wasm/qtloader.js build-wasm/qtlogo.svg
