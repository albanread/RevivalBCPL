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
    // Evaluate LHS, result in X0
    codeGen.visitExpression(node->left.get());
    uint32_t lhs_reg = codeGen.registerManager.acquireRegister("lhs_temp_reg_" + std::to_string(codeGen.instructions.getCurrentAddress()), 0); // Acquire a scratch register for LHS

    // Move LHS result from X0 to the acquired scratch register
    codeGen.instructions.mov(lhs_reg, codeGen.X0, "Save LHS result to scratch register");

    // Evaluate RHS, result in X0
    codeGen.visitExpression(node->right.get());
    uint32_t rhs_reg = codeGen.X0; // RHS result is in X0

    switch (node->op) {
        case TokenType::OpPlus:
            codeGen.instructions.add(codeGen.X0, lhs_reg, rhs_reg, "Addition");
            break;
        case TokenType::OpMinus:
            codeGen.instructions.sub(codeGen.X0, lhs_reg, rhs_reg, "Subtraction");
            break;
        case TokenType::OpMultiply:
            codeGen.instructions.mul(codeGen.X0, lhs_reg, rhs_reg, "Multiplication");
            break;
        case TokenType::OpDivide:
            // Integer division (signed)
            codeGen.instructions.sdiv(codeGen.X0, lhs_reg, rhs_reg, "Signed Division");
            break;
        case TokenType::OpRemainder:
            // Integer remainder (signed)
            // AArch64 doesn't have a direct REM instruction. It's usually implemented as:
            // REM = LHS - (LHS / RHS) * RHS
            { // Use a block to limit scope of temp_div_reg
                uint32_t temp_div_reg = codeGen.registerManager.acquireRegister("rem_temp_reg_" + std::to_string(codeGen.instructions.getCurrentAddress()), 0);
                codeGen.instructions.sdiv(temp_div_reg, lhs_reg, rhs_reg, "Temp for division in REM"); // temp_div_reg = LHS / RHS
                codeGen.instructions.mul(temp_div_reg, temp_div_reg, rhs_reg, "Temp for (LHS/RHS)*RHS"); // temp_div_reg = (LHS/RHS)*RHS
                codeGen.instructions.sub(codeGen.X0, lhs_reg, temp_div_reg, "Remainder"); // X0 = LHS - temp_div_reg
                codeGen.registerManager.releaseRegister(temp_div_reg);
            }
            break;

        // Comparisons (result -1 for true, 0 for false)
        case TokenType::OpEq:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for equality");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::EQ, "Set X0 if equal");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true"); // BCPL true is -1
            break;
        case TokenType::OpNe:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for inequality");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::NE, "Set X0 if not equal");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
            break;
        case TokenType::OpLt:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for less than");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::LT, "Set X0 if less than");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
            break;
        case TokenType::OpGt:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for greater than");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::GT, "Set X0 if greater than");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
            break;
        case TokenType::OpLe:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for less than or equal");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::LE, "Set X0 if less than or equal");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
            break;
        case TokenType::OpGe:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for greater than or equal");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::GE, "Set X0 if greater than or equal");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
            break;

        // Logical operations (bitwise)
        case TokenType::OpLogAnd:
            codeGen.instructions.and_op(codeGen.X0, lhs_reg, rhs_reg, "Bitwise AND");
            break;
        case TokenType::OpLogOr:
            codeGen.instructions.orr(codeGen.X0, lhs_reg, rhs_reg, "Bitwise OR");
            break;
        case TokenType::OpLshift:
            codeGen.instructions.lsl(codeGen.X0, lhs_reg, rhs_reg, "Logical Left Shift");
            break;
        case TokenType::OpRshift:
            codeGen.instructions.lsr(codeGen.X0, lhs_reg, rhs_reg, "Logical Right Shift");
            break;

        // Floating point operations (need to use floating point registers and instructions)
        case TokenType::OpFloatPlus:
        case TokenType::OpFloatMinus:
        case TokenType::OpFloatMultiply:
        case TokenType::OpFloatDivide:
        case TokenType::OpFloatEq:
        case TokenType::OpFloatNe:
        case TokenType::OpFloatLt:
        case TokenType::OpFloatGt:
        case TokenType::OpFloatLe:
        case TokenType::OpFloatGe:
            throw std::runtime_error("Floating point binary operations not implemented yet.");

        default:
            throw std::runtime_error("Unsupported binary operator: " + Token::tokenTypeToString(node->op));
    }

    codeGen.registerManager.releaseRegister(lhs_reg);
}

void ExpressionCodeGenerator::visitFunctionCall(const FunctionCall* node) {
    throw std::runtime_error("ExpressionCodeGenerator::visitFunctionCall not implemented yet");
}

void ExpressionCodeGenerator::visitConditionalExpression(const ConditionalExpression* node) {
    // Evaluate the condition
    codeGen.visitExpression(node->condition.get());

    std::string elseLabel = codeGen.labelManager.generateLabel("cond_else");
    std::string endLabel = codeGen.labelManager.generateLabel("cond_end");

    // If X0 is 0 (false), branch to elseLabel
    codeGen.instructions.cbz(codeGen.X0, elseLabel, "Branch to else if condition is false");

    // Evaluate true expression
    codeGen.visitExpression(node->trueExpr.get());
    codeGen.instructions.b(endLabel, "Jump to end after true expression");

    // Else label
    codeGen.instructions.setPendingLabel(elseLabel);
    codeGen.labelManager.defineLabel(elseLabel, codeGen.instructions.getCurrentAddress());

    // Evaluate false expression
    codeGen.visitExpression(node->falseExpr.get());

    // End label
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());
}

void ExpressionCodeGenerator::visitValof(const Valof* node) {
    codeGen.visitStatement(node->body.get());
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