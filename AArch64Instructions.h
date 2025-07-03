// AArch64Instructions.h
#ifndef AARCH64_INSTRUCTIONS_H
#define AARCH64_INSTRUCTIONS_H

#include <cstdint>
#include <vector>
#include <string>

class LabelManager;

/**
 * AArch64 Instruction Generator and Binary Encoder
 *
 * This class provides functionality for generating AArch64 assembly instructions
 * and encoding them to binary machine code for JIT compilation.
 *
 * The instruction encoding process works as follows:
 * 1. Instructions are created with their binary encodings pre-computed
 * 2. computeAddresses() assigns memory addresses to each instruction
 * 3. resolveAllBranches() updates branch instruction encodings with correct offsets
 * 4. encodeToBuffer() outputs the final binary machine code
 *
 * Each instruction is exactly 4 bytes (32 bits) in AArch64, and the binary
 * encoding follows the little-endian format required by the architecture.
 */
class AArch64Instructions {
public:
    // Register definitions
    static const uint32_t X0 = 0;   // First argument/return value
    static const uint32_t X1 = 1;   // Second argument/B register
    static const uint32_t X2 = 2;   // Third argument/C register
    static const uint32_t X3 = 3;
    static const uint32_t X4 = 4;
    static const uint32_t X5 = 5;
    static const uint32_t X6 = 6;
    static const uint32_t X7 = 7;
    static const uint32_t X9 = 9;
    static const uint32_t X10 = 10;
    static const uint32_t X28 = 28; // Global pointer (G)
    static const uint32_t X29 = 29; // Frame pointer (FP)
    static const uint32_t X30 = 30; // Link register (LR)
    static const uint32_t SP = 31;  // Stack pointer
    static const uint32_t XZR = 31; // Zero register (when used as source)

    struct Instruction {
        uint32_t encoding;
        std::string assembly;
        std::string comment;
        bool needsLabelResolution = false;
        std::string targetLabel;
        size_t address; // Address of the instruction in the generated code
        bool hasLabel = false;
        std::string label;

        bool isStore() const { return (encoding & 0x3B000000) == 0x38000000; } // Simplified check for STR/LDR (immediate offset)
        bool isLoad() const { return (encoding & 0x3B000000) == 0x38000000; } // Simplified check for STR/LDR (immediate offset)
        void resolveLabel(int32_t offset) { encoding |= ((offset / 4) & 0x7FFFF) << 5; } // Simplified for B/BL
        std::string toString() const { return assembly; }

        /**
         * Encode this instruction to binary machine code.
         * Writes the 32-bit encoding to the provided buffer in little-endian format
         * as required by AArch64 architecture.
         * @param out Buffer to write the 4-byte instruction encoding to
         */
        void encode(uint8_t* out) const {
            // AArch64 uses little-endian encoding
            out[0] = static_cast<uint8_t>(encoding & 0xFF);
            out[1] = static_cast<uint8_t>((encoding >> 8) & 0xFF);
            out[2] = static_cast<uint8_t>((encoding >> 16) & 0xFF);
            out[3] = static_cast<uint8_t>((encoding >> 24) & 0xFF);
        }
    };

    void setPendingLabel(const std::string& label);

private:
    std::vector<Instruction> instructions;
    std::string pendingLabel_;
    void addInstruction(Instruction instr);

public:
    // Basic instruction generators
    void mov(uint32_t rd, uint32_t rm, const std::string& comment = "");
    void movz(uint32_t rd, uint16_t imm16, uint8_t shift, const std::string& comment = "");
    void movk(uint32_t rd, uint16_t imm16, uint8_t shift, const std::string& comment = "");
    enum ShiftType {
        LSL, // Logical Shift Left
        LSR, // Logical Shift Right
        ASR, // Arithmetic Shift Right
        ROR  // Rotate Right
    };

    void add(uint32_t rd, uint32_t rn, uint32_t rm, ShiftType shift_type, uint32_t shift_amount, const std::string& comment = "");
    void add(uint32_t rd, uint32_t rn, uint32_t imm, const std::string& comment = "");
    void sub(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment);
    void sub_imm(uint32_t rd, uint32_t rn, uint32_t imm, const std::string& comment);
    void sub_reg(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment);
    void mul(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void sdiv(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void lsl(uint32_t rd, uint32_t rn, uint32_t imm, const std::string& comment = "");
    void lslv(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment);
    void lsrv(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment);
    void lsr(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void msub(uint32_t rd, uint32_t rn, uint32_t rm, uint32_t ra, const std::string& comment = "");
    void stp(uint32_t rt1, uint32_t rt2, uint32_t rn, int32_t imm, const std::string& comment = "");
    void ldp(uint32_t rt1, uint32_t rt2, uint32_t rn, int32_t imm, const std::string& comment = "");
    void str(uint32_t rt, uint32_t rn, int32_t imm, const std::string& comment = "");
    void ldr(uint32_t rt, uint32_t rn, int32_t imm, const std::string& comment = "");
    void b(const std::string& label, const std::string& comment = "");
    void bl(const std::string& label, const std::string& comment = "");
    void ret(const std::string& comment = "");

    void adr(uint32_t rd, const std::string& label, const std::string& comment = "");
    void br(uint32_t rn, const std::string& comment = "");

    void cbz(uint32_t rt, const std::string& label, const std::string& comment = "");

    // Helper methods for common BCPL patterns
    void moveAtoB() { mov(X1, X0, "B := A"); }
    void moveBtoC() { mov(X2, X1, "C := B"); }
    void loadImmediate(uint32_t rd, int64_t value, const std::string& comment = "");


    void neg(uint32_t rd, uint32_t rm, const std::string& comment = "");
    void eor(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void and_op(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void orr(uint32_t rd, uint32_t rn, uint32_t rm, const std::string& comment = "");
    void cmp(uint32_t rn, uint32_t rm, const std::string& comment = "");
    void beq(const std::string& label, const std::string& comment = "");
    void bne(const std::string& label, const std::string& comment = "");
    void bge(const std::string& label, const std::string& comment = "");
    void blt(const std::string& label, const std::string& comment = "");
    void ble(const std::string& label, const std::string& comment = "");
    void bgt(const std::string& label, const std::string& comment = "");
    void cset(uint32_t rd, uint32_t cond, const std::string& comment = "");

    enum Condition {
        EQ = 0b0000,
        NE = 0b0001,
        CS = 0b0010, HS = 0b0010,
        CC = 0b0011, LO = 0b0011,
        MI = 0b0100,
        PL = 0b0101,
        VS = 0b0110,
        VC = 0b0111,
        HI = 0b1000,
        LS = 0b1001,
        GE = 0b1010,
        LT = 0b1011,
        GT = 0b1100,
        LE = 0b1101,
        AL = 0b1110,
        NV = 0b1111,
    };


    // Helper method to convert register numbers to strings
    std::string regName(uint32_t reg) const;

    // Get the current instruction address (for label resolution)
    size_t getCurrentAddress() const;

    // Resolve a branch instruction's offset
    void resolveBranch(size_t instructionIndex, int32_t offset);
    void addUnresolvedBranch(const std::string& label);
    void resolveBranches(const LabelManager& labelManager);

    /**
     * Compute addresses for all instructions in the sequence.
     * This pass assigns addresses to each instruction, enabling proper
     * branch target resolution. Each AArch64 instruction is 4 bytes.
     * @param baseAddress Starting address for the first instruction
     */
    void computeAddresses(size_t baseAddress = 0);

    /**
     * Resolve all branch targets and update instruction encodings.
     * This pass must be called after computeAddresses() and after all
     * labels have been defined. It updates branch instruction encodings
     * with the correct relative offsets to their targets.
     */
    void resolveAllBranches();

    /**
     * Encode all instructions to a binary buffer.
     * @param buffer Output buffer to write encoded instructions to
     * @param bufferSize Size of the output buffer in bytes
     * @return Number of bytes written to the buffer
     */
    size_t encodeToBuffer(uint8_t* buffer, size_t bufferSize) const;

    void clear();
    Instruction& at(size_t index);
    size_t size() const;

    std::vector<Instruction>& getInstructions() { return instructions; }
    const std::vector<Instruction>& getInstructions() const { return instructions; }
};

#endif // AARCH64_INSTRUCTIONS_H
