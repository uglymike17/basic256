#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# run_examples.sh <path-to-basic256-binary>
#
# Runs every example listed in TestSuite/testsuite_examples_ci.txt headlessly
# via `basic256 -s`, one process each, and fails the CI step if any of them
# crashes, stops on an error, prints ERROR or is still running after
# TIME_LIMIT seconds.
#
# The examples run from a temporary copy of Examples/Original_Examples, not
# from the checkout: several of them write files into their own folder
# (databases, binary.dat, foo.txt), and the packaging steps that follow copy
# Examples/ straight into the release, so running them in place would ship
# whatever they leave behind.
#
# macOS has no `timeout` command, so the time limit is a background watchdog.
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

BASIC256_BIN_ARG="${1:?Usage: run_examples.sh <path-to-basic256-binary>}"
TIME_LIMIT=60
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LIST_FILE="${REPO_DIR}/TestSuite/testsuite_examples_ci.txt"

# Resolve to an absolute path before changing directory for each example.
if command -v realpath >/dev/null 2>&1; then
    BASIC256_BIN="$(realpath "${BASIC256_BIN_ARG}")"
else
    BASIC256_BIN="$(cd "$(dirname "${BASIC256_BIN_ARG}")" && pwd)/$(basename "${BASIC256_BIN_ARG}")"
fi

# Same environment as run_testsuite.sh, for the same reasons: no display on
# the Linux runners, and Qt is not staged next to the binary until packaging.
if [ "$(uname -s)" = "Linux" ]; then
    export QT_QPA_PLATFORM=offscreen
fi
if [ -n "${QT_DIR:-}" ]; then
    export LD_LIBRARY_PATH="${QT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
    export QT_PLUGIN_PATH="${QT_DIR}/plugins"
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
cp -R "${REPO_DIR}/Examples/Original_Examples" "${WORK_DIR}/examples"
OUT_FILE="${WORK_DIR}/out.txt"
ERR_FILE="${WORK_DIR}/err.txt"

PASSED=0
FAILED=0
FAILED_NAMES=""

echo "==> Running the original examples listed in TestSuite/testsuite_examples_ci.txt"
while IFS= read -r line || [ -n "${line}" ]; do
    example="$(echo "${line}" | tr -d '\r' | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')"
    case "${example}" in ''|'#'*) continue ;; esac

    example_dir="${WORK_DIR}/examples/$(dirname "${example}")"
    example_file="$(basename "${example}")"
    if [ ! -f "${example_dir}/${example_file}" ]; then
        echo "FAIL  ${example}  (listed but not found)"
        FAILED=$((FAILED + 1)); FAILED_NAMES="${FAILED_NAMES} ${example}"
        continue
    fi

    start=$(date +%s)
    ( cd "${example_dir}" && exec "${BASIC256_BIN}" -s "${example_file}" ) >"${OUT_FILE}" 2>"${ERR_FILE}" </dev/null &
    pid=$!
    ( sleep "${TIME_LIMIT}" && kill -9 "${pid}" ) >/dev/null 2>&1 &
    watchdog=$!
    # 2>/dev/null drops bash's own "Killed" job notice; the FAIL line says it.
    wait "${pid}" 2>/dev/null
    code=$?
    kill "${watchdog}" >/dev/null 2>&1
    wait "${watchdog}" >/dev/null 2>&1
    seconds=$(( $(date +%s) - start ))

    reason=""
    if [ "${seconds}" -ge "${TIME_LIMIT}" ]; then
        reason="still running after ${TIME_LIMIT}s"
    elif [ "${code}" -ne 0 ]; then
        reason="exit code ${code}"
    elif grep -q "ERROR" "${ERR_FILE}"; then
        reason="printed an ERROR"
    fi

    if [ -z "${reason}" ]; then
        echo "pass  ${example}  (${seconds}s)"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL  ${example}  (${reason})"
        echo "----- stdout (last 20 lines)"
        tail -n 20 "${OUT_FILE}"
        echo "----- stderr"
        cat "${ERR_FILE}"
        echo "-----"
        FAILED=$((FAILED + 1)); FAILED_NAMES="${FAILED_NAMES} ${example}"
    fi
done < "${LIST_FILE}"

echo "==> ${PASSED} examples passed, ${FAILED} failed"
if [ "${FAILED}" -ne 0 ]; then
    echo "==> FAIL:${FAILED_NAMES}"
    exit 1
fi
if [ "${PASSED}" -eq 0 ]; then
    echo "==> FAIL: no examples were run - is ${LIST_FILE} empty?"
    exit 1
fi
echo "==> Original examples passed."
