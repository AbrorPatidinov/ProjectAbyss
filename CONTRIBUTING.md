# Contributing to AbyssLang

Thank you for your interest in contributing to AbyssLang.

AbyssLang is a systems programming language built from scratch in C, including its own compiler and virtual machine. Contributions should align with the long-term vision of performance, clarity, and low-level control.

---

## Project Philosophy

- Simplicity over unnecessary abstraction
- Explicit behavior over hidden magic
- Performance and control as core principles
- Clean, readable C code
- Deterministic design decisions

Before contributing, please understand the architecture and coding style used in the project.

---

## How to Contribute

### 1. Fork the Repository

Create your own fork and clone it locally: git clone https://github.com/AbrorPatidinov/ProjectAbyss.git


---

### 2. Create a Feature Branch

Do not commit directly to `main`.
git checkout -b feature/your-feature-name


Branch naming examples:

- feature/add-array-support
- fix/parser-memory-leak
- improvement/error-messages

---

### 3. Follow Code Style

- Use consistent indentation
- Keep functions focused and minimal
- Avoid unnecessary global state
- Comment complex logic clearly
- Do not introduce external dependencies without discussion

If you modify core compiler or VM logic, explain your reasoning clearly in the pull request.

---

### 4. Test Your Changes

Before submitting:

- Ensure the project builds successfully
- Test new language features manually
- Confirm no regressions were introduced

If applicable, provide example AbyssLang code demonstrating your feature.

---

### 5. Submit a Pull Request

In your PR:

- Describe what you changed
- Explain why the change is necessary
- Mention any architectural impact
- Reference related issues if applicable

Be concise but thorough.

---

## What Not to Do

- Do not rewrite large parts of the project without discussion
- Do not introduce breaking changes without clear justification
- Do not submit untested code
- Do not mix unrelated changes in one PR

---

## Reporting Bugs

Open an Issue and include:

- Steps to reproduce
- Expected behavior
- Actual behavior
- Platform / compiler details

---

## Feature Requests

Before implementing a major feature:

1. Open an Issue
2. Describe the proposal
3. Explain design decisions
4. Wait for discussion before implementation

---

## Respect the Vision

AbyssLang is an evolving language project with a defined direction.
All contributions are reviewed with long-term architecture in mind.

Thank you for contributing.
