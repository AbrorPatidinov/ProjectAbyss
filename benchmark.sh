#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
# 10 MILLION ITERATIONS
N=10000000

# --- Build Step ---
echo "Building AbyssLang..."
make

# --- Generate AbyssLang Source ---
echo "Generating sample.al with N=${N}..."
cat > sample.al <<AL
void main() {
    print("AbyssLang benchmark start");

    int i = 0;
    int N = ${N};

    while (i < N) {
      print(42);
      i = i + 1;
    }

    print("AbyssLang benchmark end");
}
AL

# --- Compile AbyssLang Source ---
echo "Compiling sample.al..."
./abyssc sample.al sample.aby

# --- Generate Equivalent Python Test Script ---
echo "Generating Python test script..."
cat > test.py <<PY
import sys
import time

def main():
    w = sys.stdout.write
    N = ${N}
    w("Python benchmark start\n")
    for i in range(N):
        w("42\n")
    w("Python benchmark end\n")

if __name__ == "__main__":
    main()
PY

# --- Run Performance Timings ---
echo
echo "======================================================="
echo "  THE BATTLE OF 10 MILLION ITERATIONS"
echo "======================================================="

echo
echo "--- AbyssLang (VM) Performance ---"
# We use /usr/bin/time -v to get detailed stats including memory
if [ -f /usr/bin/time ]; then
    /usr/bin/time -v ./abyss_vm sample.aby > /dev/null
else
    time ./abyss_vm sample.aby > /dev/null
fi

echo
echo "--- Python 3 Performance ---"
if [ -f /usr/bin/time ]; then
    /usr/bin/time -v python3 test.py > /dev/null
else
    time python3 test.py > /dev/null
fi

echo
echo "Benchmark complete."
