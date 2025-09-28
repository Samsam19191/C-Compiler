# PLD-Comp – C++ Compiler Project

This project is a **compiler for an object-oriented language**, developed as part of the PLD-Comp course at INSA Lyon.  
The compiler is written in **C++** with **ANTLR4** for parsing, and it generates **ARM assembly code** that can be executed on a **Raspberry Pi 3**.

---

## ✨ Features

- Supports the compilation of a subset of C-like / object-oriented language constructs:
  - Variables and expressions
  - Functions with parameters and return values
  - Control structures: `if/else`, `while`
  - Initialization at declaration
  - Operators: `+=`, `-=`, `*=`, `++`, `--`
  - Support for `char` type
- Generates **ARM assembly code** compatible with Raspberry Pi 3
- Modular design to facilitate extensions and improvements
- Tested against GCC for validation and correctness

---

## 🛠️ Tech Stack

- **Language:** C++17
- **Parsing:** ANTLR4
- **Build System:** CMake / Make
- **Target Architecture:** ARM (Raspberry Pi 3)

---
Developed by a team of 6 INSA Lyon students
