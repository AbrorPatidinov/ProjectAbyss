#!/usr/bin/env bash

# Compile AbyssLang
echo "Compiling..."
./abyssc compute.al compute.aby

echo
echo "========================================"
echo "    PURE COMPUTE BATTLE (100M OPS)"
echo "========================================"

# Run AbyssLang
./abyss_vm compute.aby

echo
echo "----------------------------------------"
echo

# Run Python
python3 compute.py
