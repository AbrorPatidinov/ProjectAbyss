# AbyssLang

> **The first systems programming language designed and built entirely in Uzbekistan.**
> Al-Khwarizmi, born in Khiva, gave the world the algorithm.
> From the same soil, the next generation is building the machines that run them.

A statically-typed systems language with manual memory control, native compilation, and a built-in memory profiler. **~5,000 lines of pure C11. Zero external dependencies.**

- **License:** Apache 2.0
- **Author:** Abrorbek Patidinov — Amity University Tashkent
- **Current version:** Bytecode v13 · 29/29 regression tests green (VM + Native)

---

## 🌌 Overview

AbyssLang is built on a dual-engine architecture:

1. **Native AOT Mode** — Transpiles to C, compiles with GCC `-O3 -flto -march=native`, produces a standalone ELF binary. Runs at the speed of silicon.
2. **Resonance VM Mode** — Stack-based virtual machine with computed-goto dispatch, featuring the **Abyss Eye** — a revolutionary built-in memory profiler that tags allocations with their source-level variable names at the opcode level, without debug symbols.

AbyssLang is 100% sovereign. The compiler is pure C11 — no Flex, no Bison, no LLVM. The standard library is written entirely in AbyssLang itself.

---

## 🔥 Why AbyssLang

- ⚡ **Blazing fast.** **6.75× faster than Python** on verified real-world workloads (Mandelbrot, packet scanning, prime sieve).
- 👁️ **The Abyss Eye HUD** — call `abyss_eye();` anywhere to summon a live terminal HUD. Visualizes active stack/heap memory, tracks allocation lifecycles (leak analysis), displays hex dumps, and shows the variable name you wrote in source code — propagated through a dedicated `OP_TAG_ALLOC` opcode.
- 🧠 **Explicit memory control.** Manual heap (`new`), auto-freeing stack (`stack`), manual `free()`. Zero garbage collection pauses.
- 🛡️ **Static type checking with Option B semantics.** Widening implicit, narrowing forbidden. Catches bugs at compile time without forcing explicit casts everywhere.
- 🛠️ **Modern syntax.** `struct`, `enum`, dynamic `interface` dispatch, tuple returns (including through interfaces), short-circuit `&&`/`||`, bitwise ops, `try`/`catch`/`throw`, string + number concatenation, forward references with mutual recursion.
- 📦 **Self-hosted standard library.** `std.math`, `std.time`, `std.io`, `std.array` — written in pure AbyssLang. No external code.

---

## 📊 Benchmarks (v13, verified)

Identical algorithms, identical hardware (AMD Ryzen 5 4600H, Arch Linux).

| Benchmark                          | AbyssLang Native | Python 3   | **Speedup** |
|------------------------------------|-----------------:|-----------:|------------:|
| Mandelbrot 800×600 (33.5M iter)    | **0.564 s**      | 4.369 s    | **7.74×**   |
| Packet Signature Scanner (500K)    | **0.046 s**      | 0.369 s    | **8.02×**   |
| Prime Sieve (10 M)                 | **0.310 s**      | 1.476 s    | **4.76×**   |
| **Total showcase runtime**         | **0.92 s**       | **6.21 s** | **6.75×**   |

Reproducible from `showcase_real.al` and `showcase_equivalent.py` in the repo.

---

## 🧠 Architecture

### The Compiler (`abyssc`)

**Two-pass** compiler that reads `.al` source and emits either bytecode (`.aby`) or transpiled C:

- **Pass 1 — Signature scan:** Collects all top-level declarations (functions with their argument types, structs, enums, interfaces, globals). No bytecode emitted.
- **Pass 2 — Bytecode emission:** Re-walks with full symbol knowledge. Forward function references resolved via patch table. Argument types checked at every call site.

Properties:

- **Lexer:** Handwritten. File-context stack for multi-file imports (idempotent). Supports decimal/hex integer literals, float literals, character literals with full escape sequences, and both line (`//`) and block (`/* */`) comments.
- **Parser:** Recursive descent, 12-level precedence climbing. No AST — bytecode emitted directly.
- **Type checker:** Integrated. Every assignment, return, and call-site argument runs through `check_assign_compat()` with Option B semantics.

### The Virtual Machine (`abyss_vm`)

Stack-based with computed-goto dispatch:

- **Stack:** 1 MiB value stack (1,048,576 × 64-bit slots)
- **Call stack:** 4,096 frames
- **Exception stack:** 256 frames
- **Opcodes:** 60+ instructions

Stack overflow, call-stack overflow, and bytecode version mismatches are all caught cleanly — never segfault.

### Native AOT Compiler

Bytecode-to-C transpiler producing standalone native binaries:

- Each opcode → equivalent C statement
- GCC `-O3 -flto=auto -march=native`
- Value union `{ int64_t i; double f; void *p; }` — eliminates memcpy overhead
- Computed-goto jump table preserved
- Output: standalone ELF, no runtime dependencies

---

## 🛠 Build Instructions

Requirements:

- GCC or Clang
- GNU Make

Build the toolchain:

```bash
make clean && make
```

Outputs:

- `abyssc` — the compiler
- `abyss_vm` — the virtual machine

Verify install:

```bash
bash tests/run.sh
# Expected: All 29 checks passed.
```

---

## 🚀 Quick Start

Create `game.al`:

```
struct Weapon {
    int damage;
    str name;
}

void main() {
    print("Equipping player...");

    // Allocate on the stack with an Abyss Eye memory comment
    Weapon sword = stack(Weapon, "Player's main sword");
    sword.damage = 50;
    sword.name = "Excalibur";

    // String + int concatenation (v13+)
    str report = "Damage: " + sword.damage;
    print(report);

    // Summon the Memory Resonance HUD
    abyss_eye();
}
```

**VM mode (for debugging + Abyss Eye):**

```bash
./abyssc game.al game.aby
./abyss_vm game.aby
```

**Native mode (for maximum speed):**

```bash
./abyssc --native game.al game_native
./game_native
```

**Native mode with profiler:**

```bash
./abyssc --native --eye game.al game_debug
./game_debug
```

---

## 📖 Language Features (v13)

### Core

- Statically typed: `int`, `float`, `char`, `str`, `void`
- Numeric literals: decimal `42`, hex `0xFF`, float `3.14`, char `'A'` with escape sequences
- `null` keyword for pointer-typed variables
- Block and line comments: `/* ... */` and `//`

### Operators

- Arithmetic with int↔float auto-promotion
- Short-circuit `&&` and `||`
- Full bitwise suite: `& | ^ ~ << >>`
- All compound assignments: `+= -= *= /= %= <<= >>= &= |= ^=` (in statements and for-loop step)
- Ternary `? :`

### Functions

- Three declaration forms: `void name()`, `int name()`, `function name() : (int ret)`
- Multiple return values (tuples), up to 8
- **Forward references** with mutual recursion
- **Argument type checking** at every call site
- Namespaced functions: `std.math.pow(2, 10)`

### Memory

- Heap: `new(Type, "comment")`, arrays: `new(Type, N)`
- Stack: `stack(Type)` — auto-freed on return
- Manual: `free(ptr)`
- **Abyss Eye profiler:** `abyss_eye();` — no debug symbols needed

### Types

- **Structs** with nested fields and compound field assignment
- **Enums** with auto-increment and explicit values
- **Interfaces** with dynamic dispatch, including tuple returns (v13+)
- Arrays with heap allocation

### Error Handling

- `try` / `catch` / `throw` with string error values
- Nested try/catch with re-throwing

### String Features (v13+)

- Dynamic concatenation: `"hello " + name` (heap-allocated)
- String + number: `"x=" + 42` → `"x=42"`
- Float conversion: `"pi=" + 3.14` → `"pi=3.140000"`

### Modules

- `import std.math;` — relative path loading
- Idempotent (importing twice is free)
- Self-hosted stdlib: `std.math`, `std.time`, `std.io`, `std.array`

For the complete language reference, see [`LANGUAGE_GUIDE.md`](LANGUAGE_GUIDE.md).

---

## 🧩 VSCode Extension

AbyssLang ships with official VSCode support:

1. Go to the `/releases` folder.
2. Install `abysslang-0.2.0.vsix` in Visual Studio Code.
3. Enjoy syntax highlighting, bracket matching, and code snippets for structs, enums, and interfaces.

---

## 🧪 Regression Tests

29 integration tests cover the full language surface — arithmetic, floats, control flow, imports, structs, bitwise, try/catch, interfaces, arrays, tuples, forward references, mutual recursion, and every compile-time limit. Each test runs against both the VM and the native backend.

```bash
bash tests/run.sh
```

---

## 🤝 Contributing

Contributions are welcome. Current focus areas:

- `std.str` — string parsing, splitting, to_int, trunc
- `std.fs` — file I/O beyond stdin
- `std.net` — socket bindings
- `std.json`

Please read `CONTRIBUTING.md` before submitting pull requests. Major architectural changes should be discussed in an Issue first.

---

## 🗺️ Roadmap

Planned for v14 and beyond:

- Explicit cast syntax: `(int)f`, `(char)(n % 256)`
- Array literals: `int[] a = {1, 2, 3};`
- Named struct literals: `Vec{x=1, y=2}`
- Array element-type tracking through indexing
- `std.math.fmod()` for float modulo
- Optional `--safe` compile flag (bounds checks, null checks, catchable div-by-zero)
- Source line/column mapping in runtime errors
- Stack traces on uncaught `throw`
- REPL
- Debugger (step, breakpoints, variable inspection)
- Package manager / module registry

---

## 📜 License

AbyssLang is open-source and licensed under the **Apache License 2.0**.

See the `LICENSE` file. Free to use, modify, and distribute in both open-source and commercial projects.

---

*Built from scratch. Zero dependencies. Total control.*
*Designed and written in Tashkent, Uzbekistan.*
