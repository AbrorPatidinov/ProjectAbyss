# AbyssLang

![AbyssLang Logo](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fcdn.wallpapersafari.com%2F72%2F46%2Fc7niSv.jpg&f=1&nofb=1&ipt=8aabd7486a84b1341efaff473c0d42216bf14373dc86fe62eb6dc2fbb1847944)

**A lightweight, high-performance interpreted language with a C-based VM.**

AbyssLang is designed to be a "transparent" language. It gives you high-level syntax (structs, functions) but keeps you close to the metal with manual memory management and real-time heap visualization.

### 🚀 Features

*   **Blazing Fast**: Significantly faster than Python in raw compute loops.
*   **Manual Memory Management**: You control the heap with `new()` and `free()`.
*   **Abyss Eye**: A built-in tool (`abyss_eye()`) to visualize the heap state, object sizes, and memory addresses in real-time.
*   **Zero Dependencies**: The entire compiler and VM are written in pure C11 standard library.
*   **Physics Ready**: Supports floating point math, structs, and complex logic.

### 🛠️ Installation

```sh
git clone https://github.com/AbrorPatidinov/ProjectAbyss.git
cd ProjectAbyss
make
```

### ⚡ Quick Start

**1. The Physics Simulation (`physics.al`)**
```c
struct Ball {
    float y;
    float vy;
}

void main() {
    Ball b = new(Ball);
    b.y = 20.0;
    
    // Physics Loop
    while (b.y > 0.0) {
        b.vy = b.vy - 0.05;
        b.y = b.y + b.vy;
        
        if (b.y < 0.0) { b.y = 0.0; }
        
        print(b.y);
    }
    
    abyss_eye(); // Visualize Memory
    free(b);
}
```

**2. Compile & Run**
```sh
./abyssc physics.al physics.aby
./abyss_vm physics.aby
```

### 📊 Benchmarks

*Tested on Arch Linux (x86_64). Lower is better.*

| Task | Iterations | AbyssLang | Python 3 | Speedup |
| :--- | :--- | :--- | :--- | :--- |
| **Pure Compute (Float Math)** | 100,000,000 | **9.66ss** | 12.35s | **~1.28x Faster** |
| **I/O Heavy Loop** | 10,000,000 | **0.107s** | 1.312s | **12.2x Faster** |

*AbyssLang demonstrates lower overhead in arithmetic and object property access compared to CPython in tight compute loops.*

### 📘 Language Documentation

#### 1. Variables & Types
AbyssLang is statically typed.
```c
int count = 10;
float gravity = 9.8;
char letter = 65; // ASCII
str name = "Abyss";
```

#### 2. Structs & Memory
No classes. Only data structures.
```c
struct Player {
    int hp;
    int xp;
}

// Allocate on Heap
Player p = new(Player);
p.hp = 100;

// Inspect Memory
abyss_eye();

// Manual Free
free(p);
```

#### 3. Control Flow
Standard C-style logic.
```c
if (x > 10) {
    print("Big");
} else {
    print("Small");
}

while (x > 0) {
    x = x - 1;
}
```

#### 4. Built-in Functions
| Function | Description |
| :--- | :--- |
| `print(val)` | Prints int, float, or string with newline. |
| `print_char(val)` | Prints a single ASCII character (no newline). |
| `input_int()` | Reads an integer from stdin. |
| `clock()` | Returns current time in seconds (float). |
| `abyss_eye()` | Prints the current state of the Heap. |
| `new(Type)` | Allocates memory for a Struct. |
| `free(var)` | Frees allocated memory. |

### 📜 License
MIT License.
