
<p align="center">
  <img src="AbyssLang.png" alt="AbyssLang Logo" width="800"/>
</p>

<h1 align="center">AbyssLang</h1>

<p align="center">
A systems programming language built from scratch in C.<br>
Custom compiler. Custom virtual machine. Explicit control.
</p>

---

## 🌌 Overview

AbyssLang is an experimental systems programming language focused on clarity, determinism, and low-level control.

It is built entirely from scratch and includes:

- A full compiler written in C
- A custom bytecode format
- A stack-based virtual machine
- Native standard modules
- A Rust-backed extension bridge
- Benchmarking tools
- A VSCode extension for syntax support

This project explores language design and runtime architecture without hiding internal mechanics.

---

## 🧠 Architecture

### Compiler (C)

Located in:

/src  
/include  

Components:

- Lexer  
- Parser  
- Symbol table  
- Code generation  
- Bytecode emission  

The compiler produces `.aby` bytecode files.

---

### Virtual Machine

Executable:

abyss_vm  

The VM is:

- Stack-based  
- Deterministic  
- Separate from the compiler for modular design  

---

### Standard Library

#### Native Abyss Modules

/std  

Includes core functionality such as I/O and math.

#### Rust Bridge Layer

/std_rust  
/include/bridge.h  

Rust modules are compiled into a static library and linked through a C bridge layer, enabling high-performance extensions while keeping the core minimal.

---

## 🛠 Build Instructions

Requirements:

- GCC or Clang  
- GNU Make  

Build:

    make

Outputs:

- abyssc      (compiler)  
- abyss_vm    (virtual machine)  

---

## 🚀 Example

Create `hello.al`:

    print("Hello from AbyssLang\n");

Compile:

    ./abyssc hello.al

Run:

    ./abyss_vm hello.aby

---

## 📦 Example Programs

Included programs:

- physics.al  
- rpg.al  
- benchmark.al  
- stack_test.al  
- ultimate.al  

These validate compiler behavior and runtime execution.

---

## 📊 Benchmarking

Benchmark scripts:

- benchmark.sh  
- bench_compute.sh  
- bench_physics.sh  

Used for measuring execution performance and runtime stability.

---

## 🧩 VSCode Extension

Located in:

/vscode  
/releases/abysslang-0.0.1.vsix  

Provides:

- Syntax highlighting  
- Snippets  
- Language configuration  

---

## 🧪 Testing

The `/tests` directory contains programs for:

- Compilation validation  
- Bytecode correctness  
- Stack behavior  
- Runtime verification  

---

## 🧭 Roadmap

Planned improvements:

- Enhanced diagnostics  
- Optimization passes  
- Expanded standard library  
- Module system  
- REPL  
- Cross-platform builds  
- Improved memory tooling  

---

## 🤝 Contributing

Contributions are welcome.

Please read `CONTRIBUTING.md` before submitting pull requests.  
Major architectural changes should be discussed first.

---

## 📜 License

AbyssLang is licensed under the Apache License 2.0.

See the `LICENSE` file for details.
