#include "ExpressionCodeGenerator.h"
#include "CodeGenerator.h"
#include "AST.h"
#include "StringAccess.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>

ExpressionCodeGenerator::ExpressionCodeGenerator(CodeGenerator& codeGenerator) : codeGen(codeGenerator) {
}

void ExpressionCodeGenerator::visitNumberLiteral(const NumberLiteral* node) {
    codeGen.instructions.loadImmediate(codeGen.X0, node->value, "Load number literal");
}

void ExpressionCodeGenerator::visitCharLiteral(const CharLiteral* node) {
    codeGen.instructions.loadImmediate(codeGen.X0, node->value, "Load char literal");
}

void ExpressionCodeGenerator::visitStringLiteral(const StringLiteral* node) {
    std::string label = ".L.str" + std::to_string(codeGen.stringPool.size());
    codeGen.stringPool.push_back(node->value);
    codeGen.instructions.adr(codeGen.X0, label, "Load string literal address");
}

void ExpressionCodeGenerator::visitVariableAccess(const VariableAccess* node) {
    // First, check for manifest constants
    if (auto it = codeGen.manifestConstants.find(node->name); it != codeGen.manifestConstants.end()) {
        codeGen.instructions.loadImmediate(codeGen.X0, it->second, "Load manifest constant " + node->name);
        return;
    }

    // Second, check for globals
    if (auto it = codeGen.globals.find(node->name); it != codeGen.globals.end()) {
        codeGen.instructions.ldr(codeGen.X0, codeGen.X28, it->second * 8, "Load global " + node->name);
        return;
    }

    // Local variables - check if already in register
    uint32_t reg = codeGen.registerManager.getVariableRegister(node->name);

    if (reg != 0xFFFFFFFF) {
        // Variable is in a register - generate register-to-register move
        codeGen.instructions.mov(codeGen.X0, reg, "Move " + node->name + " from " + codeGen.instructions.regName(reg) + " to X0");
    } else {
        // Variable is not in register - load from stack
        int offset = codeGen.getLocalOffset(node->name);
        uint32_t new_reg = codeGen.registerManager.acquireRegister(node->name, offset);
        codeGen.instructions.mov(codeGen.X0, new_reg, "Move " + node->name + " from " + codeGen.instructions.regName(new_reg) + " to X0");
    }
}

// Placeholder implementations for other expression visitor methods
void ExpressionCodeGenerator::visitUnaryOp(const UnaryOp* node) {
    codeGen.visitExpression(node->rhs.get());

    switch (node->op) {
        case TokenType::OpLogNot:
            codeGen.instructions.eor(codeGen.X0, codeGen.X0, 1, "Logical NOT");
            break;
        case TokenType::OpMinus:
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Arithmetic negation");
            break;
        case TokenType::OpAt:  // @ operator (address-of)
            // For variables, calculate address instead of loading value
            if (auto var = dynamic_cast<const VariableAccess*>(node->rhs.get())) {
                if (auto it = codeGen.globals.find(var->name); it != codeGen.globals.end()) {
                    codeGen.instructions.add(codeGen.X0, codeGen.X28, it->second * 8, AArch64Instructions::LSL, 0, "Address of global " + var->name);
                } else {
                    int offset = codeGen.getLocalOffset(var->name);
                    codeGen.instructions.add(codeGen.X0, codeGen.X29, offset, AArch64Instructions::LSL, 0, "Address of local " + var->name);
                }
            } else {
                throw std::runtime_error("@ operator requires addressable operand");
            }
            break;
        case TokenType::OpBang:  // ! operator (indirection)
            codeGen.instructions.ldr(codeGen.X0, codeGen.X0, 0, "Indirection");
            break;
        default:
            throw std::runtime_error("Unknown unary operator");
    }
}

void ExpressionCodeGenerator::visitBinaryOp(const BinaryOp* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitBinaryOp not implemented yet");
}

void ExpressionCodeGenerator::visitFunctionCall(const FunctionCall* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitFunctionCall not implemented yet");
}

void ExpressionCodeGenerator::visitConditionalExpression(const ConditionalExpression* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitConditionalExpression not implemented yet");
}

void ExpressionCodeGenerator::visitValof(const Valof* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitValof not implemented yet");
}

void ExpressionCodeGenerator::visitTableConstructor(const TableConstructor* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitTableConstructor not implemented yet");
}

void ExpressionCodeGenerator::visitVectorConstructor(const VectorConstructor* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitVectorConstructor not implemented yet");
}

void ExpressionCodeGenerator::visitStringAccess(const StringAccess* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitStringAccess not implemented yet");
}

void ExpressionCodeGenerator::visitCharacterAccess(const CharacterAccess* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitCharacterAccess not implemented yet");
}