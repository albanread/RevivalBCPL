# Adding New Optimization Passes

The BCPL optimizer has been refactored to use a modular pass-based architecture. This document explains how to add new optimization passes to the system.

## Architecture Overview

The optimizer uses the following components:

- **OptimizationPass**: Base interface that all optimization passes must implement
- **PassManager**: Coordinates and sequences multiple optimization passes  
- **Optimizer**: Main interface that uses PassManager to apply optimizations

## Current Passes

The system currently includes these optimization passes:

1. **ConstantFoldingPass**: Performs constant folding and algebraic simplifications
   - Evaluates constant expressions at compile time (e.g., `2 + 3` → `5`)
   - Applies algebraic identities (e.g., `x * 1` → `x`, `x + 0` → `x`)
   - Optimizes conditional expressions with constant conditions
   - Performs strength reduction (e.g., `x * 2` → `x << 1`)

2. **LoopInvariantCodeMotionPass**: Moves loop-invariant code outside loops
   - Identifies expressions that don't depend on loop variables
   - Hoists invariant computations out of loop bodies
   - Reduces redundant computation in loops

## Adding a New Pass

To add a new optimization pass, follow these steps:

### 1. Create the Pass Header File

Create a new header file (e.g., `MyOptimizationPass.h`):

```cpp
#ifndef MY_OPTIMIZATION_PASS_H
#define MY_OPTIMIZATION_PASS_H

#include "OptimizationPass.h"
#include "AST.h"

class MyOptimizationPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    // Add any helper methods or visitor pattern methods here
    // Follow the pattern used in ConstantFoldingPass.h
};

#endif // MY_OPTIMIZATION_PASS_H
```

### 2. Implement the Pass

Create the implementation file (e.g., `MyOptimizationPass.cpp`):

```cpp
#include "MyOptimizationPass.h"

ProgramPtr MyOptimizationPass::apply(ProgramPtr program) {
    // Implement your optimization logic here
    // You can use the visitor pattern like other passes
    return program; // Return the optimized program
}

std::string MyOptimizationPass::getName() const {
    return "My Optimization Pass";
}
```

### 3. Update CMakeLists.txt

Add your new source file to the executable in `CMakeLists.txt`:

```cmake
add_executable(compiler
    # ... existing files ...
    MyOptimizationPass.cpp
    # ... rest of files ...
)
```

### 4. Register the Pass

Add your pass to the default pass pipeline in `Optimizer.cpp`:

```cpp
#include "MyOptimizationPass.h"

void Optimizer::setupDefaultPasses() {
    // Register the default optimization passes
    passManager.addPass(std::make_unique<ConstantFoldingPass>(manifests));
    passManager.addPass(std::make_unique<LoopInvariantCodeMotionPass>(manifests));
    passManager.addPass(std::make_unique<MyOptimizationPass>()); // Add your pass
}
```

### 5. Build and Test

```bash
make clean && make compiler
./compiler your_test_file.b
```

## Pass Design Guidelines

### Visitor Pattern

Most passes use the visitor pattern to traverse and transform the AST. Follow this structure:

```cpp
class MyPass : public OptimizationPass {
private:
    // Visitor dispatchers
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    
    // Specific node visitors
    ExprPtr visit(BinaryOp* node);
    StmtPtr visit(ForStatement* node);
    // ... etc for each AST node type
};
```

### State Management

If your pass needs to maintain state:

```cpp
class MyPass : public OptimizationPass {
private:
    // Pass-specific state
    std::unordered_map<std::string, Value> symbolTable;
    bool inLoopContext = false;
    
    // Reset state for each program
    void resetState();
};
```

### Manifest Constants

Access manifest constants through the manifests parameter:

```cpp
class MyPass : public OptimizationPass {
public:
    MyPass(std::unordered_map<std::string, int64_t>& manifests) 
        : manifests(manifests) {}

private:
    std::unordered_map<std::string, int64_t>& manifests;
};
```

## Best Practices

1. **Keep passes focused**: Each pass should have a single optimization goal
2. **Preserve semantics**: Never change the meaning of the program
3. **Handle all node types**: Ensure your visitor methods handle all AST node types
4. **Test thoroughly**: Verify your optimizations don't break existing functionality
5. **Document behavior**: Clearly document what optimizations your pass performs

## Example: Dead Code Elimination Pass

Here's a skeleton for a dead code elimination pass:

```cpp
class DeadCodeEliminationPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override {
        return visit(program.get());
    }
    
    std::string getName() const override {
        return "Dead Code Elimination Pass";
    }

private:
    std::set<std::string> usedVariables;
    
    // First pass: collect used variables
    void collectUsedVariables(Program* program);
    
    // Second pass: remove unused declarations
    ProgramPtr visit(Program* node);
    DeclPtr visit(LetDeclaration* node);
    // ... other visitors
};
```

This modular architecture makes it easy to add new optimizations while keeping the codebase maintainable and the optimization pipeline flexible.