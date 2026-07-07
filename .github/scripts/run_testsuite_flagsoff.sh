#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# run_testsuite_flagsoff.sh <path-to-basic256-binary>
#
# Phase 3 dress rehearsal (WASM.md): runs TestSuite/testsuite_flagsoff_ci.kbs
# headlessly via `basic256 -s`, against a build configured with all six
# BASIC256_ENABLE_* flags OFF (see build_Linux_x86_flagsoff.sh). Verifies
# that SYSTEM raises the new ERROR_NOTAVAILABLE (129) and execution
# continues normally, instead of crashing, hanging, or silently no-opping.
#
# Same output-checking convention as run_testsuite.sh: a failed assertion
# does not change basic256's exit code (BASIC-256's `end` statement is a
# normal, deliberate stop), so this script greps the captured output for
# "fail" and for the final "BASIC256_FLAGSOFF_CI_PASSED" marker, in
# addition to the exit code (which still catches a genuine interpreter
# crash/script-load error).
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

BASIC256_BIN_ARG="${1:?Usage: run_testsuite_flagsoff.sh <path-to-basic256-binary>}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTSUITE_DIR="$(cd "${SCRIPT_DIR}/../../TestSuite" && pwd)"
LOG_FILE="$(mktemp)"

if command -v realpath >/dev/null 2>&1; then
    BASIC256_BIN="$(realpath "${BASIC256_BIN_ARG}")"
else
    BASIC256_BIN="$(cd "$(dirname "${BASIC256_BIN_ARG}")" && pwd)/$(basename "${BASIC256_BIN_ARG}")"
fi

if [ "$(uname -s)" = "Linux" ]; then
    export QT_QPA_PLATFORM=offscreen
fi

if [ -n "${QT_DIR:-}" ]; then
    export LD_LIBRARY_PATH="${QT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
    export QT_PLUGIN_PATH="${QT_DIR}/plugins"
fi

echo "==> Running flags-off dress rehearsal via: ${BASIC256_BIN} -s testsuite_flagsoff_ci.kbs"
( cd "${TESTSUITE_DIR}" && "${BASIC256_BIN}" -s testsuite_flagsoff_ci.kbs ) 2>&1 | tee "${LOG_FILE}"
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

if ! grep -q "BASIC256_FLAGSOFF_CI_PASSED" "${LOG_FILE}"; then
    echo "==> FAIL: final 'BASIC256_FLAGSOFF_CI_PASSED' marker not found (script stopped early)"
    FAILED=1
fi

rm -f "${LOG_FILE}"

if [ "${FAILED}" -ne 0 ]; then
    exit 1
fi

echo "==> Flags-off dress rehearsal passed."
