# AbyssLang — Complete Language Documentation

> **The first programming language designed and built entirely in Uzbekistan.**
> A statically-typed systems language with manual memory control, native compilation, and a built-in memory profiler.
> Written in ~5,000 lines of pure C11. Zero external dependencies.

**Author:** Abrorbek Patidinov
**License:** Apache 2.0
**Repository:** [github.com/AbrorPatidinov/ProjectAbyss](https://github.com/AbrorPatidinov/ProjectAbyss)
**This document describes:** Bytecode version 13 · 29/29 regression tests green (VM + Native)

---

## What's new in v13

- **Character literals:** `'A'`, `'\n'`, `'\t'`, `'\\'`, `'\''`, `'\0'`, `'\r'`, `'\"'`
- **Hex literals:** `0xFF`, `0xDEADBEEF`
- **Block comments:** `/* ... */`
- **`null` keyword** for pointer-typed variables
- **Full compound assignment operators:** `*=` `/=` `%=` `<<=` `>>=` `&=` `|=` `^=` (plus existing `+=` `-=`), in both statements and for-loop step
- **Short-circuit `&&` and `||`** — the RHS is skipped entirely when the LHS already decides the result
- **Int ↔ float auto-promotion** in arithmetic and comparisons — mixing `int` and `float` now yields correct `float` results (previously produced silent corruption)
- **String + number concatenation** — `"x=" + 42` and `"pi=" + 3.14` now work via implicit numeric→string conversion
- **Tuple returns through interface dispatch** — `a, b = obj.method(x);`
- **Argument type checking at call sites** — passing a `float` where `int` is expected is now a compile-time error
- **Option B int/char distinction** — widening allowed (`int ← char`), narrowing forbidden (`char ← int`) with a suggested fix in the error message
- **Float modulo** now errors cleanly instead of silently producing garbage

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
21. [Type System & Conversions](#21-type-system--conversions)
22. [Gotchas & Things That May Surprise You](#22-gotchas--things-that-may-surprise-you)
23. [Grammar Summary](#23-grammar-summary)
24. [Architecture Overview](#24-architecture-overview)
25. [Roadmap](#25-roadmap)

---

## 1. Quick Start

### Build the Toolchain

```bash
make clean && make
```

Produces:

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

### Verify Install

```bash
bash tests/run.sh
# Expected: "All 29 checks passed."
```

---

## 2. Types & Variables

AbyssLang is statically typed. Every variable must be declared with its type.

### Primitive Types

| Type    | Storage          | Description                                      | Example               |
|---------|------------------|--------------------------------------------------|-----------------------|
| `int`   | 64-bit signed    | Signed integer                                   | `int age = 25;`       |
| `float` | 64-bit IEEE-754  | Double-precision floating point                  | `float pi = 3.14159;` |
| `char`  | 64-bit slot      | Character value (see §21 for char vs int)        | `char grade = 'A';`   |
| `str`   | pointer          | Null-terminated string                           | `str name = "Abyss";` |
| `void`  | —                | No value (return type only)                      | `void main() { }`     |

### Numeric Literals

```
int dec = 42;          // decimal
int hex = 0xFF;        // hexadecimal (255)
int neg = -100;        // negated
float pi = 3.14159;    // float literal (requires the decimal point)
```

### Character Literals

```
char A = 'A';          // 65
char newline = '\n';   // 10
char tab = '\t';       // 9
char null_ch = '\0';   // 0
char quote = '\'';     // 39
char backslash = '\\'; // 92
```

Character literals produce integer values at the bytecode level but are compatible with `char`-typed variables.

### Variable Declaration

```
// With initializer (recommended)
int score = 100;
float velocity = 9.81;
str greeting = "Hello";

// Without initializer — zero-initialized
int counter;     // counter == 0
float f;         // f == 0.0
str s;           // s == NULL pointer (don't use without assigning first)
```

> **Warning:** Declaring a `str` or struct-pointer variable without initializing it produces a **null pointer**. Use `null` explicitly:
>
> ```
> str s = null;                     // explicit, clear intent
> if (s == null) { return; }        // null comparison works
> ```

### Type Promotion

Mixing `int` and `float` in arithmetic auto-promotes the `int`:

```
int a = 5;
float b = 2.5;
float c = a + b;   // c == 7.5  ✓
```

The compiler emits `OP_I2F` at compile time. Pure-int or pure-float code has zero added overhead.

See [§21 Type System & Conversions](#21-type-system--conversions) for the complete matrix.

---

## 3. Operators

### Arithmetic

| Operator | Description                                  | Example         |
|----------|----------------------------------------------|-----------------|
| `+`      | Addition; string concat; number→string cat   | `a + b`         |
| `-`      | Subtraction / unary negation                 | `a - b` or `-x` |
| `*`      | Multiplication                               | `a * b`         |
| `/`      | Division                                     | `a / b`         |
| `%`      | Modulo (integer only — errors on floats)     | `a % b`         |

### String Concatenation with Numbers (v13+)

```
str msg = "x = " + 42;          // "x = 42"
str pi = "pi = " + 3.14;        // "pi = 3.140000"
str mix = "err code " + 0xFF;   // "err code 255"
```

Works both directions: `42 + "ms"` and `"ms: " + 42` both succeed.

### Comparison

| Operator | Description           |
|----------|-----------------------|
| `==`     | Equal to              |
| `!=`     | Not equal to          |
| `<`      | Less than             |
| `>`      | Greater than          |
| `<=`     | Less than or equal    |
| `>=`     | Greater than or equal |

All return `int` (0 or 1). Mixed int/float comparisons auto-promote the int side.

### Logical (Short-Circuit in v13+)

| Operator | Description              |
|----------|--------------------------|
| `&&`     | Short-circuit logical AND |
| `\|\|`   | Short-circuit logical OR  |
| `!`      | Logical NOT              |

`&&` skips the RHS when LHS is false. `||` skips the RHS when LHS is true:

```
// Null guard — ptr.field is never evaluated if ptr is null
if (ptr != null && ptr.field > 0) { ... }

// Early-out — expensive_check() only runs when needed
if (cheap_check() || expensive_check()) { ... }
```

Implemented via `OP_JZ` branch-and-skip. Zero cost for the skipped side.

### Bitwise

| Operator | Description   | Example        |
|----------|---------------|----------------|
| `&`      | Bitwise AND   | `flags & mask` |
| `\|`     | Bitwise OR    | `flags \| bit` |
| `^`      | Bitwise XOR   | `a ^ b`        |
| `~`      | Bitwise NOT   | `~flags`       |
| `<<`     | Left shift    | `val << 2`     |
| `>>`     | Right shift   | `val >> 4`     |

### Assignment & Compound Assignment (v13+)

| Operator | Description            |
|----------|------------------------|
| `=`      | Assign                 |
| `+=`     | Add and assign         |
| `-=`     | Subtract and assign    |
| `*=`     | Multiply and assign    |
| `/=`     | Divide and assign      |
| `%=`     | Modulo and assign      |
| `<<=`    | Left-shift and assign  |
| `>>=`    | Right-shift and assign |
| `&=`     | Bit-AND and assign     |
| `\|=`    | Bit-OR and assign      |
| `^=`     | Bit-XOR and assign     |
| `++`     | Increment by 1         |
| `--`     | Decrement by 1         |

All compound operators work in regular statements **and** in for-loop step expressions.

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

// v13+: any compound assign works in the step
for (int i = 0; i < 100; i += 2) { print(i); }
for (int i = 1024; i > 0; i >>= 1) { print(i); }
```

### break & continue

```
for (int i = 0; i < 100; i++) {
    if (i % 2 != 0) { continue; }
    if (i > 50)     { break;    }
    print(i);
}
```

### Block Scoping

```
{
    int temp = 42;
    print(temp);   // works
}
// temp no longer accessible — using it is a compile error
```

For-loop iteration variables are block-scoped.

---

## 5. Functions

### Declarations

Three equivalent forms:

```
// Form 1: void function
void greet() {
    print("Hello!");
}

// Form 2: return type before name
int double_it(int x) {
    return x * 2;
}

// Form 3: `function` keyword with named returns
function square(int x) : (int result) {
    return x * x;
}
```

### Forward References

Functions may be called before they are defined. Two-pass compilation collects signatures first, then compiles bodies:

```
void main() {
    print(is_even(10));   // defined below, still works
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

```
function divide(int a, int b) : (int quot, int rem) {
    quot = a / b;
    rem = a % b;
    return quot, rem;
}

void main() {
    int q; int r;
    q, r = divide(17, 5);
    print("17 / 5 = %int remainder %int", q, r);
    // Output: 17 / 5 = 3 remainder 2
}
```

**v13+:** Tuples also work through interface dispatch:

```
interface Divider {
    function split(int v) : (int half, int rem);
}

function half_split(int v) : (int a, int b) {
    a = v / 2;
    b = v % 2;
    return a, b;
}

void main() {
    Divider d = stack(Divider);
    d.split = half_split;

    int h; int r;
    h, r = d.split(17);        // interface tuple dispatch
    print("%int %int", h, r);   // 8 1
}
```

### Argument Type Checking (v13+)

The compiler verifies every argument type at compile time:

```
void take_int(int x) { print(x); }

void main() {
    char c = 'A';
    take_int(c);   // OK — widening char → int

    float f = 3.14;
    take_int(f);   // ERROR: cannot assign float to int (narrowing)
}
```

See [§21 Type System & Conversions](#21-type-system--conversions).

### Namespaced Functions

```
function std.math.pow(int base, int exp) : (int result) {
    // ...
}
```

Callers use the full path: `std.math.pow(2, 10)`.

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

p.position = new(Vector, "Player Pos");
p.position.x = 10.5;
p.position.y = 20.0;

// All compound assignments work on fields
p.health -= 25;
p.health *= 2;
p.position.x += 1.0;

p.health++;
p.health--;
```

> Every struct field occupies one 64-bit slot regardless of declared type. A `char` field uses 8 bytes, not 1. This simplifies the bytecode but means AbyssLang is not suitable for packed binary protocol structs directly. Use `int[]` for byte packing.

---

## 7. Memory Management

AbyssLang gives explicit control over memory. **No garbage collector.**

### Heap Allocation — `new()`

```
// With optional Abyss Eye comment
Player p = new(Player, "Main Character");

// Without comment
Player q = new(Player);
```

### Array Allocation — `new(Type, size)`

```
int[] scores = new(int, 100, "Player Scores");
int[] data   = new(int, 256);
```

> Element size is fixed at 8 bytes. `new(char, 256)` allocates 2048 bytes.

### Stack Allocation — `stack()`

Auto-freed when the enclosing function returns. Faster than heap.

```
function calculate() : (float result) {
    Vector temp = stack(Vector, "Temp Calc");
    temp.x = 5.0;
    temp.y = 10.0;
    return temp.x + temp.y;
    // No free() needed — temp vanishes automatically
}
```

### Manual Free — `free()`

```
Player p = new(Player, "Hero");
free(p);     // memory returned; p is now dangling — reassign or set p = null
```

> **Double-free is undefined.** Use `p = null;` after freeing so later access crashes fast at the null check.

### Summary

| Method         | Lifetime                  | Speed   | Use Case                |
|----------------|---------------------------|---------|-------------------------|
| `new(Type)`    | Until `free()` called     | Normal  | Long-lived objects      |
| `new(Type, N)` | Until `free()` called     | Normal  | Dynamic arrays          |
| `stack(Type)`  | Until function returns    | Fastest | Temporary / local data  |

---

## 8. Enums

```
enum State {
    IDLE,             // 0
    RUNNING = 5,      // 5
    STOPPED,          // 6 (auto-increments)
    ERROR   = -1      // -1
}

void main() {
    int current = State.RUNNING;
    if (current == State.RUNNING) {
        print("Active.");
    }
}
```

Enums can be used as variable types:

```
State s = State.IDLE;
```

**Limit:** max 64 values per enum.

---

## 9. Interfaces

Interfaces define a signature contract. Any function matching the signature can be assigned to an interface field at runtime.

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

Dispatch uses `OP_CALL_DYN_BOT` — no vtable, no runtime type identity. The only guarantee is signature compatibility.

**v13+:** Tuple returns through interface dispatch work (see §5).

---

## 10. Arrays

Heap-allocated, fixed size known at allocation time.

```
int[] scores = new(int, 10, "Scores");

scores[0] = 100;
scores[1] = 200;

print(scores[0]);

scores[0]++;
scores[1]--;

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

> **⚠ No bounds checking.** Out-of-range writes silently corrupt memory. Track lengths yourself. `--safe` mode with runtime bounds checks is on the roadmap.

---

## 11. Strings

### Literals

```
str greeting = "Hello, World!";
```

Null-terminated byte pointers. Not length-prefixed.

### Dynamic Concatenation

```
str first = "Hello, ";
str second = "World!";
str message = first + second;     // heap allocated
free(message);
```

### Number Concatenation (v13+)

```
int err = 0xDEAD;
str report = "error: " + err;    // "error: 57005"

float r = 0.95;
str label = "ratio=" + r;         // "ratio=0.950000"
```

Dynamic strings are tracked by the Abyss Eye as `DynString`.

---

## 12. Error Handling

Structured `try` / `catch` / `throw`. Errors are string values only.

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

### Nested

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

`throw` is cooperative. The VM does not convert hardware signals into catchable exceptions:

- **Division by zero** — hardware SIGFPE; dies immediately
- **Null pointer dereference** — segfaults
- **Array out-of-bounds** — silently corrupts memory
- **Stack overflow** — caught by VM, fatal (not catchable)

Guard before the operation:

```
function safe_divide(int a, int b) : (int res) {
    if (b == 0) { throw "division by zero"; }
    return a / b;
}
```

Short-circuit makes null guards idiomatic:

```
if (ptr != null && ptr.value > 0) { /* safe */ }
```

---

## 13. Modules & Imports

### Importing

```
import std.math;
import std.time;
```

`import a.b.c;` loads `a/b/c.al` relative to the current directory.

### Idempotent

```
import std.math;
import std.math;         // no-op; already loaded
```

### Defining Module Functions

```
function std.math.pow(int base, int exp) : (int result) {
    result = 1;
    for (int i = 0; i < exp; i++) {
        result = result * base;
    }
    return result;
}
```

---

## 14. The Abyss Eye

The Abyss Eye is AbyssLang's built-in memory profiler. Not an external tool — woven into the instruction set at the opcode level.

### Usage

```
abyss_eye();     // summons the memory HUD at this point
```

### What It Displays

| Section            | Information                                                             |
|--------------------|-------------------------------------------------------------------------|
| System Resources   | Stack/heap totals, freed bytes, allocation count                        |
| Active Memory      | Every live pointer: address, size, type, variable name, scope, hex dump |
| Lifecycle Analysis | Full allocation IP → free IP history; leak warnings                     |

### Memory Comments

```
int[] buffer = new(int, 1024, "Network Receive Buffer");
abyss_eye();
// Shows: buffer | Heap | 8192 bytes | Array | "Network Receive Buffer"
```

### How Variable Names Get There

The compiler emits `OP_TAG_ALLOC` when `new()` or `stack()` is assigned to a named variable. This propagates the source-level variable name to the runtime allocation tracker — no debug symbols needed.

Dynamic allocations (`"x=" + n`, `str + str`) are labeled automatically as `DynString`, `int->str`, `float->str`, or `Dynamic String Concat`.

### VM vs Native Mode

- **VM mode:** Full HUD with colored terminal output, hex dumps, lifecycle tracking. Always on.
- **Native mode (default):** Disabled for zero overhead. Stubs compile to empty.
- **Native + `--eye`:** Profiler active with O(1) hashtable-backed tracker.

  ```bash
  ./abyssc --native --eye program.al output
  ```

---

## 15. Formatted Print

Format specifiers are type-tagged:

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

### Simple Print

```
print(42);          // 42\n
print(3.14);        // 3.140000\n
print("hello");     // hello\n
print(someChar);    // <one character>  ← no trailing newline
```

> `print()` of `int`/`float`/`str` appends a newline. `print()` of a single `char` does NOT (uses `OP_PRINT_CHAR`, one byte).

---

## 16. Native Bridge Functions

| Function       | Returns | Description                            |
|----------------|---------|----------------------------------------|
| `clock()`      | `float` | Monotonic clock, seconds since start   |
| `input_int()`  | `int`   | Blocking line read from stdin, parsed  |

> `input_int()` reads a line via `fgets` and parses via `atoll`. Non-integer input returns 0.

---

## 17. Compilation Modes

### VM Mode

```bash
./abyssc program.al program.aby
./abyss_vm program.aby
```

- Full Abyss Eye profiler
- Computed-goto dispatch
- Bytecode magic `ABYSSBC`, version byte **13**

### Native AOT Mode

```bash
./abyssc --native program.al program_native
./program_native
```

- Bytecode → C → GCC `-O3 -flto=auto -march=native`
- Abyss Eye disabled (zero overhead)
- Standalone ELF binary
- Typical: **5×–10× faster than Python** (verified: Mandelbrot 7.74×, Packet Scan 8.02×, Prime Sieve 4.76×)

### Native + Profiler

```bash
./abyssc --native --eye program.al program_debug
```

Native speed with O(1)-backed Abyss Eye.

---

## 18. Standard Library Reference

The standard library is written entirely in AbyssLang.

### std.math

| Function                                                                          | Description                            |
|-----------------------------------------------------------------------------------|----------------------------------------|
| `std.math.abs(int x) : int`                                                       | Absolute value                         |
| `std.math.pow(int base, int exp) : int`                                           | Integer exponentiation                 |
| `std.math.min(int a, int b) : int`                                                | Minimum of two                         |
| `std.math.max(int a, int b) : int`                                                | Maximum of two                         |
| `std.math.clamp(int val, int lo, int hi) : int`                                   | Constrain to range                     |
| `std.math.gcd(int a, int b) : int`                                                | GCD (Euclid)                           |
| `std.math.lcm(int a, int b) : int`                                                | LCM                                    |
| `std.math.sqrt(int n) : int`                                                      | Integer sqrt (Newton)                  |
| `std.math.factorial(int n) : int`                                                 | Factorial                              |
| `std.math.is_prime(int n) : int`                                                  | Primality (6k±1)                       |
| `std.math.fib(int n) : int`                                                       | Nth Fibonacci                          |
| `std.math.map(int val, int in_lo, int in_hi, int out_lo, int out_hi) : int`       | Range mapping                          |

### std.time

| Function                                 | Description                            |
|------------------------------------------|----------------------------------------|
| `std.time.now() : float`                 | Monotonic timestamp (seconds)          |
| `std.time.elapsed(float start) : float`  | Seconds since `start`                  |

### std.io

| Function                                  | Description                            |
|-------------------------------------------|----------------------------------------|
| `std.io.banner(str text)`                 | Decorated banner                       |
| `std.io.separator()`                      | Horizontal line                        |
| `std.io.stat(str label, int value)`       | Labeled integer                        |
| `std.io.kv(str key, str value)`           | Key-value pair                         |
| `std.io.prompt_int(str msg) : int`        | Prompt + read int                      |
| `std.io.newline()`                        | Blank line                             |

### std.array

| Function                                                | Description                            |
|---------------------------------------------------------|----------------------------------------|
| `std.array.fill(int[] arr, int len, int val)`           | Fill with value                        |
| `std.array.sum(int[] arr, int len) : int`               | Sum elements                           |
| `std.array.min(int[] arr, int len) : int`               | Minimum                                |
| `std.array.max(int[] arr, int len) : int`               | Maximum                                |
| `std.array.find(int[] arr, int len, int target) : int`  | Linear search (index or -1)            |
| `std.array.count(int[] arr, int len, int target) : int` | Count occurrences                      |
| `std.array.reverse(int[] arr, int len)`                 | Reverse in-place                       |
| `std.array.swap(int[] arr, int i, int j)`               | Swap two elements                      |
| `std.array.sort(int[] arr, int len)`                    | Insertion sort                         |
| `std.array.is_sorted(int[] arr, int len) : int`         | Sorted check (1=yes, 0=no)             |
| `std.array.copy(int[] src, int[] dst, int len)`         | Copy contents                          |

---

## 19. Compile-Time Limits

| Construct                           | Limit | Why                                              |
|-------------------------------------|-------|--------------------------------------------------|
| Locals per function                 | 256   | `OP_GET_LOCAL` uses 1-byte index                 |
| Globals total                       | 256   | `OP_GET_GLOBAL` uses 1-byte index                |
| Arguments per call                  | 255   | `OP_CALL` argc is 1 byte                         |
| Tracked arg types (type-check)      | 32    | First 32 args type-checked; rest skipped         |
| Return values per function          | 8     | Fixed buffer in `FuncInfo`                       |
| Fields per struct                   | 64    | Fixed array in `StructInfo.fields[64]`           |
| Values per enum                     | 64    | Fixed array in `EnumInfo.values[64]`             |
| Imported modules total              | 128   | Static loaded-files table                        |

Each limit produces a specific actionable error, never silent corruption.

---

## 20. Runtime Safety Guarantees

### Stack Overflow

```
[FATAL ERROR] Stack overflow (limit 1048576 slots).
  Usually deep recursion or expressions pushing without popping.
```

### Call-Stack Overflow

```
[FATAL ERROR] Call stack overflow (limit 4096 frames).
  Usually unbounded recursion. Check base cases.
```

Infinite recursion caught cleanly — no segfault.

### Bytecode Validation

```
[FATAL ERROR] Bytecode version mismatch.
  File program.aby was compiled with bytecode v12.
  This VM expects v13. Recompile with the matching abyssc.
```

### What Is NOT Protected

- Array bounds (§10, §22)
- Null pointer dereference (§2, §22)
- Double-free (§7)
- Division by zero (§12)
- Integer overflow (wraps silently; usual 64-bit behavior)

Explicit design choices. Roadmap includes opt-in `--safe` flag.

---

## 21. Type System & Conversions

AbyssLang v13 has a compile-time type system with **Option B semantics**: widening implicit, narrowing forbidden without explicit intent.

### Conversion Matrix

| Source \ Target | `int`    | `float`   | `char`    | `str`        | `struct S` | `T[]`       |
|-----------------|----------|-----------|-----------|--------------|------------|-------------|
| `int`           | ✓ exact  | ✓ `I2F`   | ✗ error   | ✗ (use `+`)  | null only  | null only   |
| `float`         | ✗ error  | ✓ exact   | ✗ error   | ✗ (use `+`)  | ✗ error    | ✗ error     |
| `char`          | ✓ widen  | ✓ `I2F`   | ✓ exact   | ✗ (use `+`)  | ✗ error    | ✗ error     |
| `str`           | ✗ error  | ✗ error   | ✗ error   | ✓ exact      | ✗ error    | ✗ error     |
| `null`          | ✓        | ✗ error   | ✗ error   | ✓            | ✓          | ✓           |
| `struct S`      | ✗        | ✗         | ✗         | ✗            | ✓ if match | ✗           |
| `T[]`           | ✗        | ✗         | ✗         | ✗            | ✗          | ✓ if match  |

### Errors with Fix Hints

```
[FATAL ERROR] call to 'take_char' arg 1: cannot assign int to char implicitly
  (narrowing, may lose data). If intentional, use explicit modulo:
  `c = val % 256;`
```

```
[FATAL ERROR] Cannot assign float to int implicitly (narrowing, truncates).
  If intentional, multiply and cast through an integer helper.
```

### Arithmetic Promotion

Mixing `int` and `float` auto-inserts `OP_I2F` on the int side:

```
int a = 5;
float b = 2.5;
float c = a + b;       // emits:  <push a> I2F <push b> ADD_F
                       // result: 7.5
```

Pure-int and pure-float code is unchanged.

### Explicit Narrowing

No cast syntax yet. To narrow intentionally:

```
// int -> char: use modulo
char c = (val % 256);

// float -> int: manual truncation (std.math.trunc on roadmap)
```

---

## 22. Gotchas & Things That May Surprise You

1. **`//` and `/* */` are the two comment syntaxes.** Both work in v13+.
2. **All values are 64-bit slots internally.** `char` and `int` occupy the same space. Distinction is enforced at the type level, not storage.
3. **`print()` with `%` requires exact-spelling type tags** (`%int`, `%str`). C's `%d`, `%s` don't work.
4. **Uninitialized `str` is null.** Using it crashes. Use `null` explicitly.
5. **Double-free is undefined.** Don't call `free()` twice.
6. **Arrays have no bounds checking.** Track length yourself.
7. **`try/catch` doesn't catch hardware signals.** Guard before the op.
8. **Interface methods returning tuples** unpack with `a, b = obj.method(x);`
9. **`str + int` allocates.** Each concatenation is a heap allocation tracked by Abyss Eye. For hot loops, preallocate.
10. **No cast syntax yet.** Narrowing requires explicit ops (e.g. `x % 256`).
11. **Short-circuit is always on.** `0 && foo()` never calls `foo()`. Rely on this for null guards.

---

## 23. Grammar Summary

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
| Heap alloc       | `new(Type)` / `new(Type, "comment")`                   |
| Array alloc      | `new(Type, size)` / `new(Type, size, "comment")`       |
| Stack alloc      | `stack(Type)` / `stack(Type, "comment")`               |
| Free memory      | `free(ptr);`                                           |
| Print            | `print(expr)` / `print("fmt %int", val)`               |
| Memory profiler  | `abyss_eye();`                                         |
| Import           | `import std.math;` / `import a.b.c;`                   |
| Try/catch        | `try { } catch (err) { }`                              |
| Throw            | `throw "message";`                                     |
| Ternary          | `(cond) ? a : b`                                       |
| For loop         | `for (int i = 0; i < n; i++) { ... }`                  |
| For loop (step)  | `for (int i = 0; i < n; i += 2) { ... }`               |
| While loop       | `while (cond) { ... }`                                 |
| Break            | `break;`                                               |
| Continue         | `continue;`                                            |
| Line comment     | `// single line`                                       |
| Block comment    | `/* multi-line */`                                     |
| Integer literal  | `42` / `0xFF`                                          |
| Float literal    | `3.14`                                                 |
| Char literal     | `'A'` / `'\n'` / `'\t'` / `'\0'` ...                   |
| String literal   | `"Hello"`                                              |
| Null literal     | `null`                                                 |

---

## 24. Architecture Overview

### Compiler (`abyssc`)

**Two-pass** design:

- **Pass 1 — Signature scan:** Collects top-level declarations (structs, enums, interfaces, function signatures with argument types, globals). No bytecode emitted.
- **Pass 2 — Bytecode emission:** Re-walks with full symbol knowledge. Forward calls resolved via patch table. Argument types checked at every call site.

Properties:

- **Lexer:** Handwritten. File-context stack for imports. Supports decimal/hex integer literals, float literals, character literals with escapes, line + block comments.
- **Parser:** Recursive descent, 12-level precedence climbing. No AST — bytecode emitted directly.
- **Type checker:** Integrated. Every assignment, return, and call-site argument runs through `check_assign_compat()` with Option B.
- **Bytecode:** `ABYSSBC` magic, version byte (13), string table, struct table, instruction stream.

### Virtual Machine (`abyss_vm`)

Stack-based with computed-goto dispatch:

- **Stack:** 1 MiB value stack (1,048,576 × 64-bit slots)
- **Call stack:** 4,096 frames
- **Exception stack:** 256 frames
- **Opcodes (v13):** 60+ instructions. New in v13: `OP_I2F`, `OP_F2I`, `OP_INT_TO_STR`, `OP_FLOAT_TO_STR`, `OP_SWAP`.

### Native AOT

- Each opcode → equivalent C statement
- GCC `-O3 -flto=auto -march=native`
- Value union: `union { int64_t i; double f; void *p; }`
- Computed-goto jump table preserved
- Output: standalone ELF, no runtime dependencies

### Bytecode Format

```
Bytes 0-6:   Magic "ABYSSBC"
Byte 7:      Version (13)
Bytes 8-11:  String table count
...          String table entries (length-prefixed)
Next 4:      Struct table count
...          Struct table entries (name + size)
Next 4:      Code size
...          Instruction stream
```

---

## 25. Roadmap

**Language features**

- Explicit cast syntax: `(int)f`, `(char)(n % 256)`
- Array element-type tracking through indexing
- `float` modulo via `std.math.fmod()`
- Array literals: `int[] a = {1, 2, 3};`
- Named struct literal syntax: `Vec{x=1, y=2}`

**Runtime**

- Optional `--safe` compile flag:
  - Array bounds checks
  - Null pointer checks
  - Division-by-zero → catchable throw
- Source line/column in runtime errors (currently IP offset)
- Stack trace on uncaught `throw`

**Tooling**

- VSCode extension polish
- Package manager / module registry
- REPL
- Debugger (step, breakpoints, variable inspection)

**Ecosystem**

- `std.str` (splitting, substring, parsing, to_int, trunc)
- `std.fs` (file I/O beyond stdin)
- `std.net` (socket bindings)
- `std.json`

---

*AbyssLang v13 — Built from scratch. Zero dependencies. Total control.*
*Designed in Tashkent, Uzbekistan.*
