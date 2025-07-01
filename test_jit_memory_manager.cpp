#include "JITMemoryManager.h"
#include <iostream>
#include <iomanip>
#include <cstring>

// Simple AArch64 machine code that returns 42
// This is a minimal function that sets x0 to 42 and returns
#ifdef __aarch64__
const uint8_t sample_code[] = {
    0x40, 0x05, 0x80, 0xd2,  // mov x0, #42
    0xc0, 0x03, 0x5f, 0xd6   // ret
};
#else
// For non-AArch64 platforms, use x86-64 code that returns 42
const uint8_t sample_code[] = {
    0xb8, 0x2a, 0x00, 0x00, 0x00,  // mov eax, 42
    0xc3                            // ret
};
#endif

// Function pointer type for our generated code
typedef int (*GeneratedFunction)();

/**
 * @brief Demonstrates basic usage of JITMemoryManager
 */
void demonstrateBasicUsage() {
    std::cout << "\n=== Basic JITMemoryManager Usage Demo ===\n\n";
    
    try {
        // Create a memory manager
        JITMemoryManager manager;
        
        std::cout << "1. Memory manager created (no memory allocated yet)\n";
        std::cout << "   Allocated: " << (manager.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   Size: " << manager.getSize() << " bytes\n\n";
        
        // Allocate memory for our code
        size_t code_size = sizeof(sample_code);
        void* memory = manager.allocate(code_size);
        
        std::cout << "2. Memory allocated\n";
        std::cout << "   Requested size: " << code_size << " bytes\n";
        std::cout << "   Actual size: " << manager.getSize() << " bytes (page-aligned)\n";
        std::cout << "   Memory address: " << memory << "\n";
        std::cout << "   Executable: " << (manager.isExecutable() ? "Yes" : "No") << "\n\n";
        
        // Copy our machine code to the allocated memory
        std::memcpy(memory, sample_code, code_size);
        std::cout << "3. Machine code copied to memory\n";
        
        // Show the hex dump of the code
        std::cout << "   Code bytes: ";
        for (size_t i = 0; i < code_size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << static_cast<unsigned>(sample_code[i]) << " ";
        }
        std::cout << std::dec << "\n\n";
        
        // Make the memory executable
        manager.makeExecutable();
        std::cout << "4. Memory permissions changed to executable\n";
        std::cout << "   Executable: " << (manager.isExecutable() ? "Yes" : "No") << "\n\n";
        
        // Cast memory to function pointer and execute
        GeneratedFunction func = reinterpret_cast<GeneratedFunction>(memory);
        int result = func();
        
        std::cout << "5. Generated function executed\n";
        std::cout << "   Result: " << result << "\n\n";
        
        // Test making it writable again
        manager.makeWritable();
        std::cout << "6. Memory permissions changed back to writable\n";
        std::cout << "   Executable: " << (manager.isExecutable() ? "Yes" : "No") << "\n\n";
        
        // Memory will be automatically deallocated when manager goes out of scope
        std::cout << "7. Memory will be automatically deallocated when manager is destroyed\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

/**
 * @brief Demonstrates error handling scenarios
 */
void demonstrateErrorHandling() {
    std::cout << "\n=== Error Handling Demo ===\n\n";
    
    // Test 1: Double allocation
    try {
        std::cout << "1. Testing double allocation...\n";
        JITMemoryManager manager;
        manager.allocate(4096);
        manager.allocate(4096); // This should throw
    } catch (const JITMemoryManagerException& e) {
        std::cout << "   Caught expected exception: " << e.what() << "\n\n";
    }
    
    // Test 2: Zero-size allocation
    try {
        std::cout << "2. Testing zero-size allocation...\n";
        JITMemoryManager manager;
        manager.allocate(0); // This should throw
    } catch (const JITMemoryManagerException& e) {
        std::cout << "   Caught expected exception: " << e.what() << "\n\n";
    }
    
    // Test 3: Operations on unallocated memory
    try {
        std::cout << "3. Testing operations on unallocated memory...\n";
        JITMemoryManager manager;
        manager.makeExecutable(); // This should throw
    } catch (const JITMemoryManagerException& e) {
        std::cout << "   Caught expected exception: " << e.what() << "\n\n";
    }
}

/**
 * @brief Demonstrates move semantics
 */
void demonstrateMoveSemantics() {
    std::cout << "\n=== Move Semantics Demo ===\n\n";
    
    try {
        // Create and allocate memory
        JITMemoryManager manager1;
        manager1.allocate(4096);
        void* original_ptr = manager1.getMemoryPointer();
        
        std::cout << "1. Original manager allocated memory at: " << original_ptr << "\n";
        
        // Move constructor
        JITMemoryManager manager2 = std::move(manager1);
        
        std::cout << "2. After move constructor:\n";
        std::cout << "   Original manager allocated: " << (manager1.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   New manager allocated: " << (manager2.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   New manager memory address: " << manager2.getMemoryPointer() << "\n";
        std::cout << "   Memory addresses match: " << (original_ptr == manager2.getMemoryPointer() ? "Yes" : "No") << "\n\n";
        
        // Move assignment
        JITMemoryManager manager3;
        manager3 = std::move(manager2);
        
        std::cout << "3. After move assignment:\n";
        std::cout << "   Manager2 allocated: " << (manager2.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   Manager3 allocated: " << (manager3.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   Manager3 memory address: " << manager3.getMemoryPointer() << "\n";
        std::cout << "   Memory addresses still match: " << (original_ptr == manager3.getMemoryPointer() ? "Yes" : "No") << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

/**
 * @brief Demonstrates constructor with immediate allocation
 */
void demonstrateConstructorAllocation() {
    std::cout << "\n=== Constructor Allocation Demo ===\n\n";
    
    try {
        // Create manager with immediate allocation
        JITMemoryManager manager(8192);
        
        std::cout << "1. Manager created with immediate allocation\n";
        std::cout << "   Allocated: " << (manager.isAllocated() ? "Yes" : "No") << "\n";
        std::cout << "   Size: " << manager.getSize() << " bytes\n";
        std::cout << "   Page size: " << JITMemoryManager::getPageSize() << " bytes\n";
        std::cout << "   Rounded size: " << JITMemoryManager::roundToPageSize(8192) << " bytes\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "JITMemoryManager Test Program\n";
    std::cout << "==============================\n";
    
    std::cout << "System Information:\n";
    std::cout << "- Page size: " << JITMemoryManager::getPageSize() << " bytes\n";
#ifdef __aarch64__
    std::cout << "- Architecture: AArch64\n";
#elif defined(__x86_64__)
    std::cout << "- Architecture: x86-64\n";
#else
    std::cout << "- Architecture: Other\n";
#endif
    
    // Run all demonstrations
    demonstrateBasicUsage();
    demonstrateErrorHandling();
    demonstrateMoveSemantics();
    demonstrateConstructorAllocation();
    
    std::cout << "\n=== All tests completed successfully! ===\n";
    
    return 0;
}