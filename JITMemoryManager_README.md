# JITMemoryManager Documentation

## Overview

The `JITMemoryManager` class provides cross-platform memory management for JIT code generation in the BCPL JIT compiler. It implements the W^X (Write XOR Execute) security model and provides platform-specific memory allocation using `mmap` on Unix-like systems and `VirtualAlloc` on Windows.

## Key Features

- **Cross-Platform Support**: Works on Linux, macOS, and Windows
- **W^X Security**: Enforces Write XOR Execute security model
- **RAII Resource Management**: Automatic cleanup with move semantics
- **Page-Aligned Allocation**: Optimal performance with system page alignment
- **Comprehensive Error Handling**: Custom exception class with detailed error messages
- **Performance Optimized**: Fast allocation/deallocation suitable for JIT scenarios

## Basic Usage

### Simple Allocation and Execution

```cpp
#include "JITMemoryManager.h"

// Create a memory manager and allocate memory
JITMemoryManager manager;
void* memory = manager.allocate(4096);  // Allocate 4KB

// Write your generated machine code to the memory
// (code generation happens here)
memcpy(memory, machine_code, code_size);

// Make the memory executable (W^X security)
manager.makeExecutable();

// Execute the generated code
typedef int (*GeneratedFunction)();
GeneratedFunction func = reinterpret_cast<GeneratedFunction>(memory);
int result = func();

// Memory is automatically deallocated when manager goes out of scope
```

### Constructor with Immediate Allocation

```cpp
// Allocate memory immediately during construction
JITMemoryManager manager(8192);  // Allocate 8KB

// Memory is ready to use
void* memory = manager.getMemoryPointer();
```

### Permission Management

```cpp
JITMemoryManager manager(4096);

// Initially memory has read/write permissions
// Write your code here...

// Change to read/execute permissions (W^X security)
manager.makeExecutable();

// Execute your code here...

// Change back to read/write for modification
manager.makeWritable();

// Modify your code here...
```

## API Reference

### Constructors

- `JITMemoryManager()` - Default constructor (no memory allocated)
- `JITMemoryManager(size_t size)` - Constructor with immediate allocation

### Memory Management

- `void* allocate(size_t size)` - Allocate memory for code generation
- `void deallocate()` - Deallocate memory (called automatically by destructor)

### Permission Control

- `void makeExecutable()` - Change permissions to read/execute (removes write)
- `void makeWritable()` - Change permissions to read/write (removes execute)

### Query Methods

- `void* getMemoryPointer() const` - Get pointer to allocated memory
- `size_t getSize() const` - Get size of allocated memory
- `bool isAllocated() const` - Check if memory is allocated
- `bool isExecutable() const` - Check if memory has execute permissions

### Static Utilities

- `static size_t getPageSize()` - Get system page size
- `static size_t roundToPageSize(size_t size)` - Round size to page boundary

## Error Handling

The `JITMemoryManager` throws `JITMemoryManagerException` for all error conditions:

```cpp
try {
    JITMemoryManager manager;
    manager.allocate(0);  // Will throw: "Cannot allocate zero bytes"
} catch (const JITMemoryManagerException& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

Common error conditions:
- Attempting to allocate zero bytes
- Double allocation without deallocation
- Operations on unallocated memory
- System-level allocation failures
- Permission change failures

## Move Semantics

The class supports efficient move operations:

```cpp
JITMemoryManager manager1(4096);
JITMemoryManager manager2 = std::move(manager1);  // Move constructor

JITMemoryManager manager3;
manager3 = std::move(manager2);  // Move assignment
```

After a move operation, the source object has no allocated memory and the destination object owns the memory.

## Platform-Specific Details

### Linux/macOS (Unix-like systems)
- Uses `mmap()` with `MAP_PRIVATE | MAP_ANONYMOUS` flags
- Uses `mprotect()` for permission changes
- Uses `munmap()` for deallocation

### Windows
- Uses `VirtualAlloc()` with `MEM_COMMIT | MEM_RESERVE` flags
- Uses `VirtualProtect()` for permission changes  
- Uses `VirtualFree()` with `MEM_RELEASE` for deallocation

## Integration with BCPL JIT

The `JITMemoryManager` is designed to be used by the BCPL JIT compiler for:

1. **Code Generation Buffer**: Allocate memory for generated machine code
2. **W^X Security**: Enforce security by making memory executable only after code generation
3. **Memory Pool Management**: Manage multiple code regions efficiently
4. **Runtime Code Patching**: Switch between writable and executable as needed

## Performance Characteristics

- **Allocation**: ~10 microseconds average (tested with 1000 cycles)
- **Permission Changes**: Very fast (system call overhead only)
- **Large Allocations**: Supports up to 100MB+ allocations efficiently
- **Multiple Regions**: Can manage multiple simultaneous allocations

## Testing

Two test programs are provided:

1. **`test_jit_memory_manager`**: Basic functionality tests including actual code execution
2. **`test_jit_memory_advanced`**: Stress tests, performance tests, and large allocation tests

Build and run tests:
```bash
make test_jit_memory_manager test_jit_memory_advanced
./test_jit_memory_manager
./test_jit_memory_advanced
```

## Thread Safety

The `JITMemoryManager` class is **not thread-safe**. Each thread should use its own instance, or external synchronization should be provided when sharing instances between threads.

## Example Output

When running the test program, you should see output similar to:

```
=== Basic JITMemoryManager Usage Demo ===

1. Memory manager created (no memory allocated yet)
   Allocated: No
   Size: 0 bytes

2. Memory allocated
   Requested size: 6 bytes
   Actual size: 4096 bytes (page-aligned)
   Memory address: 0x7f14bec40000
   Executable: No

3. Machine code copied to memory
   Code bytes: b8 2a 00 00 00 c3

4. Memory permissions changed to executable
   Executable: Yes

5. Generated function executed
   Result: 42

=== All tests completed successfully! ===
```

This demonstrates successful allocation, code execution, and proper memory management.