#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# run_testsuite.sh <path-to-basic256-binary>
#
# Runs TestSuite/testsuite_ci.kbs headlessly via `basic256 -s` and fails the
# CI step if the interpreter crashes/errors OR if any test assertion failed.
#
# A failed assertion does NOT change basic256's exit code -- BASIC-256's
# `end` statement is a normal, deliberate stop, used both when
# testsuite_common_include.kbs's same()/different()/etc. helpers detect a
# failure (they print "fail" then call `end`) and on the success path at the
# end of testsuite_ci.kbs. So this script checks the captured output for the
# literal word "fail" and for the final "BASIC256_TESTSUITE_CI_PASSED"
# marker, in addition to the exit code (which still catches a genuine
# interpreter crash/script-load error).
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

BASIC256_BIN_ARG="${1:?Usage: run_testsuite.sh <path-to-basic256-binary>}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTSUITE_DIR="$(cd "${SCRIPT_DIR}/../../TestSuite" && pwd)"
LOG_FILE="$(mktemp)"

# Resolve to an absolute path BEFORE cd'ing into TestSuite/ below -- the
# caller passes a path relative to the repo root (e.g. "build/basic256"),
# which stops resolving the moment the working directory changes.
if command -v realpath >/dev/null 2>&1; then
    BASIC256_BIN="$(realpath "${BASIC256_BIN_ARG}")"
else
    BASIC256_BIN="$(cd "$(dirname "${BASIC256_BIN_ARG}")" && pwd)/$(basename "${BASIC256_BIN_ARG}")"
fi

# GitHub Actions Linux runners have no display server at all; the
# "offscreen" QPA platform lets Qt widgets construct without one. Harmless
# to set on other OSes too (silent mode never shows a window regardless),
# but only Linux strictly needs it.
if [ "$(uname -s)" = "Linux" ]; then
    export QT_QPA_PLATFORM=offscreen
fi

# This runs *before* packaging, straight from the raw build tree: Qt6 has
# not been staged next to the binary yet (that's windeployqt/linuxdeployqt's
# job), and on platforms using an aqtinstall Qt6 (not a system apt package),
# the shared libs aren't registered with the system loader either, and the
# platform plugin (offscreen/xcb) won't be found next to the binary either.
# The binary's own BUILD_WITH_INSTALL_RPATH ("$ORIGIN/lib", see
# CMakeLists.txt) doesn't help here -- that "lib" directory only exists once
# packaging has assembled it. QT_DIR is set by build_Linux_x86.sh
# (aqtinstall); it's unset on RPi/Trixie, which uses ldconfig-registered
# system Qt6 with its plugins in Qt's own compiled-in default path already
# findable without this.
if [ -n "${QT_DIR:-}" ]; then
    export LD_LIBRARY_PATH="${QT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
    export QT_PLUGIN_PATH="${QT_DIR}/plugins"
fi

echo "==> Running TestSuite (unattended subset) via: ${BASIC256_BIN} -s testsuite_ci.kbs"
( cd "${TESTSUITE_DIR}" && "${BASIC256_BIN}" -s testsuite_ci.kbs ) 2>&1 | tee "${LOG_FILE}"
EXIT_CODE="${PIPESTATUS[0]}"

FAILED=0

if [ "${EXIT_CODE}" -ne 0 ]; then
    echo "==> FAIL: basic256 exited with code ${EXIT_CODE} (interpreter error/crash)"
    FAILED=1
fi

if grep -qw "fail" "${LOG_FILE}"; then
    echo "==> FAIL: at least one test assertion printed 'fail'"
    FAILED=1
fi

if ! grep -q "BASIC256_TESTSUITE_CI_PASSED" "${LOG_FILE}"; then
    echo "==> FAIL: final 'BASIC256_TESTSUITE_CI_PASSED' marker not found (script stopped early)"
    FAILED=1
fi

rm -f "${LOG_FILE}"

if [ "${FAILED}" -ne 0 ]; then
    exit 1
fi

echo "==> TestSuite passed."
