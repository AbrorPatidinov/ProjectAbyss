---

## 🌌 Overview

AbyssLang is a modern, statically-typed systems programming language designed for raw speed, explicit memory control, and developer visibility. 

Built entirely from scratch in pure C11, AbyssLang features a dual-engine architecture:
1. **Native AOT Mode:** Compiles directly to highly optimized raw machine code, running at the speed of silicon.
2. **Resonance VM Mode:** A custom stack-based virtual machine featuring the **Abyss Eye**, a revolutionary built-in memory profiler.

AbyssLang is 100% independent. The compiler is pure C, and the Standard Library is written entirely in AbyssLang itself.

---

## 🔥 Key Features

* ⚡ **Blazing Fast (Native AOT):** In heavy 100-million iteration floating-point physics simulations, AbyssLang Native is **over 6x faster than Python**.
* 👁️ **The Abyss Eye HUD:** Call `abyss_eye();` anywhere in your code to summon a futuristic terminal HUD. It visualizes active stack/heap memory, tracks allocation lifecycles (leak analysis), displays live hex dumps, and even shows custom developer commit messages attached to memory pointers!
* 🧠 **Explicit Memory Control:** Manual heap allocation (`new`) and auto-freeing stack allocation (`stack`), with zero Garbage Collection pauses.
* 🛠️ **Modern Systems Syntax:** Full support for `struct`, `enum`, dynamic `interface` dispatch, tuple unpacking, bitwise operations, and `try/catch/throw` error handling.
* 📦 **Self-Hosted Standard Library:** No external dependencies. The `std.math` and `std.io` modules are written in pure AbyssLang.

---

## 📊 Benchmarks

**The 100 Million Iteration Physics Simulation (Float Math & Structs)**

| Language | Execution Time |
| :--- | :--- |
| **AbyssLang (Native AOT)** | **~2.04 seconds** |
| Python 3 | ~12.85 seconds |

*AbyssLang maps virtual stack variables directly into CPU hardware registers via GCC Link Time Optimization (LTO).*

---

## 🧠 Architecture

### The Compiler (`abyssc`)
A lightning-fast, single-pass compiler that translates `.al` source code directly into either `.aby` bytecode or highly optimized Native C code.

### The Virtual Machine (`abyss_vm`)
A deterministic, stack-based VM that tracks the exact Instruction Pointer (IP) and Frame Pointer (FP) of every byte of memory allocated, powering the Abyss Eye HUD.

---

## 🛠 Build Instructions

Requirements:
- GCC or Clang
- GNU Make

Build the toolchain:
```bash
make
```

Outputs:
- `abyssc` (The Compiler)
- `abyss_vm` (The Virtual Machine)

---

## 🚀 Quick Start

Create `game.al`:
```c
struct Weapon {
    int damage;
}

void main() {
    print("Equipping player...");
    
    // Allocate on the stack with a memory comment!
    Weapon sword = stack(Weapon, "Player's main sword");
    sword.damage = 50;

    // Summon the Memory Resonance HUD
    abyss_eye();
}
```

**Run in VM Mode (For Debugging & Abyss Eye):**
```bash
./abyssc game.al game.aby
./abyss_vm game.aby
```

**Compile to Native Binary (For Maximum Speed):**
```bash
./abyssc --native game.al game_native
./game_native
```

---

## 🧩 VSCode Extension

AbyssLang comes with official VSCode support!
1. Go to the `/releases` folder.
2. Install `abysslang-0.2.0.vsix` in Visual Studio Code.
3. Enjoy full syntax highlighting, bracket matching, and code snippets for structs, enums, and interfaces.

---

## 🤝 Contributing

Contributions are welcome! We are currently looking for help expanding the pure-AbyssLang standard library (File I/O, Networking, etc.).

Please read `CONTRIBUTING.md` before submitting pull requests. Major architectural changes should be discussed in an Issue first.

---

## 📜 License

AbyssLang is proudly open-source and licensed under the **Apache License 2.0**.

See the `LICENSE` file for details. You are free to use, modify, and distribute this language in both open-source and commercial projects.
