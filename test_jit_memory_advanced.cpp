#include "JITMemoryManager.h"
#include <iostream>
#include <vector>
#include <chrono>

/**
 * @brief Stress test with multiple memory managers
 */
void stressTestMultipleAllocations() {
    std::cout << "=== Stress Test: Multiple Allocations ===\n\n";
    
    const size_t num_managers = 10;
    const size_t allocation_size = 4096;
    
    std::vector<std::unique_ptr<JITMemoryManager>> managers;
    
    try {
        // Allocate multiple memory regions
        for (size_t i = 0; i < num_managers; ++i) {
            auto manager = std::make_unique<JITMemoryManager>(allocation_size);
            std::cout << "Manager " << i << ": allocated " << manager->getSize() 
                     << " bytes at " << manager->getMemoryPointer() << "\n";
            managers.push_back(std::move(manager));
        }
        
        std::cout << "\nAll " << num_managers << " allocations successful!\n";
        
        // Test making them all executable
        for (size_t i = 0; i < managers.size(); ++i) {
            managers[i]->makeExecutable();
            std::cout << "Manager " << i << ": made executable\n";
        }
        
        std::cout << "\nAll memory regions made executable successfully!\n";
        
        // Test making them writable again
        for (size_t i = 0; i < managers.size(); ++i) {
            managers[i]->makeWritable();
            std::cout << "Manager " << i << ": made writable\n";
        }
        
        std::cout << "\nAll memory regions made writable successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in stress test: " << e.what() << "\n";
    }
    
    // Managers will be automatically cleaned up
    std::cout << "\nStress test completed - all memory will be cleaned up automatically\n\n";
}

/**
 * @brief Test performance of allocation and deallocation
 */
void performanceTest() {
    std::cout << "=== Performance Test ===\n\n";
    
    const size_t num_iterations = 1000;
    const size_t allocation_size = 4096;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_iterations; ++i) {
        JITMemoryManager manager(allocation_size);
        manager.makeExecutable();
        manager.makeWritable();
        // Destructor will deallocate automatically
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Completed " << num_iterations << " allocation/deallocation cycles\n";
    std::cout << "Total time: " << duration.count() << " microseconds\n";
    std::cout << "Average time per cycle: " << (duration.count() / num_iterations) << " microseconds\n\n";
}

/**
 * @brief Test large allocation sizes
 */
void testLargeAllocations() {
    std::cout << "=== Large Allocation Test ===\n\n";
    
    std::vector<size_t> sizes = {
        1024 * 1024,     // 1 MB
        10 * 1024 * 1024, // 10 MB
        100 * 1024 * 1024 // 100 MB
    };
    
    for (size_t size : sizes) {
        try {
            std::cout << "Testing allocation of " << (size / (1024 * 1024)) << " MB...\n";
            
            JITMemoryManager manager(size);
            std::cout << "  Allocated: " << manager.getSize() << " bytes\n";
            std::cout << "  Address: " << manager.getMemoryPointer() << "\n";
            
            manager.makeExecutable();
            std::cout << "  Made executable successfully\n";
            
            manager.makeWritable();
            std::cout << "  Made writable successfully\n\n";
            
        } catch (const std::exception& e) {
            std::cerr << "  Failed: " << e.what() << "\n\n";
        }
    }
}

int main() {
    std::cout << "JITMemoryManager Advanced Testing\n";
    std::cout << "==================================\n\n";
    
    stressTestMultipleAllocations();
    performanceTest();
    testLargeAllocations();
    
    std::cout << "=== All advanced tests completed! ===\n";
    
    return 0;
}