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
    // Handle special cases first
    if (node->op == TokenType::OpBang) {  // Vector subscript (V!E)
        // Evaluate index first
        codeGen.visitExpression(node->right.get());
        // Save index
        uint32_t indexReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(indexReg, codeGen.X0);

        // Evaluate vector address
        codeGen.visitExpression(node->left.get());

        // Calculate final address and load
        codeGen.instructions.add(codeGen.X0, codeGen.X0, indexReg, AArch64Instructions::LSL, 3, "Calculate element address (word size)");
        codeGen.instructions.ldr(codeGen.X0, codeGen.X0, 0, "Load vector element");
        codeGen.scratchAllocator.release(indexReg);
        return;
    }

    // Normal binary operators
    codeGen.visitExpression(node->right.get());
    uint32_t rightReg = codeGen.scratchAllocator.acquire();
    codeGen.instructions.mov(rightReg, codeGen.X0);

    codeGen.visitExpression(node->left.get());

    switch (node->op) {
        case TokenType::OpPlus:
            codeGen.instructions.add(codeGen.X0, codeGen.X0, rightReg, AArch64Instructions::LSL, 0, "Add");
            break;
        case TokenType::OpMinus:
            codeGen.instructions.sub(codeGen.X0, codeGen.X0, rightReg, "Subtract");
            break;
        case TokenType::OpMultiply:
            codeGen.instructions.mul(codeGen.X0, codeGen.X0, rightReg, "Multiply");
            break;
        case TokenType::OpDivide:
            // TODO: Add division by zero check
            codeGen.instructions.sdiv(codeGen.X0, codeGen.X0, rightReg, "Divide");
            break;
        case TokenType::OpRemainder:
            // TODO: Add division by zero check
            codeGen.instructions.sdiv(codeGen.X2, codeGen.X0, rightReg, "Divide for remainder");
            codeGen.instructions.msub(codeGen.X0, codeGen.X2, rightReg, codeGen.X0, "Calculate remainder");
            break;
        case TokenType::OpEq:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::EQ);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpNe:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::NE);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpLt:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::LT);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpGt:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::GT);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpLe:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::LE);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpGe:
            codeGen.instructions.cmp(codeGen.X0, rightReg);
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::GE);
            codeGen.instructions.neg(codeGen.X0, codeGen.X0);
            break;
        case TokenType::OpLshift:
            codeGen.instructions.lsl(codeGen.X0, codeGen.X0, 1, "Left shift by 1 (strength reduction)");
            break;
        case TokenType::OpRshift:
            codeGen.instructions.lsr(codeGen.X0, codeGen.X0, rightReg, "Right shift");
            break;
        // Add other operators (AND, OR, etc.)
        default:
            throw std::runtime_error("Unknown binary operator");
    }
    codeGen.scratchAllocator.release(rightReg);
}

void ExpressionCodeGenerator::visitFunctionCall(const FunctionCall* node) {
    // Simplified function call implementation
    // For now, handle basic calls with up to 2 arguments
    
    if (node->arguments.size() > 2) {
        throw std::runtime_error("Function calls with more than 2 arguments not yet supported in refactored code");
    }
    
    // Evaluate arguments and place in X0, X1
    if (node->arguments.size() >= 1) {
        codeGen.visitExpression(node->arguments[0].get());
        if (node->arguments.size() == 2) {
            // Save first argument in X1
            codeGen.instructions.mov(codeGen.X1, codeGen.X0, "Move first arg to X1");
            // Evaluate second argument
            codeGen.visitExpression(node->arguments[1].get());
            // Second argument is now in X0, first is in X1
        }
    }
    
    // Generate call
    if (auto funcVar = dynamic_cast<const VariableAccess*>(node->function.get())) {
        // Direct function call
        if (auto it = codeGen.functions.find(funcVar->name); it != codeGen.functions.end()) {
            codeGen.instructions.bl(funcVar->name, "Call " + funcVar->name);
        } else {
            throw std::runtime_error("Unknown function: " + funcVar->name);
        }
    } else {
        throw std::runtime_error("Indirect function calls not yet supported in refactored code");
    }
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