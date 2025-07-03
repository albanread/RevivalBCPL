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

    // *** FIX: Use the scratch allocator for temporary values ***
    uint32_t lhs_reg = codeGen.scratchAllocator.acquire();

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
            codeGen.instructions.sdiv(codeGen.X0, lhs_reg, rhs_reg, "Signed Division");
            break;
        case TokenType::OpRemainder:
            {
                uint32_t temp_div_reg = codeGen.scratchAllocator.acquire();
                codeGen.instructions.sdiv(temp_div_reg, lhs_reg, rhs_reg, "Temp for division in REM");
                codeGen.instructions.mul(temp_div_reg, temp_div_reg, rhs_reg, "Temp for (LHS/RHS)*RHS");
                codeGen.instructions.sub(codeGen.X0, lhs_reg, temp_div_reg, "Remainder");
                codeGen.scratchAllocator.release(temp_div_reg);
            }
            break;

        // Comparisons (result -1 for true, 0 for false)
        case TokenType::OpEq:
            codeGen.instructions.cmp(lhs_reg, rhs_reg, "Compare for equality");
            codeGen.instructions.cset(codeGen.X0, AArch64Instructions::EQ, "Set X0 if equal");
            codeGen.instructions.neg(codeGen.X0, codeGen.X0, "Convert 1 to -1 for true");
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
            codeGen.instructions.lslv(codeGen.X0, lhs_reg, rhs_reg, "Logical Left Shift by register");
            break;
        case TokenType::OpRshift:
            codeGen.instructions.lsrv(codeGen.X0, lhs_reg, rhs_reg, "Logical Right Shift by register");
            break;

        default:
            throw std::runtime_error("Unsupported binary operator: " + Token::tokenTypeToString(node->op));
    }

    codeGen.scratchAllocator.release(lhs_reg);
}

void ExpressionCodeGenerator::visitFunctionCall(const FunctionCall* node) {
    // Calculate the number of arguments that will be passed on the stack.
    size_t num_stack_args = 0;
    if (node->arguments.size() > 8) {
        num_stack_args = node->arguments.size() - 8;
    }

    // Update maxOutgoingParamSpace based on actual stack arguments.
    size_t currentCallParamBytes = num_stack_args * 8;
    if (currentCallParamBytes > codeGen.maxOutgoingParamSpace) {
        codeGen.maxOutgoingParamSpace = currentCallParamBytes;
    }

    codeGen.saveCallerSavedRegisters(); // Save caller-saved registers before argument evaluation.

    // Allocate space for stack arguments if necessary.
    if (num_stack_args > 0) {
        codeGen.instructions.sub(codeGen.SP, codeGen.SP, currentCallParamBytes, "Allocate space for outgoing arguments");
    }

    // Evaluate arguments and place them in registers or on the stack.
    // Evaluate in reverse order to avoid overwriting argument registers.
    for (int i = node->arguments.size() - 1; i >= 0; --i) {
        codeGen.visitExpression(node->arguments[i].get()); // Result is in X0

        if (i < 8) { // First 8 arguments go into registers X0-X7
            codeGen.instructions.mov(codeGen.X0 + i, codeGen.X0, "Move arg " + std::to_string(i) + " to X" + std::to_string(i));
        } else { // Arguments beyond the 8th go onto the stack
            // Stack arguments are pushed in order, so calculate offset from the beginning of the allocated block.
            size_t stack_offset_index = i - 8;
            codeGen.instructions.str(codeGen.X0, codeGen.SP, stack_offset_index * 8, "Store arg " + std::to_string(i) + " to stack");
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
        // Indirect function call through expression
        codeGen.visitExpression(node->function.get());
        codeGen.instructions.br(codeGen.X0);
    }

    // Deallocate stack arguments if necessary.
    if (num_stack_args > 0) {
        codeGen.instructions.add(codeGen.SP, codeGen.SP, currentCallParamBytes, "Deallocate outgoing arguments");
    }

    codeGen.restoreCallerSavedRegisters(); // Restore caller-saved registers after the call.
}

void ExpressionCodeGenerator::visitConditionalExpression(const ConditionalExpression* node) {
    auto elseLabel = codeGen.labelManager.generateLabel("cond_else");
    auto endLabel = codeGen.labelManager.generateLabel("cond_end");

    // Evaluate the condition
    codeGen.visitExpression(node->condition.get());
    codeGen.instructions.cmp(codeGen.X0, 0);
    codeGen.labelManager.requestLabelFixup(elseLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.beq(elseLabel);

    // If true, evaluate the 'then' expression
    codeGen.visitExpression(node->trueExpr.get());
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(endLabel);

    // If false, evaluate the 'else' expression
    codeGen.instructions.setPendingLabel(elseLabel);
    codeGen.labelManager.defineLabel(elseLabel, codeGen.instructions.getCurrentAddress());
    codeGen.visitExpression(node->falseExpr.get());

    // Define the end label
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());
}

void ExpressionCodeGenerator::visitValof(const Valof* node) {
    codeGen.visitStatement(node->body.get());
}

void ExpressionCodeGenerator::visitTableConstructor(const TableConstructor* node) {
    throw std::runtime_error("Table constructors not yet implemented.");
}

void ExpressionCodeGenerator::visitVectorConstructor(const VectorConstructor* node) {
    codeGen.visitExpression(node->size.get());
    // The size of the vector is now in X0.
    // We need to call the `bcpl_vec` runtime function to allocate the vector.
    codeGen.instructions.bl("bcpl_vec", "Allocate vector on heap");
    // The result of the allocation (the pointer to the vector) is in X0.
}

void ExpressionCodeGenerator::visitCharacterAccess(const CharacterAccess* node) {
    // Evaluate index first
    codeGen.visitExpression(node->index.get());
    // Save index
    uint32_t indexReg = codeGen.scratchAllocator.acquire();
    codeGen.instructions.mov(indexReg, codeGen.X0);

    // Evaluate string address
    codeGen.visitExpression(node->string.get());

    // Calculate final address and load
    codeGen.instructions.add(codeGen.X0, codeGen.X0, indexReg, AArch64Instructions::LSL, 2, "Calculate character address (4-byte chars)");
    codeGen.instructions.ldr(codeGen.X0, codeGen.X0, 0, "Load character");
    codeGen.scratchAllocator.release(indexReg);
}

void ExpressionCodeGenerator::visitStringAccess(const StringAccess* node) {
    // This is typically handled by visitVariableAccess for string literals
    // or by visitCharacterAccess for indexed access.
    // If this is meant for something else, it needs further implementation.
    throw std::runtime_error("ExpressionCodeGenerator::visitStringAccess not implemented for this context.");
}

// (Add this function at the end of the file with the other method implementations)

void ExpressionCodeGenerator::visitVectorAccess(const VectorAccess* node) {
    // 1. Evaluate the index expression. The result will be in X0.
    codeGen.visitExpression(node->index.get());

    // 2. Acquire a scratch register and save the index value into it.
    uint32_t indexReg = codeGen.scratchAllocator.acquire();
    codeGen.instructions.mov(indexReg, codeGen.X0, "Save index value");

    // 3. Evaluate the vector expression to get its base address in X0.
    codeGen.visitExpression(node->vector.get());
    uint32_t vectorBaseReg = codeGen.X0; // Base address is now in X0

    // 4. Calculate the final element's address.
    // Address = Base Address + (Index * 8)
    // The LSL #3 is a "Logical Shift Left" by 3 bits, which is a fast way to multiply by 8 (the size of each element).
    codeGen.instructions.add(vectorBaseReg, vectorBaseReg, indexReg, AArch64Instructions::LSL, 3, "Calculate vector element address");

    // 5. Load the value from the calculated address into X0.
    codeGen.instructions.ldr(codeGen.X0, vectorBaseReg, 0, "Load vector element value");

    // 6. Release the scratch register.
    codeGen.scratchAllocator.release(indexReg);
}
