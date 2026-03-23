# AbyssLang — Complete Language Documentation

> **The first programming language designed and built entirely in Uzbekistan.**
> A statically-typed systems language with manual memory control, native compilation, and a built-in memory profiler.
> Written in ~5,000 lines of pure C11. Zero external dependencies.

**Author:** Abrorbek Patidinov
**License:** Apache 2.0
**Repository:** [github.com/AbrorPatidinov/ProjectAbyss](https://github.com/AbrorPatidinov/ProjectAbyss)

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
19. [Grammar Summary](#19-grammar-summary)
20. [Architecture Overview](#20-architecture-overview)

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

```c
void main() {
    print("Hello, Abyss!");
}
```

### Compile & Run

```bash
# VM mode (enables Abyss Eye profiler)
./abyssc hello.al hello.aby
./abyss_vm hello.aby

# Native mode (maximum performance)
./abyssc --native hello.al hello
./hello
```

---

## 2. Types & Variables

AbyssLang is statically typed. Every variable must be declared with its type.

### Primitive Types

| Type    | Size      | Description                          | Example                    |
|---------|-----------|--------------------------------------|----------------------------|
| `int`   | 64-bit    | Signed integer                       | `int age = 25;`            |
| `float` | 64-bit    | Double-precision floating point      | `float pi = 3.14159;`      |
| `char`  | 8-bit     | Single ASCII character               | `char grade = 'A';`        |
| `str`   | pointer   | String (pointer to character data)   | `str name = "Abyss";`      |
| `void`  | —         | No value (function return type only) | `void main() { }`          |

### Variable Declaration

```c
// With initializer
int score = 100;
float velocity = 9.81;
str greeting = "Hello";

// Without initializer (defaults to 0)
int counter;
```

### Type Promotion

When mixing `int` and `float` in arithmetic, the result is `float`:

```c
int a = 5;
float b = 2.5;
float c = a + b;  // 7.5 (auto-promoted to float)
```

---

## 3. Operators

### Arithmetic

| Operator | Description                    | Example        |
|----------|--------------------------------|----------------|
| `+`      | Addition (also string concat)  | `a + b`        |
| `-`      | Subtraction / unary negation   | `a - b` or `-x`|
| `*`      | Multiplication                 | `a * b`        |
| `/`      | Division                       | `a / b`        |
| `%`      | Modulo (integer only)          | `a % b`        |

### Comparison

| Operator | Description        |
|----------|--------------------|
| `==`     | Equal to           |
| `!=`     | Not equal to       |
| `<`      | Less than          |
| `>`      | Greater than       |
| `<=`     | Less than or equal |
| `>=`     | Greater than or equal |

### Logical

| Operator | Description |
|----------|-------------|
| `&&`     | Logical AND |
| `\|\|`   | Logical OR  |
| `!`      | Logical NOT |

### Bitwise

| Operator | Description          | Example          |
|----------|----------------------|------------------|
| `&`      | Bitwise AND          | `flags & mask`   |
| `\|`     | Bitwise OR           | `flags \| bit`   |
| `^`      | Bitwise XOR          | `a ^ b`          |
| `~`      | Bitwise NOT          | `~flags`         |
| `<<`     | Left shift           | `val << 2`       |
| `>>`     | Right shift          | `val >> 4`       |

### Assignment

| Operator | Description         | Equivalent      |
|----------|---------------------|-----------------|
| `=`      | Assign              | `x = 5`         |
| `+=`     | Add and assign       | `x = x + 5`    |
| `-=`     | Subtract and assign  | `x = x - 5`    |
| `++`     | Increment by 1       | `x = x + 1`    |
| `--`     | Decrement by 1       | `x = x - 1`    |

### Ternary

```c
int max = (a > b) ? a : b;
```

### Operator Precedence (highest to lowest)

| Level | Operators                 |
|-------|---------------------------|
| 1     | Unary: `-` `!` `~`        |
| 2     | `*` `/` `%`               |
| 3     | `+` `-`                   |
| 4     | `<<` `>>`                 |
| 5     | `<` `<=` `>` `>=`         |
| 6     | `==` `!=`                 |
| 7     | `&`                       |
| 8     | `^`                       |
| 9     | `\|`                      |
| 10    | `&&`                      |
| 11    | `\|\|`                    |
| 12    | `? :` (ternary)           |

---

## 4. Control Flow

### if / else

```c
if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else {
    print("F");
}
```

### while

```c
int i = 0;
while (i < 100) {
    print(i);
    i++;
}
```

### for

```c
for (int i = 0; i < 100; i++) {
    print(i);
}
```

### break & continue

```c
for (int i = 0; i < 100; i++) {
    if (i % 2 != 0) { continue; }  // Skip odd numbers
    if (i > 50) { break; }          // Stop at 50
    print(i);
}
```

### Block Scoping

Variables declared inside `{ }` blocks are scoped to that block:

```c
{
    int temp = 42;
    print(temp);  // Works
}
// temp is no longer accessible here
```

---

## 5. Functions

### Basic Declaration

There are two syntaxes for functions:

```c
// Syntax 1: void functions
void greet() {
    print("Hello!");
}

// Syntax 2: Return type before name
int double_it(int x) {
    return x * 2;
}

// Syntax 3: function keyword with named returns
function square(int x) : (int result) {
    return x * x;
}
```

### Multiple Return Values (Tuples)

Functions can return multiple values. Callers unpack them with comma-separated assignment:

```c
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

### Namespaced Functions

Functions can be defined with dot-separated names for module organization:

```c
function std.math.pow(int base, int exp) : (int result) {
    // ...
}
```

---

## 6. Structs

### Definition

```c
struct Vector {
    float x;
    float y;
}

struct Player {
    str name;
    int health;
    Vector position;  // Nested struct
}
```

### Field Access

```c
Player p = new(Player, "Main Character");
p.name = "Artorias";
p.health = 100;

// Nested field access
p.position = new(Vector, "Player Pos");
p.position.x = 10.5;
p.position.y = 20.0;

// Field compound assignment
p.health -= 25;
p.position.x += 1.0;

// Field increment/decrement
p.health++;
p.health--;
```

---

## 7. Memory Management

AbyssLang gives you explicit control over memory. There is no garbage collector.

### Heap Allocation: `new()`

Allocates memory on the heap. Lives until explicitly freed with `free()`.

```c
// Struct allocation with optional comment for Abyss Eye
Player p = new(Player, "Main Character");

// Struct allocation without comment
Player q = new(Player);
```

### Array Allocation: `new(type, size)`

```c
// Array with size and optional comment
int[] scores = new(int, 100, "Player Scores");

// Array without comment
int[] data = new(int, 256);
```

### Stack Allocation: `stack()`

Allocates memory on the stack frame. Automatically freed when the enclosing function returns. Faster than heap allocation.

```c
function calculate() : (float result) {
    Vector temp = stack(Vector, "Temp Calc");
    temp.x = 5.0;
    temp.y = 10.0;
    return temp.x + temp.y;
    // No free() needed — temp vanishes automatically
}
```

### Manual Free: `free()`

```c
Player p = new(Player, "Hero");
// ... use p ...
free(p);  // Memory returned to the system
```

### Summary

| Method             | Lifetime                  | Speed    | Use Case                     |
|--------------------|---------------------------|----------|------------------------------|
| `new(Type)`        | Until `free()` is called  | Normal   | Long-lived objects           |
| `new(Type, N)`     | Until `free()` is called  | Normal   | Dynamic arrays               |
| `stack(Type)`      | Until function returns    | Fastest  | Temporary / local data       |

---

## 8. Enums

Enums define named integer constants. Values auto-increment from the last explicit value.

```c
enum State {
    IDLE,           // 0
    RUNNING = 5,    // 5
    STOPPED,        // 6 (auto-increments)
    ERROR = -1      // -1
}

void main() {
    int current = State.RUNNING;
    if (current == State.RUNNING) {
        print("Active.");
    }
}
```

Enums can also be used as variable types:

```c
State s = State.IDLE;
```

---

## 9. Interfaces

Interfaces define a contract: a set of method signatures. Any function matching the signature can be assigned at runtime, enabling dynamic dispatch (polymorphism).

```c
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

---

## 10. Arrays

Arrays are heap-allocated with a fixed size.

```c
// Allocate
int[] scores = new(int, 10, "Scores");

// Write
scores[0] = 100;
scores[1] = 200;

// Read
print(scores[0]);  // 100

// Increment / decrement elements
scores[0]++;
scores[1]--;

// Free
free(scores);
```

### Struct Arrays

```c
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

---

## 11. Strings

### String Literals

```c
str greeting = "Hello, World!";
```

### Dynamic Concatenation

The `+` operator on strings allocates new heap memory:

```c
str first = "Hello, ";
str second = "World!";
str message = first + second;  // "Hello, World!"
```

### String Format Printing

See [Formatted Print](#15-formatted-print).

---

## 12. Error Handling

AbyssLang uses `try` / `catch` / `throw` for structured error recovery. Errors are string values.

```c
function safe_divide(int a, int b) : (int res) {
    if (b == 0) {
        throw "Division by zero!";
    }
    return a / b;
}

void main() {
    try {
        int x = safe_divide(10, 0);
        print(x);  // Never reached
    } catch (err) {
        print("Caught: %str", err);
    }
}
```

### Nested Try/Catch

```c
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

---

## 13. Modules & Imports

### Importing

```c
import std.math;
import std.time;
```

`import std.math;` opens the file `std/math.al` relative to the current directory and includes all its definitions.

### Calling Imported Functions

```c
int result = std.math.pow(2, 10);  // 1024
float now = std.time.now();
```

### Defining Module Functions

Inside `std/math.al`:

```c
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

```c
abyss_eye();  // Summon the memory HUD at any point
```

### What It Displays

| Section             | Information                                                              |
|---------------------|--------------------------------------------------------------------------|
| System Resources    | Active stack/heap memory totals, freed bytes, total allocation count     |
| Active Memory       | Every live pointer: address, size, type, variable name, scope, hex dump  |
| Lifecycle Analysis  | Full history: allocation IP → free IP, leak warnings for unfreed memory  |

### Memory Comments

The optional string argument to `new()` and `stack()` appears in the Abyss Eye output:

```c
int[] buffer = new(int, 1024, "Network Receive Buffer");
abyss_eye();
// Shows: buffer | Heap | 8192 bytes | Array | "Network Receive Buffer"
```

### How It Works

The compiler emits a dedicated `OP_TAG_ALLOC` opcode whenever a `new()` or `stack()` result is assigned to a variable. This opcode propagates the source-level variable name to the runtime allocation tracker, allowing the profiler to display human-readable labels next to raw memory addresses — without debug symbols or source maps.

### VM vs Native Mode

- **VM mode:** Full Abyss Eye with colored terminal HUD, hex dumps, and lifecycle tracking.
- **Native mode (default):** Abyss Eye disabled for maximum performance. Use `--eye` flag to enable:
  ```bash
  ./abyssc --native --eye program.al output
  ```

---

## 15. Formatted Print

The `print()` function supports inline type-tagged format specifiers:

| Specifier | Type    | Example                             |
|-----------|---------|-------------------------------------|
| `%int`    | Integer | `print("Score: %int", score);`      |
| `%float`  | Float   | `print("PI: %float", pi);`         |
| `%str`    | String  | `print("Name: %str", name);`       |
| `%char`   | Char    | `print("Grade: %char", grade);`    |

### Multiple Arguments

```c
print("Player %str scored %int points at (%float, %float)",
      name, score, x, y);
```

### Simple Print

```c
print(42);           // Prints: 42
print(3.14);         // Prints: 3.140000
print("hello");      // Prints: hello
```

---

## 16. Native Bridge Functions

Built-in functions that call into the host runtime:

| Function       | Returns | Description                           |
|----------------|---------|---------------------------------------|
| `clock()`      | `float` | High-resolution monotonic clock (seconds) |
| `input_int()`  | `int`   | Read integer from stdin               |

---

## 17. Compilation Modes

### VM Mode (Debugging & Profiling)

```bash
./abyssc program.al program.aby
./abyss_vm program.aby
```

- Full Abyss Eye memory profiler enabled
- Deterministic stack-based execution
- Bytecode format: custom binary with `ABYSSBC` magic header

### Native AOT Mode (Production Performance)

```bash
./abyssc --native program.al program_native
./program_native
```

- Transpiles bytecode to optimized C
- Compiles with `gcc -O3 -flto=auto -march=native`
- Abyss Eye disabled for zero overhead
- Produces standalone ELF binary with no runtime dependencies

### Native + Profiler Mode

```bash
./abyssc --native --eye program.al program_debug
./program_debug
```

- Native speed with Abyss Eye enabled
- Useful for profiling production-scale workloads

---

## 18. Standard Library Reference

The standard library is written entirely in AbyssLang. Import with `import std.<module>;`.

### std.math

| Function                              | Description                                |
|---------------------------------------|--------------------------------------------|
| `std.math.abs(int x) : int`          | Absolute value                             |
| `std.math.pow(int base, int exp) : int` | Integer exponentiation                  |
| `std.math.min(int a, int b) : int`   | Minimum of two values                      |
| `std.math.max(int a, int b) : int`   | Maximum of two values                      |
| `std.math.clamp(int val, int lo, int hi) : int` | Constrain value to range        |
| `std.math.gcd(int a, int b) : int`   | Greatest common divisor (Euclid)           |
| `std.math.lcm(int a, int b) : int`   | Least common multiple                      |
| `std.math.sqrt(int n) : int`         | Integer square root (Newton's method)      |
| `std.math.factorial(int n) : int`    | Factorial                                  |
| `std.math.is_prime(int n) : int`     | Primality test (6k±1 method)               |
| `std.math.fib(int n) : int`          | Nth Fibonacci number                       |
| `std.math.map(int val, int in_lo, int in_hi, int out_lo, int out_hi) : int` | Range mapping |

### std.time

| Function                              | Description                                |
|---------------------------------------|--------------------------------------------|
| `std.time.now() : float`             | Current monotonic timestamp (seconds)      |
| `std.time.elapsed(float start) : float` | Seconds elapsed since `start`           |

### std.io

| Function                              | Description                                |
|---------------------------------------|--------------------------------------------|
| `std.io.banner(str text)`            | Print decorated banner                     |
| `std.io.separator()`                 | Print horizontal line                      |
| `std.io.stat(str label, int value)`  | Print labeled integer                      |
| `std.io.kv(str key, str value)`      | Print key-value pair                       |
| `std.io.prompt_int(str msg) : int`   | Print prompt and read integer              |
| `std.io.newline()`                   | Print blank line                           |

### std.array

| Function                              | Description                                |
|---------------------------------------|--------------------------------------------|
| `std.array.fill(int[] arr, int len, int val)` | Fill array with value             |
| `std.array.sum(int[] arr, int len) : int`     | Sum all elements                  |
| `std.array.min(int[] arr, int len) : int`     | Find minimum value                |
| `std.array.max(int[] arr, int len) : int`     | Find maximum value                |
| `std.array.find(int[] arr, int len, int target) : int` | Linear search (returns index or -1) |
| `std.array.count(int[] arr, int len, int target) : int` | Count occurrences          |
| `std.array.reverse(int[] arr, int len)`       | Reverse array in-place            |
| `std.array.swap(int[] arr, int i, int j)`     | Swap two elements                 |
| `std.array.sort(int[] arr, int len)`          | Sort array (insertion sort)       |
| `std.array.is_sorted(int[] arr, int len) : int` | Check if sorted (1=yes, 0=no) |
| `std.array.copy(int[] src, int[] dst, int len)` | Copy array contents             |

---

## 19. Grammar Summary

| Construct        | Syntax                                               |
|------------------|------------------------------------------------------|
| Variable         | `type name = expr;`                                  |
| Array            | `type[] name = new(type, size);`                     |
| Struct def       | `struct Name { type field; }`                        |
| Enum def         | `enum Name { A, B = 5, C }`                         |
| Interface def    | `interface Name { function sig; }`                   |
| Function         | `function name(args) : (ret_type ret_name) { }`     |
| Void function    | `void name(args) { }`                               |
| Typed function   | `int name(args) { return expr; }`                    |
| Heap alloc       | `new(Type)` or `new(Type, "comment")`                |
| Array alloc      | `new(Type, size)` or `new(Type, size, "comment")`    |
| Stack alloc      | `stack(Type)` or `stack(Type, "comment")`            |
| Free memory      | `free(ptr);`                                         |
| Print            | `print(expr)` or `print("fmt %int", val)`            |
| Memory profiler  | `abyss_eye();`                                       |
| Import           | `import std.math;`                                   |
| Try/catch        | `try { } catch (err) { }`                           |
| Throw            | `throw "message";`                                   |
| Ternary          | `(cond) ? a : b`                                     |
| For loop         | `for (int i = 0; i < n; i++) { }`                   |
| While loop       | `while (cond) { }`                                   |
| Break            | `break;`                                             |
| Continue         | `continue;`                                          |
| Comment          | `// single line`                                     |

---

## 20. Architecture Overview

### The Compiler (`abyssc`)

A single-pass compiler that reads `.al` source files and emits either bytecode (`.aby`) or transpiled C code. Key properties:

- **Lexer:** Handwritten scanner with file context stack for multi-file imports
- **Parser:** Recursive descent with 12-level precedence climbing
- **Code generation:** Direct bytecode emission (no AST intermediate)
- **Bytecode format:** Custom binary — 7-byte magic `ABYSSBC`, version byte, string table, struct table, instruction stream

### The Virtual Machine (`abyss_vm`)

A stack-based virtual machine with computed goto dispatch:

- **Stack:** 1MB value stack (1,048,576 slots)
- **Call stack:** 4,096 frames
- **Exception stack:** 256 frames
- **Dispatch:** Computed goto (GCC label-as-value extension)
- **Opcodes:** 55+ instructions covering arithmetic, control flow, memory, I/O, and profiling

### Native AOT Compiler

Bytecode-to-C transpiler producing standalone native binaries:

- **Translation:** Each bytecode instruction → equivalent C code
- **Optimization:** GCC `-O3 -flto=auto -march=native`
- **Value representation:** `union { int64_t i; double f; void *p; }` (eliminates memcpy)
- **Control flow:** Computed goto jump table (preserves VM dispatch efficiency)
- **Output:** Standalone ELF binary, no runtime dependencies

### Bytecode Format

```
Bytes 0-6:   Magic "ABYSSBC"
Byte 7:      Version number
Bytes 8-11:  String table count
...          String table entries (length-prefixed)
Next 4:      Struct table count
...          Struct table entries (name + size)
Next 4:      Code size
...          Instruction stream
```

---

*AbyssLang — Built from scratch. Zero dependencies. Total control.*
*Designed in Tashkent, Uzbekistan.*
