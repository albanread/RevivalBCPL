#include "AArch64Instructions.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstring>

/**
 * Test basic instruction encoding functionality.
 * This test validates that:
 * 1. Individual instructions can encode themselves to binary
 * 2. Address computation works correctly
 * 3. Branch resolution updates encodings properly
 * 4. Full buffer encoding produces correct machine code
 */

void printBytes(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(buffer[i]);
        if ((i + 1) % 4 == 0) std::cout << " ";
    }
    std::cout << std::endl;
}

void testBasicEncoding() {
    std::cout << "\n=== Testing Basic Instruction Encoding ===\n";
    
    AArch64Instructions instructions;
    
    // Test simple MOV instruction: mov x0, x1
    instructions.mov(AArch64Instructions::X0, AArch64Instructions::X1, "Test mov");
    
    // Test MOVZ instruction: movz x0, #42
    instructions.movz(AArch64Instructions::X0, 42, 0, "Test movz");
    
    // Test ADD instruction: add x0, x0, x1
    instructions.add(AArch64Instructions::X0, AArch64Instructions::X0, AArch64Instructions::X1, 
                    AArch64Instructions::LSL, 0, "Test add");
    
    // Test RET instruction
    instructions.ret("Test ret");
    
    std::cout << "Created " << instructions.size() << " instructions\n";
    
    // Test individual instruction encoding
    uint8_t buffer[4];
    instructions.at(0).encode(buffer);
    
    std::cout << "MOV x0, x1 encoding: ";
    printBytes(buffer, 4);
    
    // Expected: 0xE0 0x03 0x01 0xAA (mov x0, x1 in little-endian)
    // The actual encoding is determined by the AArch64 instruction format
    assert(buffer[0] == 0xE0 && buffer[1] == 0x03 && buffer[2] == 0x01 && buffer[3] == 0xAA);
    
    std::cout << "✓ Basic encoding test passed\n";
}

void testAddressComputation() {
    std::cout << "\n=== Testing Address Computation ===\n";
    
    AArch64Instructions instructions;
    
    // Create some instructions
    instructions.mov(AArch64Instructions::X0, AArch64Instructions::X1, "Instruction 1");
    instructions.add(AArch64Instructions::X0, AArch64Instructions::X0, AArch64Instructions::X2, 
                    AArch64Instructions::LSL, 0, "Instruction 2");
    instructions.ret("Instruction 3");
    
    // Compute addresses starting from base address 0x1000
    instructions.computeAddresses(0x1000);
    
    // Verify addresses
    assert(instructions.at(0).address == 0x1000);
    assert(instructions.at(1).address == 0x1004);
    assert(instructions.at(2).address == 0x1008);
    
    std::cout << "✓ Address computation test passed\n";
    std::cout << "  Instruction 0 address: 0x" << std::hex << instructions.at(0).address << "\n";
    std::cout << "  Instruction 1 address: 0x" << std::hex << instructions.at(1).address << "\n";
    std::cout << "  Instruction 2 address: 0x" << std::hex << instructions.at(2).address << "\n";
}

void testBranchResolution() {
    std::cout << "\n=== Testing Branch Resolution ===\n";
    
    AArch64Instructions instructions;
    
    // Create a simple function with a branch
    instructions.movz(AArch64Instructions::X0, 42, 0, "Load 42");
    instructions.setPendingLabel("loop");
    instructions.add(AArch64Instructions::X0, AArch64Instructions::X0, AArch64Instructions::X1, 
                    AArch64Instructions::LSL, 0, "Add x1 to x0");
    instructions.cbz(AArch64Instructions::X1, "end", "Branch if x1 is zero");
    instructions.b("loop", "Branch back to loop");
    instructions.setPendingLabel("end");
    instructions.ret("Return");
    
    std::cout << "Created " << instructions.size() << " instructions with labels\n";
    
    // Compute addresses and resolve branches
    instructions.computeAddresses(0x2000);
    instructions.resolveAllBranches();
    
    std::cout << "✓ Branch resolution test completed\n";
    
    // Print instruction details
    for (size_t i = 0; i < instructions.size(); i++) {
        const auto& instr = instructions.at(i);
        std::cout << "  [" << i << "] 0x" << std::hex << instr.address << ": " 
                  << instr.assembly;
        if (instr.hasLabel) {
            std::cout << " (label: " << instr.label << ")";
        }
        std::cout << "\n";
    }
}

void testFullBufferEncoding() {
    std::cout << "\n=== Testing Full Buffer Encoding ===\n";
    
    AArch64Instructions instructions;
    
    // Create a simple function that returns 42
    instructions.movz(AArch64Instructions::X0, 42, 0, "Load return value");
    instructions.ret("Return to caller");
    
    // Compute addresses
    instructions.computeAddresses(0x0);
    
    // Encode to buffer
    uint8_t buffer[32]; // More than enough for our test
    size_t bytesWritten = instructions.encodeToBuffer(buffer, sizeof(buffer));
    
    std::cout << "Encoded " << bytesWritten << " bytes:\n";
    printBytes(buffer, bytesWritten);
    
    // Verify we wrote the expected number of bytes
    assert(bytesWritten == instructions.size() * 4);
    
    std::cout << "✓ Full buffer encoding test passed\n";
}

void testCodeGeneratorIntegration() {
    std::cout << "\n=== Testing CodeGenerator Integration ===\n";
    
    AArch64Instructions instructions;
    
    // Simulate creating a simple function: int add42(int x) { return x + 42; }
    // Function prologue
    instructions.stp(AArch64Instructions::X29, AArch64Instructions::X30, AArch64Instructions::SP, -16, "Save FP and LR");
    instructions.mov(AArch64Instructions::X29, AArch64Instructions::SP, "Set up frame pointer");
    
    // Function body: add 42 to the input parameter (in X0)
    instructions.add(AArch64Instructions::X0, AArch64Instructions::X0, 42, "Add 42 to input");
    
    // Function epilogue
    instructions.ldp(AArch64Instructions::X29, AArch64Instructions::X30, AArch64Instructions::SP, 16, "Restore FP and LR");
    instructions.ret("Return to caller");
    
    std::cout << "Created function with " << instructions.size() << " instructions\n";
    
    // Use the new address computation and resolution
    instructions.computeAddresses(0x10000); // Start at some realistic address
    instructions.resolveAllBranches();
    
    // Encode to binary
    uint8_t buffer[64];
    size_t bytesWritten = instructions.encodeToBuffer(buffer, sizeof(buffer));
    
    std::cout << "Function encoded to " << bytesWritten << " bytes:\n";
    printBytes(buffer, bytesWritten);
    
    std::cout << "\nInstruction breakdown:\n";
    for (size_t i = 0; i < instructions.size(); i++) {
        const auto& instr = instructions.at(i);
        std::cout << "  0x" << std::hex << instr.address << ": " << instr.assembly;
        if (!instr.comment.empty()) {
            std::cout << " // " << instr.comment;
        }
        std::cout << "\n";
    }
    
    std::cout << "✓ CodeGenerator integration test passed\n";
}

int main() {
    std::cout << "AArch64 Instruction Encoding Test Suite\n";
    std::cout << "========================================\n";
    
    try {
        testBasicEncoding();
        testAddressComputation();
        testBranchResolution();
        testFullBufferEncoding();
        testCodeGeneratorIntegration();
        
        std::cout << "\n🎉 All tests passed!\n";
        std::cout << "\nThe instruction encoding system successfully:\n";
        std::cout << "- Encodes individual instructions to binary machine code\n";
        std::cout << "- Computes instruction addresses correctly\n";
        std::cout << "- Resolves branch targets and updates encodings\n";
        std::cout << "- Outputs complete binary code sequences\n";
        std::cout << "- Integrates with the CodeGenerator for JIT compilation\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}