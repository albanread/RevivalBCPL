#ifndef JIT_MEMORY_MANAGER_H
#define JIT_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @class JITMemoryManager
 * @brief A cross-platform memory manager for JIT code generation.
 * 
 * This class provides platform-specific memory allocation and management
 * for JIT-compiled code. It handles the allocation of executable memory
 * regions, permission changes (Write XOR Execute security), and proper
 * cleanup of allocated resources.
 * 
 * Key features:
 * - Cross-platform support (Linux, macOS, Windows)
 * - W^X (Write XOR Execute) security model
 * - Proper memory alignment for code generation
 * - Comprehensive error checking and reporting
 * - RAII-style resource management
 */
class JITMemoryManager {
public:
    /**
     * @brief Default constructor.
     * 
     * Creates a JITMemoryManager instance with no allocated memory.
     * Call allocate() to reserve memory for code generation.
     */
    JITMemoryManager();

    /**
     * @brief Constructor that immediately allocates memory.
     * 
     * @param size The size in bytes of memory to allocate.
     * @throws std::runtime_error if memory allocation fails.
     */
    explicit JITMemoryManager(size_t size);

    /**
     * @brief Destructor.
     * 
     * Automatically deallocates any allocated memory regions.
     */
    ~JITMemoryManager();

    // Disable copy constructor and assignment to prevent double-free
    JITMemoryManager(const JITMemoryManager&) = delete;
    JITMemoryManager& operator=(const JITMemoryManager&) = delete;

    // Enable move constructor and assignment for efficient resource transfer
    JITMemoryManager(JITMemoryManager&& other) noexcept;
    JITMemoryManager& operator=(JITMemoryManager&& other) noexcept;

    /**
     * @brief Allocates a memory region for code generation.
     * 
     * The allocated memory will initially have read and write permissions.
     * After writing the generated code, call makeExecutable() to change
     * permissions to read and execute (W^X security model).
     * 
     * @param size The size in bytes of memory to allocate.
     * @return Pointer to the allocated memory region.
     * @throws std::runtime_error if allocation fails or memory is already allocated.
     */
    void* allocate(size_t size);

    /**
     * @brief Changes memory permissions to read and execute (removes write permission).
     * 
     * This implements the W^X (Write XOR Execute) security model by removing
     * write permissions and adding execute permissions to the allocated memory.
     * Call this after writing all generated code to the memory region.
     * 
     * @throws std::runtime_error if the operation fails or no memory is allocated.
     */
    void makeExecutable();

    /**
     * @brief Changes memory permissions back to read and write (removes execute permission).
     * 
     * This allows modification of the memory region again. Useful for
     * recompilation or code patching scenarios.
     * 
     * @throws std::runtime_error if the operation fails or no memory is allocated.
     */
    void makeWritable();

    /**
     * @brief Deallocates the memory region.
     * 
     * After calling this method, the memory pointer becomes invalid.
     * The destructor will automatically call this if not called explicitly.
     */
    void deallocate();

    /**
     * @brief Gets a pointer to the allocated memory region.
     * 
     * @return Pointer to the memory region, or nullptr if no memory is allocated.
     */
    void* getMemoryPointer() const { return memory_; }

    /**
     * @brief Gets the size of the allocated memory region.
     * 
     * @return Size in bytes of the allocated memory, or 0 if no memory is allocated.
     */
    size_t getSize() const { return size_; }

    /**
     * @brief Checks if memory is currently allocated.
     * 
     * @return True if memory is allocated, false otherwise.
     */
    bool isAllocated() const { return memory_ != nullptr; }

    /**
     * @brief Checks if the memory region is currently executable.
     * 
     * @return True if the memory has execute permissions, false otherwise.
     */
    bool isExecutable() const { return is_executable_; }

    /**
     * @brief Gets the system page size.
     * 
     * This is useful for determining optimal alignment and allocation sizes.
     * 
     * @return The system page size in bytes.
     */
    static size_t getPageSize();

    /**
     * @brief Rounds up a size to the next page boundary.
     * 
     * @param size The size to round up.
     * @return The size rounded up to the next page boundary.
     */
    static size_t roundToPageSize(size_t size);

private:
    /**
     * @brief Platform-specific memory allocation implementation.
     * 
     * @param size The size in bytes to allocate.
     * @return Pointer to the allocated memory.
     * @throws std::runtime_error if allocation fails.
     */
    void* platformAllocate(size_t size);

    /**
     * @brief Platform-specific memory deallocation implementation.
     * 
     * @param ptr Pointer to the memory to deallocate.
     * @param size Size of the memory region.
     */
    void platformDeallocate(void* ptr, size_t size);

    /**
     * @brief Platform-specific memory protection change implementation.
     * 
     * @param ptr Pointer to the memory region.
     * @param size Size of the memory region.
     * @param executable If true, make memory executable; if false, make writable.
     * @throws std::runtime_error if the operation fails.
     */
    void platformSetPermissions(void* ptr, size_t size, bool executable);

    // Member variables
    void* memory_;          ///< Pointer to the allocated memory region
    size_t size_;           ///< Size of the allocated memory region in bytes
    bool is_executable_;    ///< True if memory currently has execute permissions
};

/**
 * @class JITMemoryManagerException
 * @brief Exception class for JITMemoryManager-specific errors.
 */
class JITMemoryManagerException : public std::runtime_error {
public:
    explicit JITMemoryManagerException(const std::string& message)
        : std::runtime_error("JITMemoryManager: " + message) {}
};

#endif // JIT_MEMORY_MANAGER_H