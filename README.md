# AbyssLang  Abyss

<p align="center">
  <img src="https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fcdn.wallpapersafari.com%2F72%2F46%2Fc7niSv.jpg&f=1&nofb=1&ipt=8aabd7486a84b1341efaff473c0d42216bf14373dc86fe62eb6dc2fbb1847944=AbyssLang" alt="AbyssLang Logo">
</p>

<p align="center">
  <b>A lightweight, high-performance interpreted language with a C-based VM, built for raw speed.</b>
  <br />
  <br />
  <a href="#the-benchmark-that-started-it-all">View Benchmark</a>
  ·
  <a href="https://github.com/AbrorPatidinov/ProjectAbyss/issues">Report Bug</a>
  ·
  <a href="https://github.com/AbrorPatidinov/ProjectAbyss/issues">Request Feature</a>
</p>

<p align="center">
  <img alt="Language" src="https://img.shields.io/badge/language-C-blue.svg?style=for-the-badge">
  <img alt="License" src="https://img.shields.io/badge/license-MIT-green.svg?style=for-the-badge">
  <img alt="Build" src="https://img.shields.io/badge/build-passing-brightgreen.svg?style=for-the-badge">
</p>

---

AbyssLang started with a simple question: can a purpose-built, interpreted language with a lean C virtual machine outperform a highly-optimized, general-purpose language like Python on its own turf?

**The answer is a definitive yes.**

## Table of Contents

- [The Benchmark That Started It All](#the-benchmark-that-started-it-all)
- [Why Is It Faster?](#why-is-it-faster)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## The Benchmark That Started It All

The initial goal was to test a simple, I/O-heavy task: looping 200,000 times and printing a number. To make it a fair fight, the Python script was heavily optimized to use `sys.stdout.write`, which is significantly faster than the standard `print()` function.

AbyssLang didn't just win; it dominated.

#### The Code

| AbyssLang (`sample.al`)                                                                                                                              | Optimized Python (`test.py`)                                                                                                                    |
| ---------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| ```c
// Simple, C-like syntax
print_show("AbyssLang benchmark start\n");

i = 0;
N = 200000;

while (i < N) {
  print(42);
  i = i + 1;
}

print_show("AbyssLang benchmark end\n");
``` | ```python
# Optimized for I/O speed
import sys
w = sys.stdout.write

N = 200000

w("Python benchmark start\n")
for i in range(N):
    w("42\n")
w("Python benchmark end\n")
``` |

#### The Results

The benchmark, executed on Arch Linux, highlights AbyssLang's superior efficiency.

```sh
# --- AbyssLang (VM) Performance ---
# User time (seconds): 0.02
# Maximum resident set size (kbytes): 2088

# --- Python 3 Performance ---
# User time (seconds): 0.03
# Maximum resident set size (kbytes): 10204