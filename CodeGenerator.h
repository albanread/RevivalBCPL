// CodeGenerator.h
#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "AST.h"
#include "AArch64Instructions.h"
#include "LabelManager.h"
#include "ScratchAllocator.h"
#include "RegisterManager.h" // Include the new RegisterManager
#include <string>
#include <unordered_map>
#include <sstream>
#include <memory>

class StringAccess;
class StatementCodeGenerator;
class ExpressionCodeGenerator;

class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator(); // Need to declare destructor when using forward declarations with unique_ptr
    uintptr_t compile(ProgramPtr program);
    void printAsm() const;

    // Give specialized code generators access to private members
    friend class StatementCodeGenerator;
    friend class ExpressionCodeGenerator;

private:
    // Core components
    AArch64Instructions instructions;
    LabelManager labelManager;
    ScratchAllocator scratchAllocator;
    RegisterManager registerManager; // New RegisterManager member
    std::stringstream assemblyListing;
    std::vector<std::string> stringPool;
    std::string currentFunctionName; // Added to track the function being compiled

    // State tracking
    // Stack management
    int currentLocalVarOffset; // Tracks offset from FP for local variables
    int maxOutgoingParamSpace; // Max bytes for outgoing parameters in this function
    int maxCallerSavedRegsSpace; // Max bytes for caller-saved register spills in this function
    std::vector<std::tuple<uint32_t, int, std::string>> savedCallerRegsAroundCall; // Stores (reg, offset_from_FP, var_name) for caller-saved registers
    std::vector<std::pair<uint32_t, int>> savedCalleeRegsInPrologue; // Stores (reg, offset_from_FP) for callee-saved registers
    std::vector<uint32_t> calleeSavedRegs; // List of callee-saved registers (x19-x28)
    std::vector<const VectorConstructor*> vectorAllocations;

    std::unordered_map<std::string, int> localVars;
    std::unordered_map<std::string, size_t> globals;
    std::unordered_map<std::string, int> manifestConstants;
    std::unordered_map<std::string, size_t> functions;

    struct PendingCase {
        std::string label;
        const Statement* statement;
    };
    std::vector<PendingCase> pendingCases;
    
    // Specialized code generators
    std::unique_ptr<StatementCodeGenerator> statementGenerator;
    std::unique_ptr<ExpressionCodeGenerator> expressionGenerator;

    // Register constants
    const uint32_t X0 = AArch64Instructions::X0;   // Result/A register
    const uint32_t X1 = AArch64Instructions::X1;   // B register
    const uint32_t X2 = AArch64Instructions::X2;   // C register
    const uint32_t X28 = AArch64Instructions::X28; // Global vector
    const uint32_t X29 = AArch64Instructions::X29; // Frame pointer
    const uint32_t X30 = AArch64Instructions::X30; // Link register
    const uint32_t SP = AArch64Instructions::SP;   // Stack pointer
    const uint32_t XZR = AArch64Instructions::XZR; // Zero register

    // AST visitors
    void visitProgram(const Program* node);
    void visitStatement(const Statement* node);
    void visitExpression(const Expression* node);

    // Code generation helpers
    void resolveLabels();
    void saveCallerSavedRegisters();
    void restoreCallerSavedRegisters();

    int allocateLocal(const std::string& name);
    int getLocalOffset(const std::string& name);
    size_t allocateGlobal();
    std::string getLabelFromComment(const std::string& comment);
    void addToListing(const std::string& instruction, const std::string& comment = "");
    void emitAddress(const std::string& label);
    std::string formatInstruction(const std::string& mnemonic,
                                const std::vector<std::string>& operands,
                                const std::string& comment = "");
    bool isRegisterInUse(uint32_t reg);
    void saveCalleeSavedRegisters();
    void restoreCalleeSavedRegisters();
    void finalizeCode();
    void resolveBranchTargets();
    void performPeepholeOptimization();
    void generateAssemblyListing();
    bool canCombineLoadStore(const AArch64Instructions::Instruction& instr1, const AArch64Instructions::Instruction& instr2);
    void combineLoadStore(AArch64Instructions::Instruction& instr1, AArch64Instructions::Instruction& instr2);
};

#endif // CODEGENERATOR_H
