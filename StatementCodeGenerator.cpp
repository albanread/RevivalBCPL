#include "StatementCodeGenerator.h"
#include "CodeGenerator.h"
#include "AST.h"
#include "StringAccess.h"
#include "VectorAllocationVisitor.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>

StatementCodeGenerator::StatementCodeGenerator(CodeGenerator& codeGenerator) : codeGen(codeGenerator) {
}

void StatementCodeGenerator::visitManifestDeclaration(const ManifestDeclaration* node) {
    for (const auto& manifest : node->manifests) {
        codeGen.manifestConstants[manifest.name] = manifest.value;
    }
}

void StatementCodeGenerator::visitGlobalDeclaration(const GlobalDeclaration* node) {
    for (const auto& global : node->globals) {
        codeGen.globals[global.name] = codeGen.globals.size();
    }
}

// Temporary delegation to the old method until we can move the implementation
void StatementCodeGenerator::visitFunctionDeclaration(const FunctionDeclaration* node) {
    // Delegate to CodeGenerator for now - will move implementation later
    codeGen.registerManager.clear(); 
    std::cout << "Visiting function declaration: " << node->name << std::endl;
    codeGen.currentFunctionName = node->name; 
    codeGen.labelManager.pushScope(LabelManager::ScopeType::FUNCTION);
    auto returnLabel = codeGen.labelManager.getCurrentReturnLabel();
    std::cout << "Generated return label: " << returnLabel << std::endl;

    // First pass to collect vector allocations
    VectorAllocationVisitor vecVisitor;
    vecVisitor.visit(node);
    codeGen.vectorAllocations = vecVisitor.allocations;

    // Generate function label and record position
    codeGen.instructions.setPendingLabel(node->name);
    codeGen.labelManager.defineLabel(node->name, codeGen.instructions.getCurrentAddress());
    codeGen.addToListing(node->name + ":", "Function entry point");

    // PROLOGUE:
    size_t prologueSubInstructionIndex = codeGen.instructions.size();
    codeGen.instructions.sub(codeGen.SP, codeGen.SP, 0, "Allocate stack frame (placeholder)");

    size_t stpInstructionIndex = codeGen.instructions.size();
    codeGen.instructions.stp(codeGen.X29, codeGen.X30, codeGen.SP, 0, "Save FP/LR at top of frame (placeholder offset)");

    codeGen.instructions.mov(codeGen.X29, codeGen.SP, "Set up frame pointer");

    // FIXME: We need to properly call the rest of the function implementation
    // For now, this is incomplete but will make it compile
    if (node->body_stmt) {
        codeGen.visitStatement(node->body_stmt.get());
    } else if (node->body_expr) {
        codeGen.visitExpression(node->body_expr.get());
    }
    
    codeGen.labelManager.popScope();
}

void StatementCodeGenerator::visitLetDeclaration(const LetDeclaration* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitCompoundStatement(const CompoundStatement* node) {
    // Simply visit each statement in the block
    for (const auto& stmt : node->statements) {
        codeGen.visitStatement(stmt.get());
    }
}

void StatementCodeGenerator::visitIfStatement(const IfStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitTestStatement(const TestStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitWhileStatement(const WhileStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitForStatement(const ForStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitSwitchonStatement(const SwitchonStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitGotoStatement(const GotoStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitLabeledStatement(const LabeledStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitAssignment(const Assignment* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitRoutineCall(const RoutineCall* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitReturnStatement(const ReturnStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitResultisStatement(const ResultisStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitBreakStatement(const BreakStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitLoopStatement(const LoopStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitRepeatStatement(const RepeatStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitEndcaseStatement(const EndcaseStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitFinishStatement(const FinishStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

// Helper methods for switch statement generation
void StatementCodeGenerator::generateJumpTable(const std::vector<SwitchonStatement::SwitchCase>& cases, const std::string& defaultLabel) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::generateBinarySearchTree(const std::vector<SwitchonStatement::SwitchCase>& cases, const std::string& defaultLabel) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::generateBinarySearchNode(const std::vector<SwitchonStatement::SwitchCase>& cases, size_t start, size_t end, const std::string& defaultLabel) {
    // TODO: Move implementation from CodeGenerator.cpp
}

bool StatementCodeGenerator::isSmallDenseRange(const std::vector<SwitchonStatement::SwitchCase>& cases) {
    // TODO: Move implementation from CodeGenerator.cpp
    return false;
}