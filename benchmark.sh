#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
N=200000 # Number of iterations for the loop

# --- Build Step ---
echo "Building AbyssLang compiler and VM..."
gcc -std=c11 -O2 -Wall -o abyssc abyssc.c
gcc -std=c11 -O2 -Wall -o abyss_vm vm.c
echo "Build complete."

# --- Compile AbyssLang Source ---
./abyssc sample.al sample.aby

# --- Generate Equivalent Python Test Script ---
cat > test.py <<PY
import sys
w = sys.stdout.write
N = ${N}
w("Python benchmark start\n")
for i in range(N):
    w("42\n")
w("Python benchmark end\n")
PY

# --- Run Performance Timings ---
echo
echo "--- AbyssLang (VM) Performance ---"
/usr/bin/time -v ./abyss_vm sample.aby > /dev/null

echo
echo "--- Python 3 Performance ---"
/usr/bin/time -v python3 test.py > /dev/null

# --- Advanced Benchmarking (if tools are installed) ---
if command -v hyperfine >/dev/null 2>&1; then
  echo
  echo "--- Hyperfine Comparative Benchmark ---"
  hyperfine --warmup 3 --runs 10 "./abyss_vm sample.aby > /dev/null" "python3 test.py > /dev/null"
fi

if command -v strace >/dev/null 2>&1; then
  echo
  echo "--- Strace Summary (write syscalls) ---"
  echo "[AbyssLang]"
  strace -c -e write ./abyss_vm sample.aby > /dev/null 2>&1 || true
  echo "[Python 3]"
  strace -c -e write python3 test.py > /dev/null 2>&1 || true
fi

if command -v perf >/dev/null 2>&1; then
  echo
  echo "--- Perf Stat Summary ---"
  echo "[AbyssLang]"
  perf stat -e cycles,instructions ./abyss_vm sample.aby > /dev/null 2>&1 || true
  echo "[Python 3]"
  perf stat -e cycles,instructions python3 test.py > /dev/null 2>&1 || true
fi

echo
echo "Benchmark complete."