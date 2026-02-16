#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
# 10 MILLION ITERATIONS
N=10000000

# --- Build Step ---
echo "Building AbyssLang compiler and VM (MAX PERFORMANCE)..."
gcc -std=c11 -O3 -march=native -flto -Wall -o abyssc abyssc.c
gcc -std=c11 -O3 -march=native -flto -Wall -o abyss_vm vm.c
echo "Build complete."

# --- Generate AbyssLang Source (Dynamic N) ---
echo "Generating sample.al with N=${N}..."
cat > sample.al <<AL
print_show("AbyssLang benchmark start\n");

i = 0;
N = ${N};

while (i < N) {
  print(42);
  i = i + 1;
}

print_show("AbyssLang benchmark end\n");
AL

# --- Compile AbyssLang Source ---
echo "Compiling sample.al..."
./abyssc sample.al sample.aby

# --- Generate Equivalent Python Test Script ---
echo "Generating Python test script..."
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
echo "======================================================="
echo "  THE BATTLE OF 10 MILLION ITERATIONS (FAIR FIGHT)"
echo "======================================================="

echo
echo "--- AbyssLang (VM) Performance ---"
/usr/bin/time -v ./abyss_vm sample.aby > /dev/null

echo
echo "--- Python 3 Performance ---"
/usr/bin/time -v python3 test.py > /dev/null

# --- Advanced Benchmarking ---
if command -v hyperfine >/dev/null 2>&1; then
  echo
  echo "--- Hyperfine Comparative Benchmark ---"
  # 10M is heavy, so we do fewer runs
  hyperfine --warmup 1 --runs 3 "./abyss_vm sample.aby > /dev/null" "python3 test.py > /dev/null"
fi

echo
echo "Benchmark complete."
