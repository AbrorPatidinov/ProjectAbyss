# 🌌 AbyssLang: The Official Language Guide

Welcome to AbyssLang! This guide will take you from writing your first "Hello World" to mastering manual memory management and dynamic interface dispatch.

AbyssLang is a statically-typed systems programming language. It is designed to be as readable as Python, but as fast and controllable as C.

---

## Table of Contents
1.[Hello World](#1-hello-world)
2. [Variables & Data Types](#2-variables--data-types)
3. [Operators & Logic](#3-operators--logic)
4. [Control Flow](#4-control-flow)
5. [Functions & Tuple Unpacking](#5-functions--tuple-unpacking)
6. [Structs & Memory Management](#6-structs--memory-management)
7. [The Abyss Eye (Memory HUD)](#7-the-abyss-eye-memory-hud)
8. [Enums & Interfaces](#8-enums--interfaces)
9. [Error Handling](#9-error-handling)
10. [Modules & Standard Library](#10-modules--standard-library)
11. [Compilation Modes](#11-compilation-modes)

---

## 1. Hello World

Every AbyssLang program starts with the `main` function.

```c
void main() {
    print("Hello, Abyss!");
}
```

You can print formatted strings using `%` followed by the type:
```c
void main() {
    str name = "Uzbekistan";
    print("Hello, %str!", name);
}
```

---

## 2. Variables & Data Types

AbyssLang is statically typed. Variables must be declared with their type.

### Primitive Types
```c
int age = 25;          // 64-bit integer
float gravity = 9.81;  // 64-bit floating point
char grade = 'A';      // Single character
str greeting = "Hi";   // String pointer
```

### Dynamic Strings
You can dynamically concatenate strings using the `+` operator. This automatically allocates memory on the heap!
```c
str first = "Hello, ";
str second = "World!";
str message = first + second;
```

### Arrays
Arrays are allocated dynamically. You can specify the type, the size, and an optional **memory comment** (useful for debugging).
```c
// Allocate an array of 5 integers
int[] scores = new(int, 5, "Player high scores");

scores[0] = 100;
scores[1] = 200;

// Arrays must be manually freed!
free(scores);
```

---

## 3. Operators & Logic

AbyssLang supports a full suite of systems-level operators.

**Arithmetic:** `+`, `-`, `*`, `/`, `%`
**Relational:** `==`, `!=`, `<`, `>`, `<=`, `>=`
**Logical:** `&&` (AND), `||` (OR), `!` (NOT)
**Bitwise:** `&`, `|`, `^`, `~`, `<<`, `>>`
**Assignment:** `=`, `+=`, `-=`, `++`, `--`

```c
int flags = 1 | 2 | 8;
if (hp > 0 && !is_poisoned) {
    hp++;
}
```

**Ternary Operator:**
```c
int max = (a > b) ? a : b;
```

---

## 4. Control Flow

### If / Else
```c
if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else {
    print("C");
}
```

### Loops
AbyssLang supports `while` and `for` loops, along with `break` and `continue`.

```c
for (int i = 0; i < 10; i++) {
    if (i == 5) { continue; }
    if (i == 8) { break; }
    print(i);
}

int playing = 1;
while (playing) {
    playing = 0;
}
```

---

## 5. Functions & Tuple Unpacking

Functions in AbyssLang can return **multiple values** simultaneously.

```c
// Function returning two integers
function divide_and_remainder(int a, int b) : (int div, int rem) {
    div = a / b;
    rem = a % b;
    return div, rem;
}

void main() {
    int d;
    int r;
    
    // Tuple unpacking!
    d, r = divide_and_remainder(10, 3);
    
    print("10 / 3 = %int with remainder %int", d, r);
}
```

---

## 6. Structs & Memory Management

AbyssLang gives you explicit control over memory. There is no Garbage Collector to slow down your game or simulation.

### Defining a Struct
```c
struct Vector {
    float x;
    float y;
}
```

### Heap Allocation (`new`)
Memory allocated with `new` lives forever until you explicitly call `free()`.
```c
Vector pos = new(Vector, "Player Position");
pos.x = 10.5;
pos.y = 20.0;

free(pos); // Prevent memory leaks!
```

### Stack Allocation (`stack`)
Memory allocated with `stack` is automatically destroyed when the function finishes. It is incredibly fast.
```c
function calculate() : (int res) {
    Vector temp = stack(Vector, "Temporary Math Vector");
    temp.x = 5.0;
    // No need to call free(temp)! It vanishes automatically.
    return 1;
}
```

---

## 7. The Abyss Eye (Memory HUD)

This is AbyssLang's killer feature. At any point in your program, you can call `abyss_eye();` to summon a futuristic terminal HUD that visualizes your RAM.

```c
void main() {
    int[] data = new(int, 100, "Network Buffer");
    
    // Show me the memory!
    abyss_eye();
    
    free(data);
}
```
**The Abyss Eye shows:**
1. **System Resources:** Total active stack/heap memory and total freed memory.
2. **Active Memory:** A table of all living pointers, their variable names, developer comments, and a **live hex dump** of the data inside them.
3. **Lifecycle Analysis:** Tracks exactly which line of code allocated the memory, and which line of code freed it (or warns you if it is leaking!).

---

## 8. Enums & Interfaces

### Enums
Enums allow you to define named integer constants.
```c
enum State {
    IDLE,           // 0
    RUNNING = 5,    // 5
    STOPPED         // 6 (Auto-increments)
}

void main() {
    State current = State.RUNNING;
    if (current == State.RUNNING) {
        print("System is running!");
    }
}
```

### Interfaces (Dynamic Dispatch)
Interfaces allow you to define a contract and swap out function implementations at runtime.

```c
interface MathOp {
    function execute(int a, int b) : (int res);
}

function add(int a, int b) : (int res) { return a + b; }
function mul(int a, int b) : (int res) { return a * b; }

void main() {
    MathOp op = stack(MathOp);
    
    op.execute = add;
    print("Add: %int", op.execute(10, 5)); // 15
    
    op.execute = mul;
    print("Mul: %int", op.execute(10, 5)); // 50
}
```

---

## 9. Error Handling

AbyssLang uses a modern `try / catch / throw` system for safe error recovery.

```c
function safe_divide(int a, int b) : (int res) {
    if (b == 0) {
        throw "CRITICAL: Division by zero!";
    }
    return a / b;
}

void main() {
    try {
        int x = safe_divide(10, 0);
    } catch (err) {
        print("Caught an error: %str", err);
    }
}
```

---

## 10. Modules & Standard Library

You can split your code into multiple files and import them. AbyssLang comes with a pure self-hosted standard library.

```c
import std.io;
import std.math;

void main() {
    int p = std.math.pow(2, 10);
    std.io.print_int(p); // 1024
}
```

---

## 11. Compilation Modes

AbyssLang features a dual-engine architecture.

### 1. Resonance VM Mode (For Debugging)
Compiles to bytecode and runs in the Abyss Virtual Machine. This mode enables the `abyss_eye()` memory profiler.
```bash
./abyssc main.al main.aby
./abyss_vm main.aby
```

### 2. Native AOT Mode (For Maximum Speed)
Translates your code to highly optimized C and compiles it directly to raw machine code. Use this for production releases. It is **over 6x faster than Python**.
```bash
./abyssc --native main.al main_native
./main_native
```

---
*Designed and built from scratch in Andijan, Uzbekistan.* 🇺🇿
