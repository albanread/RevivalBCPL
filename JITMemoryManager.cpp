#include "JITMemoryManager.h"
#include <string>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <cerrno>
    #include <cstring>
#endif

#include <algorithm>

// Constructor implementations
JITMemoryManager::JITMemoryManager()
    : memory_(nullptr), size_(0), is_executable_(false) {
}

JITMemoryManager::JITMemoryManager(size_t size)
    : memory_(nullptr), size_(0), is_executable_(false) {
    allocate(size);
}

JITMemoryManager::~JITMemoryManager() {
    deallocate();
}

// Move constructor
JITMemoryManager::JITMemoryManager(JITMemoryManager&& other) noexcept
    : memory_(other.memory_), size_(other.size_), is_executable_(other.is_executable_) {
    other.memory_ = nullptr;
    other.size_ = 0;
    other.is_executable_ = false;
}

// Move assignment operator
JITMemoryManager& JITMemoryManager::operator=(JITMemoryManager&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        deallocate();
        
        // Transfer ownership
        memory_ = other.memory_;
        size_ = other.size_;
        is_executable_ = other.is_executable_;
        
        // Reset other object
        other.memory_ = nullptr;
        other.size_ = 0;
        other.is_executable_ = false;
    }
    return *this;
}

void* JITMemoryManager::allocate(size_t size) {
    if (memory_ != nullptr) {
        throw JITMemoryManagerException("Memory already allocated. Call deallocate() first.");
    }
    
    if (size == 0) {
        throw JITMemoryManagerException("Cannot allocate zero bytes.");
    }
    
    // Round up to page size for optimal performance and platform requirements
    size_t aligned_size = roundToPageSize(size);
    
    try {
        memory_ = platformAllocate(aligned_size);
        size_ = aligned_size;
        is_executable_ = false;
        
        return memory_;
    } catch (const std::exception& e) {
        throw JITMemoryManagerException(std::string("Failed to allocate ") + std::to_string(size) + 
                                       " bytes: " + e.what());
    }
}

void JITMemoryManager::makeExecutable() {
    if (memory_ == nullptr) {
        throw JITMemoryManagerException("No memory allocated.");
    }
    
    if (is_executable_) {
        return; // Already executable
    }
    
    try {
        platformSetPermissions(memory_, size_, true);
        is_executable_ = true;
    } catch (const std::exception& e) {
        throw JITMemoryManagerException(std::string("Failed to make memory executable: ") + e.what());
    }
}

void JITMemoryManager::makeWritable() {
    if (memory_ == nullptr) {
        throw JITMemoryManagerException("No memory allocated.");
    }
    
    if (!is_executable_) {
        return; // Already writable
    }
    
    try {
        platformSetPermissions(memory_, size_, false);
        is_executable_ = false;
    } catch (const std::exception& e) {
        throw JITMemoryManagerException(std::string("Failed to make memory writable: ") + e.what());
    }
}

void JITMemoryManager::deallocate() {
    if (memory_ != nullptr) {
        platformDeallocate(memory_, size_);
        memory_ = nullptr;
        size_ = 0;
        is_executable_ = false;
    }
}

size_t JITMemoryManager::getPageSize() {
#ifdef _WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
#else
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        throw JITMemoryManagerException("Failed to get system page size.");
    }
    return static_cast<size_t>(page_size);
#endif
}

size_t JITMemoryManager::roundToPageSize(size_t size) {
    size_t page_size = getPageSize();
    return ((size + page_size - 1) / page_size) * page_size;
}

// Platform-specific implementations

#ifdef _WIN32
// Windows implementation using VirtualAlloc/VirtualProtect/VirtualFree

void* JITMemoryManager::platformAllocate(size_t size) {
    void* ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == nullptr) {
        DWORD error = GetLastError();
        throw std::runtime_error("VirtualAlloc failed with error code: " + std::to_string(error));
    }
    return ptr;
}

void JITMemoryManager::platformDeallocate(void* ptr, size_t size) {
    (void)size; // Unused parameter in Windows
    if (!VirtualFree(ptr, 0, MEM_RELEASE)) {
        // Note: We don't throw in destructor path, just log the error
        // In a real implementation, you might want to use a logging system
    }
}

void JITMemoryManager::platformSetPermissions(void* ptr, size_t size, bool executable) {
    DWORD new_protection = executable ? PAGE_EXECUTE_READ : PAGE_READWRITE;
    DWORD old_protection;
    
    if (!VirtualProtect(ptr, size, new_protection, &old_protection)) {
        DWORD error = GetLastError();
        throw std::runtime_error("VirtualProtect failed with error code: " + std::to_string(error));
    }
}

#else
// Unix-like systems (Linux, macOS) implementation using mmap/mprotect/munmap

void* JITMemoryManager::platformAllocate(size_t size) {
    // Use mmap to allocate memory with read and write permissions initially
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ptr == MAP_FAILED) {
        throw std::runtime_error(std::string("mmap failed: ") + strerror(errno));
    }
    
    return ptr;
}

void JITMemoryManager::platformDeallocate(void* ptr, size_t size) {
    if (munmap(ptr, size) != 0) {
        // Note: We don't throw in destructor path, just log the error
        // In a real implementation, you might want to use a logging system
    }
}

void JITMemoryManager::platformSetPermissions(void* ptr, size_t size, bool executable) {
    int protection;
    
    if (executable) {
        // W^X security: read and execute, but not write
        protection = PROT_READ | PROT_EXEC;
    } else {
        // Writable: read and write, but not execute
        protection = PROT_READ | PROT_WRITE;
    }
    
    if (mprotect(ptr, size, protection) != 0) {
        throw std::runtime_error(std::string("mprotect failed: ") + strerror(errno));
    }
}

#endif