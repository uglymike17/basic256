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

BASIC256_BIN="${1:?Usage: run_testsuite.sh <path-to-basic256-binary>}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTSUITE_DIR="$(cd "${SCRIPT_DIR}/../../TestSuite" && pwd)"
LOG_FILE="$(mktemp)"

# GitHub Actions Linux runners have no display server at all; the
# "offscreen" QPA platform lets Qt widgets construct without one. Harmless
# to set on other OSes too (silent mode never shows a window regardless),
# but only Linux strictly needs it.
if [ "$(uname -s)" = "Linux" ]; then
    export QT_QPA_PLATFORM=offscreen
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
