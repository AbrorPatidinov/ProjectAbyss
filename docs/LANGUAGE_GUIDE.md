# AbyssLang — Complete Language Documentation

> **The first programming language designed and built entirely in Uzbekistan.**
> A statically-typed systems language with manual memory control, native compilation, and a built-in memory profiler.
> Written in ~5,000 lines of pure C11. Zero external dependencies.

**Author:** Abrorbek Patidinov
**License:** Apache 2.0
**Repository:** [github.com/AbrorPatidinov/ProjectAbyss](https://github.com/AbrorPatidinov/ProjectAbyss)
**This document describes:** Bytecode version 12 · 29/29 regression tests green (VM + Native)

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Types & Variables](#2-types--variables)
3. [Operators](#3-operators)
4. [Control Flow](#4-control-flow)
5. [Functions](#5-functions)
6. [Structs](#6-structs)
7. [Memory Management](#7-memory-management)
8. [Enums](#8-enums)
9. [Interfaces](#9-interfaces)
10. [Arrays](#10-arrays)
11. [Strings](#11-strings)
12. [Error Handling](#12-error-handling)
13. [Modules & Imports](#13-modules--imports)
14. [The Abyss Eye](#14-the-abyss-eye)
15. [Formatted Print](#15-formatted-print)
16. [Native Bridge Functions](#16-native-bridge-functions)
17. [Compilation Modes](#17-compilation-modes)
18. [Standard Library Reference](#18-standard-library-reference)
19. [Compile-Time Limits](#19-compile-time-limits)
20. [Runtime Safety Guarantees](#20-runtime-safety-guarantees)
21. [Gotchas & Things That May Surprise You](#21-gotchas--things-that-may-surprise-you)
22. [Grammar Summary](#22-grammar-summary)
23. [Architecture Overview](#23-architecture-overview)
24. [Roadmap](#24-roadmap)

---

## 1. Quick Start

### Build the Toolchain

```bash
make clean && make
```

This produces two binaries:

- `abyssc` — The AbyssLang compiler
- `abyss_vm` — The Resonance Virtual Machine

### Hello World

```
void main() {
    print("Hello, Abyss!");
}
```

### Compile & Run

```bash
# VM mode — bytecode execution with Abyss Eye profiler
./abyssc hello.al hello.aby
./abyss_vm hello.aby

# Native mode — maximum performance (compiles via GCC -O3 -flto)
./abyssc --native hello.al hello
./hello
```

### Verify Your Install

```bash
bash tests/run.sh
# Expected: "All 29 checks passed."
```

---

## 2. Types & Variables

AbyssLang is statically typed. Every variable must be declared with its type.

### Primitive Types

| Type    | Storage          | Description                                          | Example               |
|---------|------------------|------------------------------------------------------|-----------------------|
| `int`   | 64-bit signed    | Signed integer                                       | `int age = 25;`       |
| `float` | 64-bit IEEE-754  | Double-precision floating point                      | `float pi = 3.14159;` |
| `char`  | 64-bit slot      | Integer treated as a character value (see gotcha §21)| `char grade = 65;`    |
| `str`   | pointer          | Null-terminated string (pointer to byte data)        | `str name = "Abyss";` |
| `void`  | —                | No value (function return type only)                 | `void main() { }`     |

> **Note on `char`:** Internally every value on the AbyssLang stack is a 64-bit slot. `char` is a type label that tells the compiler you intend to use the low byte as an ASCII character. There is no truncation on assignment — `char c = 300;` stores 300; only the print path interprets the low byte. Character literals like `'A'` are **not supported**; use the numeric value (`char grade = 65;`) or write a UTF-8 string (`str grade = "A";`) instead.

### Variable Declaration

```
// With initializer (recommended)
int score = 100;
float velocity = 9.81;
str greeting = "Hello";

// Without initializer — ZERO-INITIALIZED
int counter;     // counter == 0
float f;         // f     == 0.0
str s;           // s     == NULL pointer (DO NOT use without assigning first)
```

> **Warning:** Declaring a `str` or struct-pointer variable without initializing it produces a **null pointer**. Reading fields from or passing it to functions that dereference it will crash the VM. Always assign before use.

### No Implicit Type Promotion

**AbyssLang does NOT auto-promote `int` to `float` in mixed-type arithmetic.** If you add an `int` to a `float`, the compiler treats both operands as floats at the bit level — which silently corrupts results.

```
int a = 5;
float b = 2.5;
float c = a + b;   // WRONG: produces garbage (≈ 2.5, not 7.5)
```

To mix types safely, **keep all operands of the same type**:

```
float a = 5.0;     // declare as float from the start
float b = 2.5;
float c = a + b;   // c == 7.5  ✓
```

This is a known limitation; a proper type-coercion layer is a roadmap item.

---

## 3. Operators

### Arithmetic

| Operator | Description                              | Example         |
|----------|------------------------------------------|-----------------|
| `+`      | Addition; also string concatenation      | `a + b`         |
| `-`      | Subtraction / unary negation             | `a - b` or `-x` |
| `*`      | Multiplication                           | `a * b`         |
| `/`      | Division (integer or float by type)      | `a / b`         |
| `%`      | Modulo (**integer only** — don't use on floats; produces undefined results) | `a % b` |

### Comparison

| Operator | Description           |
|----------|-----------------------|
| `==`     | Equal to              |
| `!=`     | Not equal to          |
| `<`      | Less than             |
| `>`      | Greater than          |
| `<=`     | Less than or equal    |
| `>=`     | Greater than or equal |

All comparisons return `int` (0 for false, 1 for true). There is no distinct `bool` type.

### Logical

| Operator | Description |
|----------|-------------|
| `&&`     | Logical AND |
| `\|\|`   | Logical OR  |
| `!`      | Logical NOT |

Operands are treated as truthy if non-zero. Result is `int` (0 or 1).

> **Note:** `&&` and `\|\|` are **not short-circuiting** in the current implementation. Both sides are always evaluated. Write guards as `if (ptr != 0) { ... ptr.field ... }` rather than relying on `if (ptr != 0 && ptr.field > 0)`.

### Bitwise

| Operator | Description   | Example        |
|----------|---------------|----------------|
| `&`      | Bitwise AND   | `flags & mask` |
| `\|`     | Bitwise OR    | `flags \| bit` |
| `^`      | Bitwise XOR   | `a ^ b`        |
| `~`      | Bitwise NOT   | `~flags`       |
| `<<`     | Left shift    | `val << 2`     |
| `>>`     | Right shift   | `val >> 4`     |

### Assignment

| Operator | Description          | Equivalent     |
|----------|----------------------|----------------|
| `=`      | Assign               | `x = 5`        |
| `+=`     | Add and assign       | `x = x + 5`    |
| `-=`     | Subtract and assign  | `x = x - 5`    |
| `++`     | Increment by 1       | `x = x + 1`    |
| `--`     | Decrement by 1       | `x = x - 1`    |

> `*=` and `/=` are not currently supported; use the long form.

### Ternary

```
int max = (a > b) ? a : b;
```

Both branches must have matching types.

### Operator Precedence (highest to lowest)

| Level | Operators               |
|-------|-------------------------|
| 1     | Unary: `-` `!` `~`      |
| 2     | `*` `/` `%`             |
| 3     | `+` `-`                 |
| 4     | `<<` `>>`               |
| 5     | `<` `<=` `>` `>=`       |
| 6     | `==` `!=`               |
| 7     | `&`                     |
| 8     | `^`                     |
| 9     | `\|`                    |
| 10    | `&&`                    |
| 11    | `\|\|`                  |
| 12    | `? :` (ternary)         |

---

## 4. Control Flow

### if / else

```
if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else {
    print("F");
}
```

### while

```
int i = 0;
while (i < 100) {
    print(i);
    i++;
}
```

### for

```
for (int i = 0; i < 100; i++) {
    print(i);
}
```

> **For-loop step restriction.** The step expression accepts only `var++`, `var--`, and `var = expression`. Compound assignment in the step (`i += 2`, `i -= N`) **is not supported** and will fail with a parse error. Workaround: use an inner `while` or move the step inside the body:
>
> ```
> // NOT ALLOWED:
> // for (int i = 0; i < 100; i += 2) { ... }
>
> // WORKS:
> int i = 0;
> while (i < 100) {
>     // body
>     i = i + 2;
> }
>
> // ALSO WORKS (using the `= expr` form):
> for (int i = 0; i < 100; i = i + 2) { /* body */ }
> ```

### break & continue

```
for (int i = 0; i < 100; i++) {
    if (i % 2 != 0) { continue; }  // Skip odd numbers
    if (i > 50)     { break;    }  // Stop at 50
    print(i);
}
```

### Block Scoping

Variables declared inside `{ }` blocks are scoped to that block:

```
{
    int temp = 42;
    print(temp);   // works
}
// temp is no longer accessible here — using it is a compile error
```

For-loop iteration variables are also block-scoped.

---

## 5. Functions

### Basic Declaration

Three equivalent syntaxes:

```
// Syntax 1: void functions
void greet() {
    print("Hello!");
}

// Syntax 2: return type before the name
int double_it(int x) {
    return x * 2;
}

// Syntax 3: `function` keyword with named returns
function square(int x) : (int result) {
    return x * x;
}
```

### Forward References

**Functions may be called before they are defined.** AbyssLang uses two-pass compilation — signatures are collected first, bodies are compiled second — so ordering doesn't matter:

```
void main() {
    print(is_even(10));   // defined below; still works
}

int is_even(int n) {
    if (n == 0) { return 1; }
    return is_odd(n - 1);
}

int is_odd(int n) {      // mutually recursive with is_even
    if (n == 0) { return 0; }
    return is_even(n - 1);
}
```

### Multiple Return Values (Tuples)

Functions can return up to **8** values. Callers unpack them with comma-separated assignment:

```
function divide(int a, int b) : (int quot, int rem) {
    quot = a / b;
    rem = a % b;
    return quot, rem;
}

void main() {
    int q;
    int r;
    q, r = divide(17, 5);
    print("17 / 5 = %int remainder %int", q, r);
    // Output: 17 / 5 = 3 remainder 2
}
```

> **Limitation:** Tuple unpacking only works with **direct function calls**, not through interface dispatch. `a, b = obj.method();` is not supported; use single-return methods through interfaces.

### Namespaced Functions

Functions can be defined with dot-separated names for module organization:

```
function std.math.pow(int base, int exp) : (int result) {
    // ...
}
```

Callers invoke them with the full namespace path: `std.math.pow(2, 10)`.

---

## 6. Structs

### Definition

```
struct Vector {
    float x;
    float y;
}

struct Player {
    str name;
    int health;
    Vector position;   // nested struct
}
```

### Field Access

```
Player p = new(Player, "Main Character");
p.name = "Artorias";
p.health = 100;

// Nested field access
p.position = new(Vector, "Player Pos");
p.position.x = 10.5;
p.position.y = 20.0;

// Compound assignment on fields
p.health -= 25;
p.position.x += 1.0;

// Increment / decrement on fields
p.health++;
p.health--;
```

> **Note on memory layout:** Every struct field occupies one 64-bit slot regardless of declared type. A `char` field uses 8 bytes, not 1. This is a deliberate design choice (uniform slot size simplifies the bytecode) but means AbyssLang is not suitable for writing packed binary protocol structs directly. Use arrays of bytes (`int[]`) for packing work.

---

## 7. Memory Management

AbyssLang gives you explicit control over memory. **There is no garbage collector.**

### Heap Allocation — `new()`

Allocates memory on the heap. Lives until explicitly freed with `free()`.

```
// Struct allocation with optional Abyss Eye comment
Player p = new(Player, "Main Character");

// Struct allocation without a comment
Player q = new(Player);
```

### Array Allocation — `new(Type, size)`

```
int[] scores = new(int, 100, "Player Scores");
int[] data   = new(int, 256);                     // no comment
```

> **Array element size is fixed at 8 bytes.** `new(char, 256)` allocates 2048 bytes, not 256. All array element access goes through 64-bit slots. This matches the struct-field slot model.

### Stack Allocation — `stack()`

Allocates memory that is automatically freed when the enclosing function returns. Faster than heap allocation for temporary data.

```
function calculate() : (float result) {
    Vector temp = stack(Vector, "Temp Calc");
    temp.x = 5.0;
    temp.y = 10.0;
    return temp.x + temp.y;
    // No free() needed — temp vanishes when calculate() returns
}
```

> Stack allocations are tracked by the Abyss Eye and appear in the HUD with a lightning icon (⚡) instead of the heap diamond (💎).

### Manual Free — `free()`

```
Player p = new(Player, "Hero");
// ... use p ...
free(p);     // memory returned to the system; p is now a dangling pointer
```

> **Double-free is undefined behavior.** `free(p); free(p);` may crash or silently corrupt the allocator state. Assign `p = 0;` after freeing if you want to re-check before another free — but currently there is no null-check operator that makes this safe either.

### Summary Table

| Method         | Lifetime                  | Speed   | Use Case                |
|----------------|---------------------------|---------|-------------------------|
| `new(Type)`    | Until `free()` is called  | Normal  | Long-lived objects      |
| `new(Type, N)` | Until `free()` is called  | Normal  | Dynamic arrays          |
| `stack(Type)`  | Until function returns    | Fastest | Temporary / local data  |

---

## 8. Enums

Enums define named integer constants. Values auto-increment from the last explicit value.

```
enum State {
    IDLE,             // 0
    RUNNING = 5,      // 5
    STOPPED,          // 6 (auto-increments from 5)
    ERROR   = -1      // -1
}

void main() {
    int current = State.RUNNING;
    if (current == State.RUNNING) {
        print("Active.");
    }
}
```

Enums can be used as variable types. Internally they are `int`:

```
State s = State.IDLE;
```

**Limit:** Max 64 values per enum. Exceeding this is a compile-time error.

---

## 9. Interfaces

Interfaces define a contract — a set of method signatures. Any function matching the signature can be assigned to an interface field at runtime, enabling dynamic dispatch.

```
interface MathOp {
    function execute(int a, int b) : (int res);
}

function add(int a, int b) : (int res) { return a + b; }
function mul(int a, int b) : (int res) { return a * b; }

void main() {
    MathOp op = stack(MathOp);

    op.execute = add;
    print("3 + 5 = %int", op.execute(3, 5));   // 8

    op.execute = mul;
    print("3 * 5 = %int", op.execute(3, 5));   // 15
}
```

Under the hood, interface fields store function bytecode addresses; dispatch uses the `OP_CALL_DYN_BOT` opcode. There is no vtable and no runtime type identity — the interface's only guarantee is signature compatibility.

> **Limitations:** Interface methods currently support single return values only. Tuple unpacking through `obj.method()` is a roadmap item.

---

## 10. Arrays

Arrays are heap-allocated with a fixed size known at allocation time.

```
// Allocate
int[] scores = new(int, 10, "Scores");

// Write
scores[0] = 100;
scores[1] = 200;

// Read
print(scores[0]);    // 100

// Increment / decrement elements in place
scores[0]++;
scores[1]--;

// Free when done
free(scores);
```

### Struct Arrays

```
struct Enemy {
    int hp;
    int damage;
}

Enemy[] enemies = new(Enemy, 5, "Enemy Array");
for (int i = 0; i < 5; i++) {
    Enemy e = new(Enemy, "Enemy Instance");
    e.hp = 100;
    e.damage = 10 + i;
    enemies[i] = e;
}
```

> **⚠ No bounds checking.** Out-of-range writes silently corrupt adjacent memory; out-of-range reads return whatever byte is at the address. This is a deliberate systems-language choice, but you should treat array length as **your** responsibility. Track lengths explicitly in a companion variable or struct field. A `--safe` compilation mode with runtime bounds checks is a planned roadmap item.

---

## 11. Strings

### String Literals

```
str greeting = "Hello, World!";
```

Strings are pointers to null-terminated byte data. They are **not** length-prefixed.

### Dynamic Concatenation

The `+` operator on two `str` values allocates new heap memory:

```
str first = "Hello, ";
str second = "World!";
str message = first + second;     // "Hello, World!" — heap allocated
// ...when done with `message`, free it:
free(message);
```

Concatenated strings are tracked by the Abyss Eye under the special `DynString` type. This means heavy string concatenation in a loop will accumulate heap allocations — if the lifetime is bounded to a function, consider `stack()`-ed buffers instead.

> **`+` between `str` and `int`/`float` is not supported** and may silently produce broken output. Convert numbers to strings via `print("%int", n)` (emits to stdout), or use formatted print patterns. A `to_str()` function is a roadmap item.

### String Format Printing

See [§15 Formatted Print](#15-formatted-print).

---

## 12. Error Handling

AbyssLang uses `try` / `catch` / `throw` for structured error recovery. Errors are **string values only** — there is no exception object type.

```
function safe_divide(int a, int b) : (int res) {
    if (b == 0) {
        throw "Division by zero!";
    }
    return a / b;
}

void main() {
    try {
        int x = safe_divide(10, 0);
        print(x);                 // never reached
    } catch (err) {
        print("Caught: %str", err);
    }
}
```

### Nested Try/Catch

```
try {
    try {
        throw "Inner error";
    } catch (e1) {
        print("Inner caught: %str", e1);
        throw "Re-thrown!";
    }
} catch (e2) {
    print("Outer caught: %str", e2);
}
```

### What `try`/`catch` Does NOT Catch

This is important. `throw` is a **cooperative** mechanism — it only triggers when your code explicitly calls it. The VM does **not** convert hardware signals or runtime errors into catchable exceptions. The following will bypass your `try` block and crash the program:

- **Division by zero** (`10 / 0`) — raises a hardware SIGFPE; process dies immediately.
- **Null pointer dereference** (accessing a field on an unassigned `str` or struct) — segfaults.
- **Array out-of-bounds** — silently corrupts memory (worse than crashing).
- **Stack overflow / call-stack overflow** — caught by the VM, but terminates with a fatal error (not catchable).

To handle these cases, **guard them before they happen**:

```
function safe_divide(int a, int b) : (int res) {
    if (b == 0) { throw "division by zero"; }     // guard FIRST
    return a / b;                                  // then compute
}
```

---

## 13. Modules & Imports

### Importing

```
import std.math;
import std.time;
```

`import std.math;` opens the file `std/math.al` relative to the current directory and includes all its definitions. Nested paths follow the same convention: `import a.b.c;` loads `a/b/c.al`.

### Idempotent Imports

Importing the same module multiple times (from different files, or even the same file) is **free** — the compiler tracks which paths have been loaded and skips duplicates silently:

```
import std.math;
import std.math;         // no-op; already loaded
```

This means you can safely `import std.math;` at the top of every file that uses it, without worrying about duplicate definition errors.

### Calling Imported Functions

```
int result = std.math.pow(2, 10);    // 1024
float now  = std.time.now();
```

### Defining Module Functions

Inside `std/math.al`:

```
function std.math.pow(int base, int exp) : (int result) {
    result = 1;
    for (int i = 0; i < exp; i++) {
        result = result * base;
    }
    return result;
}
```

The function name includes the full namespace path.

---

## 14. The Abyss Eye

The Abyss Eye is AbyssLang's built-in memory profiler. It is not an external tool — it is woven into the instruction set at the opcode level.

### Usage

```
abyss_eye();     // summons the memory HUD at the current point in execution
```

### What It Displays

| Section            | Information                                                                       |
|--------------------|-----------------------------------------------------------------------------------|
| System Resources   | Active stack/heap memory totals, freed bytes, total allocation count              |
| Active Memory      | Every live pointer — address, size, type, variable name, scope, hex dump          |
| Lifecycle Analysis | Full history — allocation IP → free IP; leak warnings for unfreed allocations     |

### Memory Comments

The optional string argument to `new()` and `stack()` appears in the Abyss Eye output:

```
int[] buffer = new(int, 1024, "Network Receive Buffer");
abyss_eye();
// Shows: buffer | Heap | 8192 bytes | Array | "Network Receive Buffer"
```

### How the Variable Name Gets There

The compiler emits a dedicated `OP_TAG_ALLOC` opcode whenever a `new()` or `stack()` result is assigned to a named variable. This opcode propagates the source-level variable name to the runtime allocation tracker, allowing the profiler to display human-readable labels next to raw memory addresses — **without any debug symbols or source maps**.

> Anonymous allocations (e.g. `return new(X);` passed directly without assignment) will appear in the HUD with their tag as `-`. Assign to a variable before returning if you want the name to survive.

### VM Mode vs Native Mode

- **VM mode:** Full Abyss Eye with colored terminal HUD, hex dumps, and lifecycle tracking. Always on.
- **Native mode (default):** Abyss Eye is **disabled** for zero overhead. Stubs compile to empty functions.
- **Native mode with profiler:** pass `--eye` to enable:

  ```bash
  ./abyssc --native --eye program.al output
  ```

  Native profiling uses a 65536-bucket hash table keyed on `(ptr >> 4) & 0xFFFF`, so lookup cost per allocation is O(1) amortized even under heavy workloads.

---

## 15. Formatted Print

The `print()` function accepts a format string followed by arguments. Format specifiers are **type-tagged**, not positional C-style:

| Specifier | Type    | Example                          |
|-----------|---------|----------------------------------|
| `%int`    | Integer | `print("Score: %int", score);`   |
| `%float`  | Float   | `print("PI: %float", pi);`       |
| `%str`    | String  | `print("Name: %str", name);`     |
| `%char`   | Char    | `print("Grade: %char", grade);`  |

### Multiple Arguments

```
print("Player %str scored %int points at (%float, %float)",
      name, score, x, y);
```

### Simple Print (single value, no format string)

```
print(42);          // emits: 42\n
print(3.14);        // emits: 3.140000\n
print("hello");     // emits: hello\n
print(someChar);    // emits: <single character>   ← no trailing newline
```

> **Behavior note:** `print()` on `int`, `float`, or `str` appends a trailing newline. `print()` on a single `char` does **not** append a newline (uses `OP_PRINT_CHAR`, which emits one byte). If you need a newline after a char, add one explicitly.

---

## 16. Native Bridge Functions

Built-in functions that call into the host runtime:

| Function       | Returns | Description                                                 |
|----------------|---------|-------------------------------------------------------------|
| `clock()`      | `float` | High-resolution monotonic clock, seconds since program start |
| `input_int()`  | `int`   | Blocking read of one line from stdin, parsed as integer      |

> `input_int()` reads one line using `fgets` and parses with `atoll`. Non-integer input silently parses as 0 (no error). End-of-file also returns 0. If you need distinguishing behavior, validate upstream.

Internal bridge functions (prefixed `__bridge_*`) exist for the stdlib's own use; they are not part of the user-facing API.

---

## 17. Compilation Modes

### VM Mode — Debugging & Profiling

```bash
./abyssc program.al program.aby
./abyss_vm program.aby
```

- Full Abyss Eye memory profiler always active
- Deterministic stack-based execution via computed-goto dispatch (GCC label-as-value extension)
- Bytecode format: custom binary with `ABYSSBC` magic header and version byte

### Native AOT Mode — Production Performance

```bash
./abyssc --native program.al program_native
./program_native
```

- Transpiles bytecode to C, then invokes `gcc -O3 -flto=auto -march=native`
- Abyss Eye **disabled** for zero overhead
- Produces a standalone ELF binary with no runtime dependencies
- Typical speedup over Python on real workloads: **5× to 10×** (verified: Mandelbrot 7.74×, Packet Scanner 8.02×, Prime Sieve 4.76×)

### Native + Profiler Mode

```bash
./abyssc --native --eye program.al program_debug
./program_debug
```

- Native speed with Abyss Eye active (O(1) hashtable-backed tracker)
- Useful for profiling production-scale workloads

---

## 18. Standard Library Reference

The standard library is written entirely in AbyssLang. Import with `import std.<module>;`.

### std.math

| Function                                                                          | Description                                |
|-----------------------------------------------------------------------------------|--------------------------------------------|
| `std.math.abs(int x) : int`                                                       | Absolute value                             |
| `std.math.pow(int base, int exp) : int`                                           | Integer exponentiation                     |
| `std.math.min(int a, int b) : int`                                                | Minimum of two values                      |
| `std.math.max(int a, int b) : int`                                                | Maximum of two values                      |
| `std.math.clamp(int val, int lo, int hi) : int`                                   | Constrain value to range                   |
| `std.math.gcd(int a, int b) : int`                                                | Greatest common divisor (Euclid)           |
| `std.math.lcm(int a, int b) : int`                                                | Least common multiple                      |
| `std.math.sqrt(int n) : int`                                                      | Integer square root (Newton's method)      |
| `std.math.factorial(int n) : int`                                                 | Factorial                                  |
| `std.math.is_prime(int n) : int`                                                  | Primality test (6k±1 method)               |
| `std.math.fib(int n) : int`                                                       | Nth Fibonacci number                       |
| `std.math.map(int val, int in_lo, int in_hi, int out_lo, int out_hi) : int`       | Range mapping                              |

### std.time

| Function                                 | Description                                |
|------------------------------------------|--------------------------------------------|
| `std.time.now() : float`                 | Current monotonic timestamp (seconds)      |
| `std.time.elapsed(float start) : float`  | Seconds elapsed since `start`              |

### std.io

| Function                                  | Description                                |
|-------------------------------------------|--------------------------------------------|
| `std.io.banner(str text)`                 | Print decorated banner                     |
| `std.io.separator()`                      | Print horizontal line                      |
| `std.io.stat(str label, int value)`       | Print labeled integer                      |
| `std.io.kv(str key, str value)`           | Print key-value pair                       |
| `std.io.prompt_int(str msg) : int`        | Print prompt and read integer              |
| `std.io.newline()`                        | Print blank line                           |

### std.array

| Function                                                | Description                                |
|---------------------------------------------------------|--------------------------------------------|
| `std.array.fill(int[] arr, int len, int val)`           | Fill array with value                      |
| `std.array.sum(int[] arr, int len) : int`               | Sum all elements                           |
| `std.array.min(int[] arr, int len) : int`               | Find minimum value                         |
| `std.array.max(int[] arr, int len) : int`               | Find maximum value                         |
| `std.array.find(int[] arr, int len, int target) : int`  | Linear search (returns index or -1)        |
| `std.array.count(int[] arr, int len, int target) : int` | Count occurrences                          |
| `std.array.reverse(int[] arr, int len)`                 | Reverse array in-place                     |
| `std.array.swap(int[] arr, int i, int j)`               | Swap two elements                          |
| `std.array.sort(int[] arr, int len)`                    | Sort array (insertion sort)                |
| `std.array.is_sorted(int[] arr, int len) : int`         | Check if sorted (1=yes, 0=no)              |
| `std.array.copy(int[] src, int[] dst, int len)`         | Copy array contents                        |

---

## 19. Compile-Time Limits

AbyssLang enforces hard caps on several constructs at the emit site. Exceeding them produces a clear compile-time error with a refactor hint — never silent corruption.

| Construct                           | Limit | Why                                              |
|-------------------------------------|-------|--------------------------------------------------|
| Locals per function                 | 256   | `OP_GET_LOCAL` / `OP_SET_LOCAL` use 1-byte index |
| Globals total                       | 256   | `OP_GET_GLOBAL` / `OP_SET_GLOBAL` use 1-byte index |
| Arguments per function call         | 255   | `OP_CALL` argc field is 1 byte                   |
| Return values per function          | 8     | Fixed-size return buffer in `FuncInfo`           |
| Fields per struct                   | 64    | Fixed-size array in `StructInfo.fields[64]`      |
| Values per enum                     | 64    | Fixed-size array in `EnumInfo.values[64]`        |
| Imported modules total              | 128   | Static loaded-files table                        |

Each limit produces a specific error message. For example:

```
[FATAL ERROR] Too many locals in function (max 256 — bytecode uses 1-byte
local index). Refactor into smaller functions or move data into a struct.
```

If your code hits a limit, these are roadmap candidates for widening the operand size in bytecode version 13.

---

## 20. Runtime Safety Guarantees

The VM and native runtime enforce the following protections:

### Stack Overflow Protection

Every `push()` checks the stack pointer against `STACK_SIZE` (1,048,576 slots). On overflow:

```
[FATAL ERROR] Stack overflow (limit 1048576 slots).
  This usually means deep recursion or an expression that keeps
  pushing values without popping. Check loops and recursion depth.
```

### Call-Stack Overflow Protection

Every `OP_CALL` checks the call depth against `CALL_STACK_SIZE` (4,096 frames). On overflow:

```
[FATAL ERROR] Call stack overflow (limit 4096 frames).
  This usually means unbounded recursion. Check your base cases.
```

Infinite recursion is **caught cleanly** — no segfault.

### Bytecode Validation

The VM checks the bytecode file's magic bytes (`ABYSSBC`) and version on load. Mismatches produce a clear error pointing to the fix:

```
[FATAL ERROR] Bytecode version mismatch.
  File program.aby was compiled with bytecode v11.
  This VM expects v12. Recompile with the matching abyssc.
```

### What Is NOT Protected

- Array bounds (see §10 and §21)
- Null pointer dereferences (see §2, §21)
- Double-free (see §7)
- Division by zero (see §12)
- Integer overflow (wraps silently; this is usually the behavior you want on 64-bit)

These are explicit design choices — AbyssLang is a systems language, and each check has a cost. Several are on the roadmap as opt-in flags (`--safe`, `--null-checks`).

---

## 21. Gotchas & Things That May Surprise You

A short list of behaviors that new users hit in their first hour. All are documented above; this is a summary with pointers.

1. **`'A'` is not a char literal.** Use `65` or the numeric codepoint. (§2)
2. **`int + float` does not auto-promote.** Use same-type operands. (§2)
3. **`//` is the only comment syntax.** No `/* ... */` block comments.
4. **`0xFF` hex literals are not supported.** Use decimal. (`0xFF = 255`)
5. **`for (...; ...; i += 2)` fails.** Use `i = i + 2`. (§4)
6. **`try/catch` does not catch div-by-zero or segfaults.** Guard before the operation. (§12)
7. **Arrays have no bounds checking.** Track length yourself. (§10)
8. **All values are 64-bit slots internally.** `char` and `int` occupy the same space. (§2, §6)
9. **`print()` with `%` requires an exact-spelling type tag** (`%int`, `%str`, etc.). Standard C `%d`, `%s` do not work. (§15)
10. **Uninitialized `str` is a null pointer** and using it will crash. (§2)
11. **Double-free is undefined.** Don't call `free()` twice on the same pointer. (§7)
12. **Logical `&&` and `\|\|` don't short-circuit.** Both sides always evaluate. (§3)
13. **Interface methods can't return tuples yet.** Single return only through dispatch. (§9)
14. **String + number doesn't convert.** Format with `print()`. (§11)

---

## 22. Grammar Summary

| Construct        | Syntax                                                 |
|------------------|--------------------------------------------------------|
| Variable         | `type name = expr;`                                    |
| Array            | `type[] name = new(type, size);`                       |
| Struct def       | `struct Name { type field; ... }`                      |
| Enum def         | `enum Name { A, B = 5, C }`                           |
| Interface def    | `interface Name { function sig; ... }`                 |
| Function (named) | `function name(args) : (ret_type ret_name) { ... }`    |
| Function (void)  | `void name(args) { ... }`                              |
| Function (typed) | `int name(args) { return expr; }`                      |
| Namespaced func  | `function a.b.name(args) : (ret_type ret_name) { ... }`|
| Heap alloc       | `new(Type)` or `new(Type, "comment")`                  |
| Array alloc      | `new(Type, size)` or `new(Type, size, "comment")`      |
| Stack alloc      | `stack(Type)` or `stack(Type, "comment")`              |
| Free memory      | `free(ptr);`                                           |
| Print            | `print(expr)` or `print("fmt %int", val)`              |
| Memory profiler  | `abyss_eye();`                                         |
| Import           | `import std.math;` (or `import a.b.c;`)                |
| Try/catch        | `try { } catch (err) { }`                              |
| Throw            | `throw "message";`                                     |
| Ternary          | `(cond) ? a : b`                                       |
| For loop         | `for (int i = 0; i < n; i++) { ... }`                  |
| While loop       | `while (cond) { ... }`                                 |
| Break            | `break;`                                               |
| Continue         | `continue;`                                            |
| Comment          | `// single line only`                                  |

---

## 23. Architecture Overview

### The Compiler (`abyssc`)

A **two-pass** compiler that reads `.al` source files and emits either bytecode (`.aby`) or transpiled C code.

- **Pass 1 — Signature scan:** Walks the source collecting all top-level declarations (structs, enums, interfaces, function signatures, globals). No bytecode emitted.
- **Pass 2 — Bytecode emission:** Re-walks the source with full symbol knowledge, emitting bytecode. Forward function references are resolved via a patch table.

Key properties:

- **Lexer:** Handwritten scanner with file context stack for multi-file imports; tracks loaded paths for idempotent imports.
- **Parser:** Recursive descent with 12-level precedence climbing. No AST — bytecode emitted directly during parse.
- **Bytecode format:** Custom binary — 7-byte magic `ABYSSBC`, version byte, string table, struct table, instruction stream.

### The Virtual Machine (`abyss_vm`)

A stack-based virtual machine with computed-goto dispatch:

- **Stack:** 1 MiB value stack (1,048,576 slots of 64 bits each)
- **Call stack:** 4,096 frames
- **Exception stack:** 256 frames
- **Dispatch:** Computed goto (GCC label-as-value extension)
- **Opcodes:** 55+ instructions covering arithmetic, control flow, memory, I/O, exceptions, dynamic dispatch, and profiling

### Native AOT Compiler

Bytecode-to-C transpiler producing standalone native binaries:

- **Translation:** Each bytecode opcode → equivalent C code, one statement per opcode
- **Optimization:** GCC `-O3 -flto=auto -march=native`
- **Value representation:** `union { int64_t i; double f; void *p; }` — eliminates memcpy overhead present in the VM
- **Control flow:** Computed-goto jump table preserved from VM dispatch
- **Output:** Standalone ELF binary, no runtime dependencies

### Bytecode Format

```
Bytes 0-6:   Magic "ABYSSBC"
Byte 7:      Version number (currently 12)
Bytes 8-11:  String table count
...          String table entries (length-prefixed)
Next 4:      Struct table count
...          Struct table entries (name + size)
Next 4:      Code size
...          Instruction stream
```

---

## 24. Roadmap

Known limitations with fixes planned. In rough priority order:

**Language features**

- Char literals: `'A'` → 65
- Hex literals: `0xFF`
- Block comments: `/* ... */`
- `*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `\|=`, `^=` compound assignment operators
- Full `i += N` support in for-loop step
- Short-circuit `&&` and `\|\|`
- `int ↔ float` auto-promotion in mixed-type arithmetic
- `str + int` via implicit `to_str` conversion
- Tuple returns through interface dispatch
- `null` keyword and pointer-null comparison

**Type system**

- Distinguish `int` vs `char` (currently interchangeable)
- Actual argument-type checking at call sites
- Array element-type tracking through indexing

**Runtime**

- Optional `--safe` compile flag enabling:
  - Array bounds checks
  - Null pointer checks
  - Division-by-zero → catchable throw
- Source line/column → bytecode mapping for runtime errors
  (currently shown as IP offset; target is `file.al:42:13` format)
- Stack trace on uncaught `throw`

**Tooling**

- VSCode extension polish (existing basic extension in `/releases`)
- Package manager / module registry
- REPL
- Debugger (step, breakpoints, variable inspection)

**Ecosystem**

- stdlib: `std.str` (splitting, substring, parsing)
- stdlib: `std.fs` (file I/O beyond stdin)
- stdlib: `std.net` (socket bindings)
- stdlib: `std.json`

---

*AbyssLang — Built from scratch. Zero dependencies. Total control.*
*Designed in Tashkent, Uzbekistan.*
