#include "CodeGenerator.h"
#include "StatementCodeGenerator.h"
#include "ExpressionCodeGenerator.h"
#include "AST.h"
#include "StringAccess.h"
#include "VectorAllocationVisitor.h"
#include "JitRuntime.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm> // For std::min

CodeGenerator::CodeGenerator() : instructions(), labelManager(), scratchAllocator(), registerManager(instructions), currentLocalVarOffset(0), maxOutgoingParamSpace(0), maxCallerSavedRegsSpace(0), manifestConstants() {
    // Initialize callee-saved registers (x19-x28)
    for (uint32_t i = 19; i <= 28; ++i) {
        calleeSavedRegs.push_back(i);
    }
    
    // Initialize specialized code generators
    statementGenerator = std::make_unique<StatementCodeGenerator>(*this);
    expressionGenerator = std::make_unique<ExpressionCodeGenerator>(*this);
}

CodeGenerator::~CodeGenerator() = default;

uintptr_t CodeGenerator::compile(ProgramPtr program) {
    // Reset state for a new compilation
    instructions.clear();
    localVars.clear();
    functions.clear();
    globals.clear();
    currentLocalVarOffset = 0;
    maxOutgoingParamSpace = 0;
    maxCallerSavedRegsSpace = 0;
    savedCalleeRegsInPrologue.clear();
    assemblyListing.str("");
    pendingCases.clear();
    registerManager.clear(); // Clear register manager state

    // Register runtime functions
    for (const auto& symbol : JitRuntime::getInstance().getSymbolTable()) {
        functions[symbol.first] = symbol.second;
    }

    // Generate code
    visitProgram(program.get());

    // Find entry point (START function)
    auto it = functions.find("START");
    if (it == functions.end()) {
        throw std::runtime_error("No START function found");
    }
    return it->second;
}

void CodeGenerator::visitProgram(const Program* node) {
    // First pass: collect all global and manifest declarations
    for (const auto& decl : node->declarations) {
        if (auto globalDecl = dynamic_cast<const GlobalDeclaration*>(decl.get())) {
            statementGenerator->visitGlobalDeclaration(globalDecl);
        } else if (auto manifestDecl = dynamic_cast<const ManifestDeclaration*>(decl.get())) {
            statementGenerator->visitManifestDeclaration(manifestDecl);
        }
    }

    // Second pass: generate code for declarations
    for (const auto& decl : node->declarations) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            statementGenerator->visitFunctionDeclaration(funcDecl);
        } else if (auto letDecl = dynamic_cast<const LetDeclaration*>(decl.get())) {
            statementGenerator->visitLetDeclaration(letDecl);
        } else if (auto globalDecl = dynamic_cast<const GlobalDeclaration*>(decl.get())) {
            statementGenerator->visitGlobalDeclaration(globalDecl);
        } else if (auto manifestDecl = dynamic_cast<const ManifestDeclaration*>(decl.get())) {
            statementGenerator->visitManifestDeclaration(manifestDecl);
        } else if (auto valof = dynamic_cast<const Valof*>(decl.get())) {
            expressionGenerator->visitValof(valof);
        }
    }
}

void CodeGenerator::visitStatement(const Statement* stmt) {
    if (auto compound = dynamic_cast<const CompoundStatement*>(stmt)) {
        statementGenerator->visitCompoundStatement(compound);
    } else if (auto letDecl = dynamic_cast<const LetDeclaration*>(stmt)) {
        statementGenerator->visitLetDeclaration(letDecl);
    } else if (auto ifStmt = dynamic_cast<const IfStatement*>(stmt)) {
        statementGenerator->visitIfStatement(ifStmt);
    } else if (auto testStmt = dynamic_cast<const TestStatement*>(stmt)) {
        statementGenerator->visitTestStatement(testStmt);
    } else if (auto whileStmt = dynamic_cast<const WhileStatement*>(stmt)) {
        statementGenerator->visitWhileStatement(whileStmt);
    } else if (auto switchStmt = dynamic_cast<const SwitchonStatement*>(stmt)) {
        statementGenerator->visitSwitchonStatement(switchStmt);
    } else if (auto forStmt = dynamic_cast<const ForStatement*>(stmt)) {
        statementGenerator->visitForStatement(forStmt);
    } else if (auto gotoStmt = dynamic_cast<const GotoStatement*>(stmt)) {
        statementGenerator->visitGotoStatement(gotoStmt);
    } else if (auto labeledStmt = dynamic_cast<const LabeledStatement*>(stmt)) {
        statementGenerator->visitLabeledStatement(labeledStmt);
    } else if (auto assign = dynamic_cast<const Assignment*>(stmt)) {
        statementGenerator->visitAssignment(assign);
    } else if (auto routine = dynamic_cast<const RoutineCall*>(stmt)) {
        statementGenerator->visitRoutineCall(routine);
    } else if (auto resultis = dynamic_cast<const ResultisStatement*>(stmt)) {
        statementGenerator->visitResultisStatement(resultis);
    } else if (auto breakStmt = dynamic_cast<const BreakStatement*>(stmt)) {
        statementGenerator->visitBreakStatement(breakStmt);
    } else if (auto returnStmt = dynamic_cast<const ReturnStatement*>(stmt)) {
        statementGenerator->visitReturnStatement(returnStmt);
    } else if (auto loopStmt = dynamic_cast<const LoopStatement*>(stmt)) {
        statementGenerator->visitLoopStatement(loopStmt);
    } else if (auto repeatStmt = dynamic_cast<const RepeatStatement*>(stmt)) {
        statementGenerator->visitRepeatStatement(repeatStmt);
    } else if (auto endcaseStmt = dynamic_cast<const EndcaseStatement*>(stmt)) {
        statementGenerator->visitEndcaseStatement(endcaseStmt);
    } else if (auto finishStmt = dynamic_cast<const FinishStatement*>(stmt)) {
        statementGenerator->visitFinishStatement(finishStmt);
    }
}

void CodeGenerator::visitExpression(const Expression* expr) {
    if (auto* numLit = dynamic_cast<const NumberLiteral*>(expr)) {
        expressionGenerator->visitNumberLiteral(numLit);
    } else if (auto* strLit = dynamic_cast<const StringLiteral*>(expr)) {
        expressionGenerator->visitStringLiteral(strLit);
    } else if (auto* charLit = dynamic_cast<const CharLiteral*>(expr)) {
        expressionGenerator->visitCharLiteral(charLit);
    } else if (auto* var = dynamic_cast<const VariableAccess*>(expr)) {
        expressionGenerator->visitVariableAccess(var);
    } else if (auto* unary = dynamic_cast<const UnaryOp*>(expr)) {
        expressionGenerator->visitUnaryOp(unary);
    } else if (auto* binary = dynamic_cast<const BinaryOp*>(expr)) {
        expressionGenerator->visitBinaryOp(binary);
    } else if (auto* call = dynamic_cast<const FunctionCall*>(expr)) {
        expressionGenerator->visitFunctionCall(call);
    } else if (auto* cond = dynamic_cast<const ConditionalExpression*>(expr)) {
        expressionGenerator->visitConditionalExpression(cond);
    } else if (auto* valof = dynamic_cast<const Valof*>(expr)) {
        expressionGenerator->visitValof(valof);
    } else if (auto* table = dynamic_cast<const TableConstructor*>(expr)) {
        expressionGenerator->visitTableConstructor(table);
    } else if (auto* vec = dynamic_cast<const VectorConstructor*>(expr)) {
        expressionGenerator->visitVectorConstructor(vec);
    } else if (auto* charAccess = dynamic_cast<const CharacterAccess*>(expr)) {
        expressionGenerator->visitCharacterAccess(charAccess);
    }
}

// Keep only the essential helper methods that are not visitor-specific
void CodeGenerator::resolveLabels() {
    auto fixups = labelManager.getFixups();
    for (const auto& fixup : fixups) {
        size_t targetAddress = labelManager.getLabelAddress(fixup.labelName);
        int32_t offset = static_cast<int32_t>(targetAddress - fixup.instructionAddress);
        instructions.resolveBranch(fixup.instructionAddress / 4, offset);
    }
}

/**
 * Compute instruction addresses and resolve all branch targets.
 * This method performs the complete address assignment and branch resolution
 * process required for binary code generation in JIT compilation.
 * @param baseAddress Starting address for the first instruction
 */
void CodeGenerator::finalizeInstructionAddressing(size_t baseAddress) {
    // Step 1: Compute addresses for all instructions
    instructions.computeAddresses(baseAddress);
    
    // Step 2: Resolve all branch targets using the new instruction-based resolution
    instructions.resolveAllBranches();
}

void CodeGenerator::saveCallerSavedRegisters() {
    // Implementation placeholder
}

void CodeGenerator::restoreCallerSavedRegisters() {
    // Implementation placeholder
}

int CodeGenerator::allocateLocal(const std::string& name) {
    if (localVars.find(name) != localVars.end()) {
        return localVars[name];
    }
    currentLocalVarOffset -= 8; // 8 bytes per local variable
    localVars[name] = currentLocalVarOffset;
    return currentLocalVarOffset;
}

int CodeGenerator::getLocalOffset(const std::string& name) {
    auto it = localVars.find(name);
    if (it != localVars.end()) {
        return it->second;
    }
    throw std::runtime_error("Local variable not found: " + name);
}

size_t CodeGenerator::allocateGlobal() {
    return globals.size() * 8; // 8 bytes per global
}

std::string CodeGenerator::getLabelFromComment(const std::string& comment) {
    return "label_" + comment;
}

void CodeGenerator::addToListing(const std::string& instruction, const std::string& comment) {
    assemblyListing << instruction;
    if (!comment.empty()) {
        assemblyListing << " // " << comment;
    }
    assemblyListing << std::endl;
}

void CodeGenerator::emitAddress(const std::string& label) {
    // Implementation placeholder
}

std::string CodeGenerator::formatInstruction(const std::string& mnemonic,
                            const std::vector<std::string>& operands,
                            const std::string& comment) {
    std::string result = mnemonic;
    if (!operands.empty()) {
        result += " " + operands[0];
        for (size_t i = 1; i < operands.size(); ++i) {
            result += ", " + operands[i];
        }
    }
    if (!comment.empty()) {
        result += " // " + comment;
    }
    return result;
}

bool CodeGenerator::isRegisterInUse(uint32_t reg) {
    return false; // Placeholder
}

void CodeGenerator::saveCalleeSavedRegisters() {
    // Implementation placeholder
}

void CodeGenerator::restoreCalleeSavedRegisters() {
    for (auto it = savedCalleeRegsInPrologue.rbegin(); it != savedCalleeRegsInPrologue.rend(); ++it) {
        instructions.ldr(it->first, X29, it->second, "Restore callee-saved register " + instructions.regName(it->first));
        currentLocalVarOffset += 8;
    }
    savedCalleeRegsInPrologue.clear();
}

void CodeGenerator::finalizeCode() {
    // Implementation placeholder
}

void CodeGenerator::resolveBranchTargets() {
    // Implementation placeholder
}

void CodeGenerator::performPeepholeOptimization() {
    // Implementation placeholder
}

void CodeGenerator::generateAssemblyListing() {
    // Implementation placeholder
}

bool CodeGenerator::canCombineLoadStore(const AArch64Instructions::Instruction& instr1, const AArch64Instructions::Instruction& instr2) {
    return false; // Placeholder
}

void CodeGenerator::combineLoadStore(AArch64Instructions::Instruction& instr1, AArch64Instructions::Instruction& instr2) {
    // Implementation placeholder
}

void CodeGenerator::printAsm() const {
    std::cout << assemblyListing.str() << std::endl;
}
