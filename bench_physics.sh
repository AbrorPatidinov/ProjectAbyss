#!/usr/bin/env bash

echo "Compiling AbyssLang Physics..."
./abyssc physics.al physics.aby

echo
echo "========================================"
echo "      PHYSICS ENGINE BENCHMARK"
echo "========================================"

if command -v hyperfine >/dev/null 2>&1; then
    # Hyperfine is the gold standard for benchmarking
    hyperfine --warmup 5 \
        "./abyss_vm physics.aby" \
        "python3 physics.py"
else
    # Fallback to simple time
    echo "Running AbyssLang (100 runs)..."
    time for i in {1..100}; do ./abyss_vm physics.aby > /dev/null; done

    echo "Running Python (100 runs)..."
    time for i in {1..100}; do python3 physics.py > /dev/null; done
fi
