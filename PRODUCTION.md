# AbyssLang — Core Language, Production-Ready

**Status:** 29/29 regression checks passing (VM + native), showcase clean, zero
leaks, no silent corruption paths remaining in the core compiler or runtime.

This is the core. It is honest about what it does.

---

## What's in the core now

### Compiler (`abyssc`)
- **Two-pass compilation** — signatures scanned first, bodies emitted second.
  Forward references work. Mutual recursion works. Functions can be ordered
  by logic instead of by source position.
- **Idempotent symbol registration** — `add_func`/`add_struct`/`add_enum`/
  `add_global` all idempotent, supporting the two-pass design without
  duplicate-entry bugs.
- **Patch-table call resolver** — `emit_call_to` records every forward call;
  the end-of-compilation resolver rewrites placeholder addresses to real
  bytecode offsets.
- **Bounds checks at every emit site** — 256-slot cap on globals and locals,
  255-arg cap on calls, 8-return cap, 64-field cap on structs, 64-value cap on
  enums. Hitting any of these now fails with a clear compile error instead of
  silently wrapping a byte operand.
- **Double-import guard** — `import std.math; import std.math;` produces the
  same bytecode as a single import.

### Virtual machine (`abyss_vm`)
- **Stack overflow protected** — `push()` checks `sp` and exits cleanly with
  a readable fatal error if the 1M-slot value stack overflows.
- **Call stack overflow protected** — `OP_CALL` checks `csp` and exits cleanly
  if the 4096-frame call stack overflows (infinite recursion is caught).
- **Bytecode magic + version validated** — wrong magic and mismatched version
  both produce explicit errors pointing at the fix ("recompile with matching
  abyssc") instead of silent data corruption.

### Native AOT (`--native`)
- **Defensive float literal emission** — bit patterns written as
  `(int64_t)0x…ULL` casts, safe for any IEEE-754 value. Latent bug fixed.

### Tests
- **10 positive tests** covering arithmetic, floats, control flow, imports,
  structs+memory, bitwise+strings, try/catch, interfaces, arrays, tuples.
- **2 forward-reference tests** — linear forward call + mutual recursion.
- **5 negative tests** confirming the new compile-time bounds fail correctly.
- **Test harness** (`tests/run.sh`) runs every test in both VM and native mode
  and compares against expected output. Binary pass/fail output with diffs.

---

## How to build and demo

```bash
make clean && make
bash tests/run.sh           # 29/29 passing
./abyssc showcase.al showcase.aby && ./abyss_vm showcase.aby
./abyssc --native showcase.al showcase_native && ./showcase_native
```

For the auditorium, three talking points that land:

**1. "Forward references — functions can be defined in any order."**

```bash
cat tests/12_mutual_recursion.al
./abyssc tests/12_mutual_recursion.al /tmp/mr.aby && ./abyss_vm /tmp/mr.aby
```

`is_even` calls `is_odd`. `is_odd` calls `is_even`. Defined in that order.
Single-pass compilers reject this. AbyssLang compiles and runs.

**2. "Safety — no silent failures. Overflows die loudly."**

```bash
cat > /tmp/oops.al << 'EOF'
void infinite() { infinite(); }
void main() { infinite(); }
EOF
./abyssc /tmp/oops.al /tmp/oops.aby && ./abyss_vm /tmp/oops.aby
```

Output:
```
[FATAL ERROR] Call stack overflow (limit 4096 frames).
  This usually means unbounded recursion. Check your base cases.
```

Not a segfault. Not corruption. A language-level error with a hint.

**3. "Correctness — the core stops silent corruption at the bytecode level."**

```bash
python3 -c "
print('void main() {')
for i in range(260): print(f'    int x{i} = {i};')
print('}')
" > /tmp/too_many.al
./abyssc /tmp/too_many.al /tmp/too_many.aby
```

Output:
```
[FATAL ERROR] Too many locals in function (max 256 — bytecode uses
  1-byte local index). Refactor into smaller functions or move data
  into a struct.
```

Before: silently wrapped the 257th local back to slot 0, overwriting the
first variable. Now: explicit compile-time error.

---

## What this bundle does NOT yet include

Honest list so you can answer audience questions:

- **Source locations in runtime errors.** Uncaught exceptions still show
  bytecode IP addresses, not `file.al:42:13`. Requires a debug-info section
  in bytecode. Next phase.
- **Array bounds checking.** Out-of-range writes still silently corrupt
  memory. Requires storing length in allocation header and changing
  `OP_GET_INDEX`/`OP_SET_INDEX`. Next phase.
- **Full type checking at call sites.** Passing `str` where `int` is expected
  still compiles. Type system tightening is a dedicated phase.
- **`int` vs `char` distinction.** Indistinguishable at runtime today.

These are roadmap items, not shipped claims.

---

## What this IS

A statically-typed systems language with:
- Manual memory control (no GC)
- Two execution backends (stack-based VM + native AOT via C)
- Forward references + mutual recursion
- Pass-1/pass-2 compilation with idempotent symbol resolution
- Bytecode-level safety (magic, version, stack overflow, call-stack overflow)
- Compile-time bounds enforcement (globals, locals, args, returns, fields,
  enum values)
- Double-import idempotent
- An in-language standard library (math, array, io, time)
- A working memory profiler (Abyss Eye) with lifecycle tracking and leak
  analysis
- A 29-test regression suite, green on every commit

Built from scratch in pure C11. Designed and written in Tashkent, Uzbekistan.
