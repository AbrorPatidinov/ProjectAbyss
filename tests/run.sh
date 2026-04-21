#!/bin/bash
# AbyssLang regression test harness
# Runs every tests/*.al file in both VM and native mode, compares stdout
# against tests/expected/<name>.txt, reports PASS/FAIL.
#
# Convention: tests whose name starts with "fail_" are EXPECTED to fail at
# compile time. The expected file holds the fragment of the error message.

set -u

ABYSSC="${ABYSSC:-./abyssc}"
VM="${VM:-./abyss_vm}"
TEST_DIR="${TEST_DIR:-tests}"
EXPECTED_DIR="${EXPECTED_DIR:-$TEST_DIR/expected}"

PASS=0
FAIL=0
FAILED_NAMES=()

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
RESET='\033[0m'

check() {
    local label="$1" expected="$2" actual="$3" name="$4"
    if [ "$actual" = "$expected" ]; then
        printf "  ${GREEN}✓${RESET} %-8s %s\n" "$label" "$name"
        PASS=$((PASS + 1))
    else
        printf "  ${RED}✗${RESET} %-8s %s\n" "$label" "$name"
        printf "     ${YELLOW}expected:${RESET}\n"
        echo "$expected" | head -5 | sed 's/^/       /'
        printf "     ${YELLOW}got:${RESET}\n"
        echo "$actual" | head -5 | sed 's/^/       /'
        FAIL=$((FAIL + 1))
        FAILED_NAMES+=("$label:$name")
    fi
}

check_contains() {
    local label="$1" needle="$2" haystack="$3" name="$4"
    if echo "$haystack" | grep -qF -- "$needle"; then
        printf "  ${GREEN}✓${RESET} %-8s %s\n" "$label" "$name"
        PASS=$((PASS + 1))
    else
        printf "  ${RED}✗${RESET} %-8s %s\n" "$label" "$name"
        printf "     ${YELLOW}expected to contain:${RESET} %s\n" "$needle"
        printf "     ${YELLOW}got:${RESET}\n"
        echo "$haystack" | head -5 | sed 's/^/       /'
        FAIL=$((FAIL + 1))
        FAILED_NAMES+=("$label:$name")
    fi
}

printf "${CYAN}AbyssLang regression suite${RESET}\n"
printf "  compiler: %s\n" "$ABYSSC"
printf "  vm:       %s\n" "$VM"
printf "\n"

shopt -s nullglob
for test in "$TEST_DIR"/*.al; do
    name=$(basename "$test" .al)
    expected_file="$EXPECTED_DIR/${name}.txt"

    # Negative tests: name starts with "fail_" — expect compile failure whose
    # stderr contains the expected file's text.
    if [[ "$name" == fail_* ]]; then
        if [ ! -f "$expected_file" ]; then
            printf "  ${YELLOW}?${RESET} SKIP     %s (no expected file)\n" "$name"
            continue
        fi
        needle=$(cat "$expected_file")
        tmp_out="/tmp/${name}.aby"
        rm -f "$tmp_out"
        err=$("$ABYSSC" "$test" "$tmp_out" 2>&1 >/dev/null)
        rc=$?
        if [ "$rc" -eq 0 ]; then
            printf "  ${RED}✗${RESET} %-8s %s (compiled when failure expected)\n" "FAIL_" "$name"
            FAIL=$((FAIL + 1))
            FAILED_NAMES+=("FAIL_:$name")
        else
            check_contains "FAIL_" "$needle" "$err" "$name"
        fi
        continue
    fi

    # Positive tests: compile, run VM, run native, compare stdout
    if [ ! -f "$expected_file" ]; then
        printf "  ${YELLOW}?${RESET} SKIP     %s (no expected file)\n" "$name"
        continue
    fi
    expected=$(cat "$expected_file")

    tmp_aby="/tmp/${name}.aby"
    tmp_native="/tmp/${name}_native"
    rm -f "$tmp_aby" "$tmp_native"

    if ! "$ABYSSC" "$test" "$tmp_aby" >/dev/null 2>&1; then
        printf "  ${RED}✗${RESET} %-8s %s (compile failed)\n" "COMPILE" "$name"
        "$ABYSSC" "$test" "$tmp_aby" 2>&1 | head -3 | sed 's/^/       /'
        FAIL=$((FAIL + 1))
        FAILED_NAMES+=("COMPILE:$name")
        continue
    fi

    actual_vm=$("$VM" "$tmp_aby" 2>&1)
    check "VM" "$expected" "$actual_vm" "$name"

    if "$ABYSSC" --native "$test" "$tmp_native" >/dev/null 2>&1; then
        actual_native=$("$tmp_native" 2>&1)
        check "NATIVE" "$expected" "$actual_native" "$name"
    else
        printf "  ${RED}✗${RESET} %-8s %s (native compile failed)\n" "NATIVE" "$name"
        FAIL=$((FAIL + 1))
        FAILED_NAMES+=("NATIVE:$name")
    fi
done

printf "\n"
if [ "$FAIL" -eq 0 ]; then
    printf "${GREEN}All %d checks passed.${RESET}\n" "$PASS"
    exit 0
else
    printf "${RED}%d failed, %d passed.${RESET}\n" "$FAIL" "$PASS"
    printf "Failures:\n"
    for n in "${FAILED_NAMES[@]}"; do
        printf "  %s\n" "$n"
    done
    exit 1
fi
