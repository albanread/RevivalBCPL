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

    // Store function address in the functions map - CRITICAL FIX
    codeGen.functions[node->name] = codeGen.instructions.getCurrentAddress();

    // First pass to collect vector allocations
    VectorAllocationVisitor vecVisitor;
    vecVisitor.visit(node);
    codeGen.vectorAllocations = vecVisitor.allocations;

    // Generate function label and record position
    codeGen.instructions.setPendingLabel(node->name);
    codeGen.labelManager.defineLabel(node->name, codeGen.instructions.getCurrentAddress());
    codeGen.addToListing(node->name + ":", "Function entry point");

    // PROLOGUE: Basic frame setup
    codeGen.instructions.stp(codeGen.X29, codeGen.X30, codeGen.SP, -16, "Store FP and LR");
    codeGen.instructions.mov(codeGen.X29, codeGen.SP, "Set up frame pointer");

    // Process function body
    if (node->body_stmt) {
        codeGen.visitStatement(node->body_stmt.get());
    } else if (node->body_expr) {
        codeGen.visitExpression(node->body_expr.get());
    }
    
    // Basic epilogue
    codeGen.instructions.ldp(codeGen.X29, codeGen.X30, codeGen.SP, 16, "Restore FP and LR");
    codeGen.instructions.ret("Return from function");
    
    codeGen.labelManager.popScope();
}

void StatementCodeGenerator::visitLetDeclaration(const LetDeclaration* node) {
    // Simplified implementation for testing
    for (const auto& init : node->initializers) {
        if (init.init) {
            // Evaluate expression, result is in X0
            codeGen.visitExpression(init.init.get());
            // For now, just allocate local storage but don't do register allocation
            int offset = codeGen.allocateLocal(init.name);
            // Store to local variable
            codeGen.instructions.str(codeGen.X0, codeGen.X29, offset, "Store local " + init.name);
        }
    }
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
    // Simplified assignment implementation 
    if (node->rhs.empty() || node->lhs.empty()) {
        throw std::runtime_error("Assignment must have both LHS and RHS");
    }
    
    // Evaluate RHS expression
    codeGen.visitExpression(node->rhs[0].get());

    // Handle simple variable assignment
    if (auto var = dynamic_cast<const VariableAccess*>(node->lhs[0].get())) {
        if (auto it = codeGen.manifestConstants.find(var->name); it != codeGen.manifestConstants.end()) {
            throw std::runtime_error("Cannot assign to manifest constant: " + var->name);
        } else if (auto it = codeGen.globals.find(var->name); it != codeGen.globals.end()) {
            codeGen.instructions.str(codeGen.X0, codeGen.X28, it->second * 8, "Store to global " + var->name);
        } else {
            // Simple local assignment - store to memory
            int offset = codeGen.getLocalOffset(var->name);
            codeGen.instructions.str(codeGen.X0, codeGen.X29, offset, "Store to local " + var->name);
        }
    } else {
        throw std::runtime_error("Complex assignment not implemented yet in refactored code");
    }
}

void StatementCodeGenerator::visitRoutineCall(const RoutineCall* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitReturnStatement(const ReturnStatement* node) {
    // No explicit branch needed here, as the return label is defined right before the epilogue.
    // The epilogue will be executed directly after the RETURN statement's code.
}

void StatementCodeGenerator::visitResultisStatement(const ResultisStatement* node) {
    // TODO: Move implementation from CodeGenerator.cpp
}

void StatementCodeGenerator::visitBreakStatement(const BreakStatement* node) {
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(endLabel, "Break from current construct");
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
    // This typically exits the current loop or function. For now, we'll treat it like a break.
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(endLabel, "Finish current construct");
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