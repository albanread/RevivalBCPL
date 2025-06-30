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
    ./bcpl_compiler --asm --debug tests/test_fact.bcpl  > tests/test_fact.s
    ```

4.  ** TODO: Assemble and link the output:**
    ```bash
    # Use the system assembler to create an object file - NOT YET :)
    #as -o tests/test_fact.o tests/test_fact.s

    # Link the object file into an executable on macOS
    #ld -o test_fact tests/test_fact.o -lSystem -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -e _start
    ```

5.  ** TODO: Run the executable:**
    ```bash
    ./test_fact
    ```

## Where are we so far

Generating text, we love text.

```C++
 compiler % ./build/compiler --debug --asm tests/test_fact.bcpl
=== BCPL Compiler ===
Source file: "tests/test_fact.bcpl"

Preprocessing...
Preprocessing complete.

=== Preprocessed Source Code ===
// Standard library header - functions are intrinsically known by the compiler/runtime

// Iterative Factorial
LET FACT_ITER(N) = VALOF $(
    LET RESULT = 1
    FOR I = 1 TO N DO $(
        RESULT := RESULT * I
    $)
    RESULTIS RESULT
$)

// Recursive Factorial
LET FACT_REC(N) = VALOF $(
    IF N = 0 THEN RESULTIS 1
    RESULTIS N * FACT_REC(N - 1)
$)

// Helper function with an accumulator
LET FACT_TAIL(N, ACCUMULATOR) = VALOF $(
    // Base case: if N is 0, the final result is in the accumulator.
    IF N = 0 THEN RESULTIS ACCUMULATOR

    // Recursive step: the call to FACT_TAIL is the very last action.
    // All calculations are done *before* the call.
    RESULTIS FACT_TAIL(N - 1, N * ACCUMULATOR)
$)

// Main function that kicks off the process
LET FACT_TCO(N) = VALOF $(
    // Start with an accumulator value of 1.
    RESULTIS FACT_TAIL(N, 1)
$)

LET START() BE $(
    WRITES("Testing Factorial Functions...*N")

    WRITES("*NIterative Factorial:*N")
    WRITES("FACT_ITER(0) = ")
    WRITEN(FACT_ITER(0))
    NEWLINE()

    WRITES("FACT_ITER(1) = ")
    WRITEN(FACT_ITER(1))
    NEWLINE()

    WRITES("FACT_ITER(5) = ")
    WRITEN(FACT_ITER(5))
    NEWLINE()

    WRITES("*NRecursive Factorial:*N")
    WRITES("FACT_REC(0) = ")
    WRITEN(FACT_REC(0))
    NEWLINE()

    WRITES("FACT_REC(1) = ")
    WRITEN(FACT_REC(1))
    NEWLINE()

    WRITES("FACT_REC(5) = ")
    WRITEN(FACT_REC(5))
    NEWLINE()

    WRITES("*NTail-Recursive Factorial:*N")
    WRITES("FACT_TCO(0) = ")
    WRITEN(FACT_TCO(0))
    NEWLINE()

    WRITES("FACT_TCO(1) = ")
    WRITEN(FACT_TCO(1))
    NEWLINE()

    WRITES("FACT_TCO(5) = ")
    WRITEN(FACT_TCO(5))
    NEWLINE()

    WRITES("*NFactorial Tests Finished.*N")
    FINISH
$)

==============================

Parsing...
parseExpression: VALOF
parsePrimaryExpression: VALOF
parseStatement: $(
parseStatement: LET
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: FOR
parseExpression: 1
parsePrimaryExpression: 1
parseExpression: N
parsePrimaryExpression: N
parseStatement: $(
parseStatement: RESULT
parseExpressionStatement: RESULT
parseExpression: RESULT
parsePrimaryExpression: RESULT
parseExpression: RESULT
parsePrimaryExpression: RESULT
parseExpression: I
parsePrimaryExpression: I
parseStatement: RESULTIS
parseExpression: RESULT
parsePrimaryExpression: RESULT
parseExpression: VALOF
parsePrimaryExpression: VALOF
parseStatement: $(
parseStatement: IF
parseExpression: N
parsePrimaryExpression: N
parseExpression: 0
parsePrimaryExpression: 0
parseStatement: RESULTIS
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: RESULTIS
parseExpression: N
parsePrimaryExpression: N
parseExpression: FACT_REC
parsePrimaryExpression: FACT_REC
parseExpression: N
parsePrimaryExpression: N
parseExpression: 1
parsePrimaryExpression: 1
parseExpression: VALOF
parsePrimaryExpression: VALOF
parseStatement: $(
parseStatement: IF
parseExpression: N
parsePrimaryExpression: N
parseExpression: 0
parsePrimaryExpression: 0
parseStatement: RESULTIS
parseExpression: ACCUMULATOR
parsePrimaryExpression: ACCUMULATOR
parseStatement: RESULTIS
parseExpression: FACT_TAIL
parsePrimaryExpression: FACT_TAIL
parseExpression: N
parsePrimaryExpression: N
parseExpression: 1
parsePrimaryExpression: 1
parseExpression: N
parsePrimaryExpression: N
parseExpression: ACCUMULATOR
parsePrimaryExpression: ACCUMULATOR
parseExpression: VALOF
parsePrimaryExpression: VALOF
parseStatement: $(
parseStatement: RESULTIS
parseExpression: FACT_TAIL
parsePrimaryExpression: FACT_TAIL
parseExpression: N
parsePrimaryExpression: N
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: $(
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: Testing Factorial Functions...

parsePrimaryExpression: Testing Factorial Functions...

parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: 
Iterative Factorial:

parsePrimaryExpression: 
Iterative Factorial:

parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_ITER(0) = 
parsePrimaryExpression: FACT_ITER(0) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_ITER
parsePrimaryExpression: FACT_ITER
parseExpression: 0
parsePrimaryExpression: 0
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_ITER(1) = 
parsePrimaryExpression: FACT_ITER(1) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_ITER
parsePrimaryExpression: FACT_ITER
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_ITER(5) = 
parsePrimaryExpression: FACT_ITER(5) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_ITER
parsePrimaryExpression: FACT_ITER
parseExpression: 5
parsePrimaryExpression: 5
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: 
Recursive Factorial:

parsePrimaryExpression: 
Recursive Factorial:

parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_REC(0) = 
parsePrimaryExpression: FACT_REC(0) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_REC
parsePrimaryExpression: FACT_REC
parseExpression: 0
parsePrimaryExpression: 0
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_REC(1) = 
parsePrimaryExpression: FACT_REC(1) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_REC
parsePrimaryExpression: FACT_REC
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_REC(5) = 
parsePrimaryExpression: FACT_REC(5) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_REC
parsePrimaryExpression: FACT_REC
parseExpression: 5
parsePrimaryExpression: 5
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: 
Tail-Recursive Factorial:

parsePrimaryExpression: 
Tail-Recursive Factorial:

parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_TCO(0) = 
parsePrimaryExpression: FACT_TCO(0) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_TCO
parsePrimaryExpression: FACT_TCO
parseExpression: 0
parsePrimaryExpression: 0
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_TCO(1) = 
parsePrimaryExpression: FACT_TCO(1) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_TCO
parsePrimaryExpression: FACT_TCO
parseExpression: 1
parsePrimaryExpression: 1
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: FACT_TCO(5) = 
parsePrimaryExpression: FACT_TCO(5) = 
parseStatement: WRITEN
parseExpressionStatement: WRITEN
parseExpression: WRITEN
parsePrimaryExpression: WRITEN
parseExpression: FACT_TCO
parsePrimaryExpression: FACT_TCO
parseExpression: 5
parsePrimaryExpression: 5
parseStatement: NEWLINE
parseExpressionStatement: NEWLINE
parseExpression: NEWLINE
parsePrimaryExpression: NEWLINE
parseStatement: WRITES
parseExpressionStatement: WRITES
parseExpression: WRITES
parsePrimaryExpression: WRITES
parseExpression: 
Factorial Tests Finished.

parsePrimaryExpression: 
Factorial Tests Finished.

parseStatement: FINISH
Parsing complete.

=== Debug Information ===

--- TOKEN STREAM ---
Line    Col     Type                Text
-----------------------------------------------------
4       1       LET                 'LET'
4       5       Identifier          'FACT_ITER'
4       14      LParen '('          '('
4       15      Identifier          'N'
4       16      RParen ')'          ')'
4       18      Op '='              '='
4       20      VALOF               'VALOF'
4       26      LSection '$('       '$('
5       5       LET                 'LET'
5       9       Identifier          'RESULT'
5       16      Op '='              '='
5       18      IntLiteral          '1'
6       5       FOR                 'FOR'
6       9       Identifier          'I'
6       11      Op '='              '='
6       13      IntLiteral          '1'
6       15      TO                  'TO'
6       18      Identifier          'N'
6       20      DO                  'DO'
6       23      LSection '$('       '$('
7       9       Identifier          'RESULT'
7       16      Op ':='             ':='
7       19      Identifier          'RESULT'
7       26      Op '*'              '*'
7       28      Identifier          'I'
8       5       RSection '$)'       '$)'
9       5       RESULTIS            'RESULTIS'
9       14      Identifier          'RESULT'
10      1       RSection '$)'       '$)'
13      1       LET                 'LET'
13      5       Identifier          'FACT_REC'
13      13      LParen '('          '('
13      14      Identifier          'N'
13      15      RParen ')'          ')'
13      17      Op '='              '='
13      19      VALOF               'VALOF'
13      25      LSection '$('       '$('
14      5       IF                  'IF'
14      8       Identifier          'N'
14      10      Op '='              '='
14      12      IntLiteral          '0'
14      14      THEN                'THEN'
14      19      RESULTIS            'RESULTIS'
14      28      IntLiteral          '1'
15      5       RESULTIS            'RESULTIS'
15      14      Identifier          'N'
15      16      Op '*'              '*'
15      18      Identifier          'FACT_REC'
15      26      LParen '('          '('
15      27      Identifier          'N'
15      29      Op '-'              '-'
15      31      IntLiteral          '1'
15      32      RParen ')'          ')'
16      1       RSection '$)'       '$)'
19      1       LET                 'LET'
19      5       Identifier          'FACT_TAIL'
19      14      LParen '('          '('
19      15      Identifier          'N'
19      16      Comma ','           ','
19      18      Identifier          'ACCUMULATOR'
19      29      RParen ')'          ')'
19      31      Op '='              '='
19      33      VALOF               'VALOF'
19      39      LSection '$('       '$('
21      5       IF                  'IF'
21      8       Identifier          'N'
21      10      Op '='              '='
21      12      IntLiteral          '0'
21      14      THEN                'THEN'
21      19      RESULTIS            'RESULTIS'
21      28      Identifier          'ACCUMULATOR'
25      5       RESULTIS            'RESULTIS'
25      14      Identifier          'FACT_TAIL'
25      23      LParen '('          '('
25      24      Identifier          'N'
25      26      Op '-'              '-'
25      28      IntLiteral          '1'
25      29      Comma ','           ','
25      31      Identifier          'N'
25      33      Op '*'              '*'
25      35      Identifier          'ACCUMULATOR'
25      46      RParen ')'          ')'
26      1       RSection '$)'       '$)'
29      1       LET                 'LET'
29      5       Identifier          'FACT_TCO'
29      13      LParen '('          '('
29      14      Identifier          'N'
29      15      RParen ')'          ')'
29      17      Op '='              '='
29      19      VALOF               'VALOF'
29      25      LSection '$('       '$('
31      5       RESULTIS            'RESULTIS'
31      14      Identifier          'FACT_TAIL'
31      23      LParen '('          '('
31      24      Identifier          'N'
31      25      Comma ','           ','
31      27      IntLiteral          '1'
31      28      RParen ')'          ')'
32      1       RSection '$)'       '$)'
34      1       LET                 'LET'
34      5       Identifier          'START'
34      10      LParen '('          '('
34      11      RParen ')'          ')'
34      13      BE                  'BE'
34      16      LSection '$('       '$('
35      5       Identifier          'WRITES'
35      11      LParen '('          '('
35      12      StringLiteral       'Testing Factorial Functions...
'
35      46      RParen ')'          ')'
37      5       Identifier          'WRITES'
37      11      LParen '('          '('
37      12      StringLiteral       '
Iterative Factorial:
'
37      38      RParen ')'          ')'
38      5       Identifier          'WRITES'
38      11      LParen '('          '('
38      12      StringLiteral       'FACT_ITER(0) = '
38      29      RParen ')'          ')'
39      5       Identifier          'WRITEN'
39      11      LParen '('          '('
39      12      Identifier          'FACT_ITER'
39      21      LParen '('          '('
39      22      IntLiteral          '0'
39      23      RParen ')'          ')'
39      24      RParen ')'          ')'
40      5       Identifier          'NEWLINE'
40      12      LParen '('          '('
40      13      RParen ')'          ')'
42      5       Identifier          'WRITES'
42      11      LParen '('          '('
42      12      StringLiteral       'FACT_ITER(1) = '
42      29      RParen ')'          ')'
43      5       Identifier          'WRITEN'
43      11      LParen '('          '('
43      12      Identifier          'FACT_ITER'
43      21      LParen '('          '('
43      22      IntLiteral          '1'
43      23      RParen ')'          ')'
43      24      RParen ')'          ')'
44      5       Identifier          'NEWLINE'
44      12      LParen '('          '('
44      13      RParen ')'          ')'
46      5       Identifier          'WRITES'
46      11      LParen '('          '('
46      12      StringLiteral       'FACT_ITER(5) = '
46      29      RParen ')'          ')'
47      5       Identifier          'WRITEN'
47      11      LParen '('          '('
47      12      Identifier          'FACT_ITER'
47      21      LParen '('          '('
47      22      IntLiteral          '5'
47      23      RParen ')'          ')'
47      24      RParen ')'          ')'
48      5       Identifier          'NEWLINE'
48      12      LParen '('          '('
48      13      RParen ')'          ')'
50      5       Identifier          'WRITES'
50      11      LParen '('          '('
50      12      StringLiteral       '
Recursive Factorial:
'
50      38      RParen ')'          ')'
51      5       Identifier          'WRITES'
51      11      LParen '('          '('
51      12      StringLiteral       'FACT_REC(0) = '
51      28      RParen ')'          ')'
52      5       Identifier          'WRITEN'
52      11      LParen '('          '('
52      12      Identifier          'FACT_REC'
52      20      LParen '('          '('
52      21      IntLiteral          '0'
52      22      RParen ')'          ')'
52      23      RParen ')'          ')'
53      5       Identifier          'NEWLINE'
53      12      LParen '('          '('
53      13      RParen ')'          ')'
55      5       Identifier          'WRITES'
55      11      LParen '('          '('
55      12      StringLiteral       'FACT_REC(1) = '
55      28      RParen ')'          ')'
56      5       Identifier          'WRITEN'
56      11      LParen '('          '('
56      12      Identifier          'FACT_REC'
56      20      LParen '('          '('
56      21      IntLiteral          '1'
56      22      RParen ')'          ')'
56      23      RParen ')'          ')'
57      5       Identifier          'NEWLINE'
57      12      LParen '('          '('
57      13      RParen ')'          ')'
59      5       Identifier          'WRITES'
59      11      LParen '('          '('
59      12      StringLiteral       'FACT_REC(5) = '
59      28      RParen ')'          ')'
60      5       Identifier          'WRITEN'
60      11      LParen '('          '('
60      12      Identifier          'FACT_REC'
60      20      LParen '('          '('
60      21      IntLiteral          '5'
60      22      RParen ')'          ')'
60      23      RParen ')'          ')'
61      5       Identifier          'NEWLINE'
61      12      LParen '('          '('
61      13      RParen ')'          ')'
63      5       Identifier          'WRITES'
63      11      LParen '('          '('
63      12      StringLiteral       '
Tail-Recursive Factorial:
'
63      43      RParen ')'          ')'
64      5       Identifier          'WRITES'
64      11      LParen '('          '('
64      12      StringLiteral       'FACT_TCO(0) = '
64      28      RParen ')'          ')'
65      5       Identifier          'WRITEN'
65      11      LParen '('          '('
65      12      Identifier          'FACT_TCO'
65      20      LParen '('          '('
65      21      IntLiteral          '0'
65      22      RParen ')'          ')'
65      23      RParen ')'          ')'
66      5       Identifier          'NEWLINE'
66      12      LParen '('          '('
66      13      RParen ')'          ')'
68      5       Identifier          'WRITES'
68      11      LParen '('          '('
68      12      StringLiteral       'FACT_TCO(1) = '
68      28      RParen ')'          ')'
69      5       Identifier          'WRITEN'
69      11      LParen '('          '('
69      12      Identifier          'FACT_TCO'
69      20      LParen '('          '('
69      21      IntLiteral          '1'
69      22      RParen ')'          ')'
69      23      RParen ')'          ')'
70      5       Identifier          'NEWLINE'
70      12      LParen '('          '('
70      13      RParen ')'          ')'
72      5       Identifier          'WRITES'
72      11      LParen '('          '('
72      12      StringLiteral       'FACT_TCO(5) = '
72      28      RParen ')'          ')'
73      5       Identifier          'WRITEN'
73      11      LParen '('          '('
73      12      Identifier          'FACT_TCO'
73      20      LParen '('          '('
73      21      IntLiteral          '5'
73      22      RParen ')'          ')'
73      23      RParen ')'          ')'
74      5       Identifier          'NEWLINE'
74      12      LParen '('          '('
74      13      RParen ')'          ')'
76      5       Identifier          'WRITES'
76      11      LParen '('          '('
76      12      StringLiteral       '
Factorial Tests Finished.
'
76      43      RParen ')'          ')'
77      5       FINISH              'FINISH'
78      1       RSection '$)'       '$)'
79      1       EOF                 ''
-----------------------------------------------------


--- ABSTRACT SYNTAX TREE ---
Program
|  FunctionDecl FACT_ITER(N)
|  |  Valof
|  |  |  Body:
|  |  |  |  CompoundStatement
|  |  |  |  |  LetDecl
|  |  |  |  |  |  Var RESULT
|  |  |  |  |  |  |  IntLiteral: 1
|  |  |  |  |  ForStatement (Var: I)
|  |  |  |  |  |  From:
|  |  |  |  |  |  |  IntLiteral: 1
|  |  |  |  |  |  To:
|  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  Body:
|  |  |  |  |  |  |  CompoundStatement
|  |  |  |  |  |  |  |  Assignment
|  |  |  |  |  |  |  |  |  LHS:
|  |  |  |  |  |  |  |  |  |  Variable: RESULT
|  |  |  |  |  |  |  |  |  RHS:
|  |  |  |  |  |  |  |  |  |  BinaryOp: Op '*'
|  |  |  |  |  |  |  |  |  |  |  Variable: RESULT
|  |  |  |  |  |  |  |  |  |  |  Variable: I
|  |  |  |  |  ResultisStatement
|  |  |  |  |  |  Value:
|  |  |  |  |  |  |  Variable: RESULT
|  FunctionDecl FACT_REC(N)
|  |  Valof
|  |  |  Body:
|  |  |  |  CompoundStatement
|  |  |  |  |  IfStatement
|  |  |  |  |  |  Condition:
|  |  |  |  |  |  |  BinaryOp: Op '='
|  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  IntLiteral: 0
|  |  |  |  |  |  Then:
|  |  |  |  |  |  |  ResultisStatement
|  |  |  |  |  |  |  |  Value:
|  |  |  |  |  |  |  |  |  IntLiteral: 1
|  |  |  |  |  ResultisStatement
|  |  |  |  |  |  Value:
|  |  |  |  |  |  |  BinaryOp: Op '*'
|  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  |  |  Variable: FACT_REC
|  |  |  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  |  |  BinaryOp: Op '-'
|  |  |  |  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  |  |  |  IntLiteral: 1
|  FunctionDecl FACT_TAIL(N, ACCUMULATOR)
|  |  Valof
|  |  |  Body:
|  |  |  |  CompoundStatement
|  |  |  |  |  IfStatement
|  |  |  |  |  |  Condition:
|  |  |  |  |  |  |  BinaryOp: Op '='
|  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  IntLiteral: 0
|  |  |  |  |  |  Then:
|  |  |  |  |  |  |  ResultisStatement
|  |  |  |  |  |  |  |  Value:
|  |  |  |  |  |  |  |  |  Variable: ACCUMULATOR
|  |  |  |  |  ResultisStatement
|  |  |  |  |  |  Value:
|  |  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  |  Variable: FACT_TAIL
|  |  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  |  BinaryOp: Op '-'
|  |  |  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  |  |  IntLiteral: 1
|  |  |  |  |  |  |  |  |  BinaryOp: Op '*'
|  |  |  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  |  |  Variable: ACCUMULATOR
|  FunctionDecl FACT_TCO(N)
|  |  Valof
|  |  |  Body:
|  |  |  |  CompoundStatement
|  |  |  |  |  ResultisStatement
|  |  |  |  |  |  Value:
|  |  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  |  Variable: FACT_TAIL
|  |  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  |  Variable: N
|  |  |  |  |  |  |  |  |  IntLiteral: 1
|  RoutineDecl START()
|  |  CompoundStatement
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "Testing Factorial Functions...
"
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "
Iterative Factorial:
"
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_ITER(0) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_ITER
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 0
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_ITER(1) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_ITER
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 1
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_ITER(5) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_ITER
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 5
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "
Recursive Factorial:
"
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_REC(0) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_REC
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 0
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_REC(1) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_REC
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 1
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_REC(5) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_REC
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 5
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "
Tail-Recursive Factorial:
"
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_TCO(0) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_TCO
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 0
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_TCO(1) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_TCO
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 1
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "FACT_TCO(5) = "
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITEN
|  |  |  |  |  Arguments:
|  |  |  |  |  |  FunctionCall
|  |  |  |  |  |  |  Function:
|  |  |  |  |  |  |  |  Variable: FACT_TCO
|  |  |  |  |  |  |  Arguments:
|  |  |  |  |  |  |  |  IntLiteral: 5
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: NEWLINE
|  |  |  RoutineCall
|  |  |  |  FunctionCall
|  |  |  |  |  Function:
|  |  |  |  |  |  Variable: WRITES
|  |  |  |  |  Arguments:
|  |  |  |  |  |  StringLiteral: "
Factorial Tests Finished.
"
|  |  |  FinishStatement
---------------------------


Generating code...
Visiting function declaration: FACT_ITER
Generated return label: return_0
Visiting function declaration: FACT_REC
Generated return label: return_3
Visiting function declaration: FACT_TAIL
Generated return label: return_5
Visiting function declaration: FACT_TCO
Generated return label: return_7
Visiting function declaration: START
Generated return label: return_8
Code generation complete.

=== Generated Assembly ===

;------------ Generated ARM64 Assembly ------------

.arch armv8-a
.text
.align 4

.text
.align 4

FACT_ITER:
    sub sp, sp, #48                // Allocate stack frame (placeholder)
    stp x29, x30, [sp, #32]        // Save FP/LR at top of frame (placeholder offset)
    mov x29, sp                    // Set up frame pointer
    str x0, [x29, #-8]             // Store parameter N to stack home
    ldr x27, [x29, #-8]            // Load variable N into x27
    movz x0, #0x1                  // Load number literal
    mov x26, x0                    // Initialize local RESULT in x26
    movz x0, #0x1                  // Load number literal
    mov x25, x0                    // Initialize loop var I in x25
    mov x0, x27                    // Move N from x27 to X0
    mov x15, x0                    // Move 'to' value into x15
    movz x0, #0x1                  // Loading 1 into x0
    mov x14, x0                    // Move 'by' value into x14
repeat_2:
    cmp x25, x15                  
    b.gt loop_end_1               
    mov x0, x25                    // Move I from x25 to X0
    mov x13, x0                   
    mov x0, x26                    // Move RESULT from x26 to X0
    mul x0, x0, x13                // Multiply
    mov x26, x0                    // Assign value to RESULT in x26
    add x25, x25, x14              // Increment I
    b repeat_2                    
loop_end_1:
    mov x0, x26                    // Move RESULT from x26 to X0
return_0:
    str x25, [x29, #-24]           // Spill I from x25 to stack
    str x27, [x29, #-8]            // Spill N from x27 to stack
    ldp x29, x30, [sp, #32]        // Restore FP/LR
    add sp, sp, #48                // Deallocate stack frame
    ret                            // Return from function
FACT_REC:
    sub sp, sp, #80                // Allocate stack frame (placeholder)
    stp x29, x30, [sp, #64]        // Save FP/LR at top of frame (placeholder offset)
    mov x29, sp                    // Set up frame pointer
    str x25, [x29, #-32]           // Save callee-saved register x25
    str x27, [x29, #-40]           // Save callee-saved register x27
    str x0, [x29, #-48]            // Store parameter N to stack home
    ldr x27, [x29, #-48]           // Load variable N into x27
    movz x0, #0x0                  // Load number literal
    mov x14, x0                   
    mov x0, x27                    // Move N from x27 to X0
    cmp x0, x14                   
    cset x0, eq                   
    neg x0, x0                    
    cbz x0, if_end_4               // Branch if condition is false
    movz x0, #0x1                  // Load number literal
if_end_4:
    str x27, [x29, #-8]            // Spill N from x27 to stack
    str x25, [x29, #-56]           // Save caller-saved register x25
    str x27, [x29, #-64]           // Save caller-saved register x27
    movz x0, #0x1                  // Load number literal
    mov x14, x0                   
    ldr x26, [x29, #-48]           // Load variable N into x26
    mov x0, x26                    // Move N from x26 to X0
    sub x0, x0, x14                // Subtract
    bl FACT_REC                    // Call FACT_REC
    ldr x27, [x29, #-64]           // Restore caller-saved register x27
    ldr x25, [x29, #-56]           // Restore caller-saved register x25
    mov x14, x0                   
    mov x0, x27                    // Move N from x27 to X0
    mul x0, x0, x14                // Multiply
return_3:
    ldr x27, [x29, #-40]           // Restore callee-saved register x27
    ldr x25, [x29, #-32]           // Restore callee-saved register x25
    ldp x29, x30, [sp, #64]        // Restore FP/LR
    add sp, sp, #80                // Deallocate stack frame
    ret                            // Return from function
FACT_TAIL:
    sub sp, sp, #96                // Allocate stack frame (placeholder)
    stp x29, x30, [sp, #80]        // Save FP/LR at top of frame (placeholder offset)
    mov x29, sp                    // Set up frame pointer
    str x25, [x29, #-40]           // Save callee-saved register x25
    str x27, [x29, #-48]           // Save callee-saved register x27
    str x0, [x29, #-56]            // Store parameter N to stack home
    ldr x27, [x29, #-56]           // Load variable N into x27
    str x1, [x29, #-64]            // Store parameter ACCUMULATOR to stack home
    ldr x26, [x29, #-64]           // Load variable ACCUMULATOR into x26
    movz x0, #0x0                  // Load number literal
    mov x14, x0                   
    mov x0, x27                    // Move N from x27 to X0
    cmp x0, x14                   
    cset x0, eq                   
    neg x0, x0                    
    cbz x0, if_end_6               // Branch if condition is false
    mov x0, x26                    // Move ACCUMULATOR from x26 to X0
if_end_6:
    movz x0, #0x1                  // Load number literal
    mov x14, x0                   
    mov x0, x27                    // Move N from x27 to X0
    sub x0, x0, x14                // Subtract
    mov x14, x0                   
    ldr x26, [x29, #-64]           // Load variable ACCUMULATOR into x26
    mov x0, x26                    // Move ACCUMULATOR from x26 to X0
    mov x15, x0                   
    mov x0, x27                    // Move N from x27 to X0
    mul x0, x0, x15                // Multiply
    mov x1, x0                    
    mov x0, x14                   
    b FACT_TAIL                    // Tail call optimization
return_5:
    str x27, [x29, #-64]           // Spill N from x27 to stack
    ldr x27, [x29, #-48]           // Restore callee-saved register x27
    ldr x25, [x29, #-40]           // Restore callee-saved register x25
    ldp x29, x30, [sp, #80]        // Restore FP/LR
    add sp, sp, #96                // Deallocate stack frame
    ret                            // Return from function
FACT_TCO:
    sub sp, sp, #128               // Allocate stack frame (placeholder)
    stp x29, x30, [sp, #112]       // Save FP/LR at top of frame (placeholder offset)
    mov x29, sp                    // Set up frame pointer
    str x25, [x29, #-56]           // Save callee-saved register x25
    str x26, [x29, #-64]           // Save callee-saved register x26
    str x27, [x29, #-72]           // Save callee-saved register x27
    str x0, [x29, #-80]            // Store parameter N to stack home
    ldr x27, [x29, #-80]           // Load variable N into x27
    str x27, [x29, #-64]           // Spill N from x27 to stack
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    ldr x24, [x29, #-80]           // Load variable N into x24
    mov x0, x24                    // Move N from x24 to X0
    movz x0, #0x1                  // Load number literal
    mov x1, x0                     // Move arg 1 to X1
    bl FACT_TAIL                   // Call FACT_TAIL
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
return_7:
    ldr x27, [x29, #-72]           // Restore callee-saved register x27
    ldr x26, [x29, #-64]           // Restore callee-saved register x26
    ldr x25, [x29, #-56]           // Restore callee-saved register x25
    ldp x29, x30, [sp, #112]       // Restore FP/LR
    add sp, sp, #128               // Deallocate stack frame
    ret                            // Return from function
START:
    sub sp, sp, #128               // Allocate stack frame (placeholder)
    stp x29, x30, [sp, #112]       // Save FP/LR at top of frame (placeholder offset)
    mov x29, sp                    // Set up frame pointer
    str x25, [x29, #-64]           // Save callee-saved register x25
    str x26, [x29, #-72]           // Save callee-saved register x26
    str x27, [x29, #-80]           // Save callee-saved register x27
    adr x0, .L.str0                // Load string literal address
    bl writes                      // Call writes
    adr x0, .L.str1                // Load string literal address
    bl writes                      // Call writes
    adr x0, .L.str2                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x0                  // Load number literal
    bl FACT_ITER                   // Call FACT_ITER
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str3                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x1                  // Load number literal
    bl FACT_ITER                   // Call FACT_ITER
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str4                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x5                  // Load number literal
    bl FACT_ITER                   // Call FACT_ITER
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str5                // Load string literal address
    bl writes                      // Call writes
    adr x0, .L.str6                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x0                  // Load number literal
    bl FACT_REC                    // Call FACT_REC
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str7                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x1                  // Load number literal
    bl FACT_REC                    // Call FACT_REC
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str8                // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x5                  // Load number literal
    bl FACT_REC                    // Call FACT_REC
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str9                // Load string literal address
    bl writes                      // Call writes
    adr x0, .L.str10               // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x0                  // Load number literal
    bl FACT_TCO                    // Call FACT_TCO
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str11               // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x1                  // Load number literal
    bl FACT_TCO                    // Call FACT_TCO
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str12               // Load string literal address
    bl writes                      // Call writes
    str x25, [x29, #-88]           // Save caller-saved register x25
    str x26, [x29, #-96]           // Save caller-saved register x26
    str x27, [x29, #-104]          // Save caller-saved register x27
    movz x0, #0x5                  // Load number literal
    bl FACT_TCO                    // Call FACT_TCO
    ldr x27, [x29, #-104]          // Restore caller-saved register x27
    ldr x26, [x29, #-96]           // Restore caller-saved register x26
    ldr x25, [x29, #-88]           // Restore caller-saved register x25
    bl writen                      // Call writen
    bl newline                     // Call newline
    adr x0, .L.str13               // Load string literal address
    bl writes                      // Call writes
    b return_8                     // Finish current construct
return_8:
    ldr x27, [x29, #-80]           // Restore callee-saved register x27
    ldr x26, [x29, #-72]           // Restore callee-saved register x26
    ldr x25, [x29, #-64]           // Restore callee-saved register x25
    ldp x29, x30, [sp, #112]       // Restore FP/LR
    add sp, sp, #128               // Deallocate stack frame
    ret                            // Return from function

.data
.L.str0:
    .string "Testing Factorial Functions...
"
.L.str1:
    .string "
Iterative Factorial:
"
.L.str2:
    .string "FACT_ITER(0) = "
.L.str3:
    .string "FACT_ITER(1) = "
.L.str4:
    .string "FACT_ITER(5) = "
.L.str5:
    .string "
Recursive Factorial:
"
.L.str6:
    .string "FACT_REC(0) = "
.L.str7:
    .string "FACT_REC(1) = "
.L.str8:
    .string "FACT_REC(5) = "
.L.str9:
    .string "
Tail-Recursive Factorial:
"
.L.str10:
    .string "FACT_TCO(0) = "
.L.str11:
    .string "FACT_TCO(1) = "
.L.str12:
    .string "FACT_TCO(5) = "
.L.str13:
    .string "
Factorial Tests Finished.
"

;------------ End of Assembly ------------


Compilation successful.


```
    
