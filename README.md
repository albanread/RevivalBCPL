# A simple BCPL Compiler for Apple Silicon

This repository contains a simple, modern compiler for the historic BCPL programming language, built from scratch with the goal of running on contemporary Apple Silicon (AArch64) hardware.

The primary motivation for this project is to (a) have fun (b) create a straightforward compiler for an important historical language, making it accessible and fun to use on a modern Mac. It's an exploration of compiler construction, a tribute to a foundational language, and an experiment in mapping a typeless language paradigm onto the highly-structured ARM64 architecture.

## Project Goals

* **Learn:** To gain a practical, hands-on understanding of the complete compiler pipeline, from lexing and parsing to register allocation and native code generation.
* **Explore:** To investigate how a typeless, word-addressable language like BCPL can be implemented on a modern, 64-bit architecture.
* **Modernize:** To create a standalone tool that targets AArch64 assembly, allowing classic BCPL code to run on today's Apple hardware.
* **Have Fun:** To rediscover the elegant simplicity of BCPL and share that experience with others who are curious about the history of computing.
* **explore LLMs:** I credit Gemini Pro 2.5 and Gemini CLI for their awesome, yet frustrating assistance.

## The History of BCPL

BCPL, or the **B**asic **C**ombined **P**rogramming **L**anguage, holds a significant place in the history of computer science.
* It was designed by **Martin Richards** at the University of Cambridge in 1966.
* It was created as a simplified, more portable version of its predecessor, CPL (Combined Programming Language).
* BCPL's philosophy of a small, simple compiler and its focus on portability were highly influential. It famously introduced the "brace" style (`$(` and `$)` in BCPL) for code blocks.
* Its most famous legacy is its direct influence on the **B** language, developed by Ken Thompson at Bell Labs. The B language, in turn, was the immediate predecessor to Dennis Ritchie's **C** programming language. This makes BCPL a direct ancestor of many of the most widely-used languages today.

## Current Status & Features

This compiler is an active work in progress. It successfully parses BCPL source files and generates executable ARM64 assembly code.

* **Target Architecture:** AArch64 (Apple Silicon).
* **Compiler Pipeline:** Features a full multi-stage pipeline including a preprocessor, parser, Abstract Syntax Tree (AST) generator, and native code generator.
* **Register Management:** Implements a robust register management system with:
    * A `RegisterManager` for tracking variables that live in callee-saved registers.
    * A `ScratchAllocator` for temporary, caller-saved registers used during expression evaluation.
* **Tail Call Optimization (TCO):** The compiler can identify tail-recursive functions and generate highly efficient, loop-like assembly that avoids stack growth. This is demonstrated in the `FACT_TAIL` function.
* **Language Support:** The compiler currently supports a core subset of BCPL features, including:
    * `LET` and `BE` declarations for functions and routines.
    * `VALOF` blocks for expression-based results.
    * `IF`/`THEN` conditional statements.
    * `FOR` loops.
    * Standard and tail-recursive function calls.

## Building and Running

The project is built with standard C++ tools.

### Prerequisites
* A C++ compiler (e.g., Clang, which comes with Xcode Command Line Tools)
* `make`

### Steps

1.  **Clone the repository:**
    ```bash
    git clone <your-repo-url>
    cd <your-repo-directory>
    ```

2.  **Build the compiler:**
    ```bash
    make
    ```
    This will create the `bcpl_compiler` executable in the root directory.

3.  **Compile a BCPL source file:**
    ```bash
    # Compile the test file to ARM64 assembly
    ./bcpl_compiler tests/test_fact.bcpl -o tests/test_fact.s
    ```

4.  **Assemble and link the output:**
    ```bash
    # Use the system assembler to create an object file
    as -o tests/test_fact.o tests/test_fact.s

    # Link the object file into an executable on macOS
    ld -o test_fact tests/test_fact.o -lSystem -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -e _start
    ```

5.  **Run the executable:**
    ```bash
    ./test_fact
    ```
