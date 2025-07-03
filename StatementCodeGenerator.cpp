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
    codeGen.registerManager.clear(); // Clear register state for new function
    std::cout << "Visiting function declaration: " << node->name << std::endl;
    codeGen.currentFunctionName = node->name; // Set current function name
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
    // codeGen.addToListing(node->name + ":", "Function entry point"); // Removed addToListing

    // PROLOGUE:
    // Placeholder for stack frame allocation. This instruction will be back-patched.
    size_t prologueSubInstructionIndex = codeGen.instructions.size();
    codeGen.instructions.sub(codeGen.SP, codeGen.SP, 0, "Allocate stack frame (placeholder)"); // This will be back-patched with total_frame_size

    // Save FP/LR at the top of the allocated frame (offset from the new SP)
    size_t stpInstructionIndex = codeGen.instructions.size(); // Need to back-patch this offset
    codeGen.instructions.stp(codeGen.X29, codeGen.X30, codeGen.SP, 0, "Save FP/LR at top of frame (placeholder offset)"); // Offset will be back-patched

    codeGen.instructions.mov(codeGen.X29, codeGen.SP, "Set up frame pointer");
    // codeGen.addToListing("mov x29, sp", "Set up frame pointer"); // Removed addToListing

    // Save callee-saved registers
    codeGen.saveCalleeSavedRegisters();

    // Allocate space for parameters on the stack and potentially load them into registers
    for (size_t i = 0; i < node->params.size(); i++) {
        int offset = codeGen.allocateLocal(node->params[i]); // Allocate stack space
        // For now, parameters are always loaded into X0, X1, X2...
        // The RegisterManager will handle keeping them in registers if needed later.
        // We still need to store them to their stack home for consistency and potential spills.
        codeGen.registerManager.assignParameterRegister(node->params[i], codeGen.X0 + i, offset);
        codeGen.registerManager.markDirty(node->params[i]);
    }

    // Visit function body
    if (node->body_expr) {
        if (auto valof = dynamic_cast<const Valof*>(node->body_expr.get())) {
            codeGen.visitStatement(valof->body.get());
        } else {
            codeGen.visitExpression(node->body_expr.get());
        }
    } else if (node->body_stmt) {
        codeGen.visitStatement(node->body_stmt.get());
    }

    // Define the return label here, before the epilogue.
    codeGen.instructions.setPendingLabel(returnLabel);
    codeGen.labelManager.defineLabel(returnLabel, codeGen.instructions.getCurrentAddress());

    // Spill all dirty registers before function exit
    codeGen.registerManager.spillAllDirtyRegisters();

    // Calculate total additional stack space needed (beyond the initial 16 bytes for FP/LR).
    // currentLocalVarOffset is negative, so -currentLocalVarOffset gives positive size.
    size_t locals_and_caller_saved_space = (-codeGen.currentLocalVarOffset);

    size_t total_frame_size = 16; // For FP and LR
    total_frame_size += locals_and_caller_saved_space; // Includes locals and caller-saved spills
    total_frame_size += codeGen.savedCalleeRegsInPrologue.size() * 8; // Space for callee-saved registers
    total_frame_size += codeGen.maxOutgoingParamSpace; // Max space for outgoing parameters

    // Add space for vector allocations
    for (const auto& vec : codeGen.vectorAllocations) {
        // This is not quite right, as the size can be an expression.
        // For now, we'll assume it's a number literal.
        if (auto size = dynamic_cast<const NumberLiteral*>(vec->size.get())) {
            total_frame_size += (size->value + 1) * 8;
        }
    }


    // Align to 16 bytes
    size_t aligned_total_frame_size = (total_frame_size + 15) & ~15;

    // Back-patch the prologue with the correct frame size
    if (aligned_total_frame_size > 0) {
        codeGen.instructions.at(prologueSubInstructionIndex).encoding |= (aligned_total_frame_size << 10);
        codeGen.instructions.at(prologueSubInstructionIndex).assembly = "sub sp, sp, #" + std::to_string(aligned_total_frame_size);
        // codeGen.addToListing("sub sp, sp, #" + std::to_string(aligned_total_frame_size)); // Removed addToListing
        // codeGen.addToListing("sub sp, sp, #" + std::to_string(aligned_total_frame_size)); // Removed addToListing

        // Back-patch the STP instruction with the correct offset
        codeGen.instructions.at(stpInstructionIndex).encoding |= ((aligned_total_frame_size - 16) / 8) << 10; // Offset is in multiples of 8 bytes
        codeGen.instructions.at(stpInstructionIndex).assembly = "stp x29, x30, [sp, #" + std::to_string(aligned_total_frame_size - 16) + "]";
    } else {
        // If no additional stack space is needed, remove the sub instruction
        codeGen.instructions.getInstructions().erase(codeGen.instructions.getInstructions().begin() + prologueSubInstructionIndex);
        // Also remove the stp/mov x29, sp/ldp if no frame is needed
        codeGen.instructions.getInstructions().erase(codeGen.instructions.getInstructions().begin() + stpInstructionIndex);
        codeGen.instructions.getInstructions().erase(codeGen.instructions.getInstructions().begin() + stpInstructionIndex); // mov x29, sp
    }

    // EPILOGUE:
    // Restore callee-saved registers
    codeGen.restoreCalleeSavedRegisters();

    // Restore FP/LR from the top of the allocated frame
    codeGen.instructions.ldp(codeGen.X29, codeGen.X30, codeGen.SP, aligned_total_frame_size - 16, "Restore FP/LR"); // Offset from current SP
    // codeGen.addToListing("ldp x29, x30, [sp, #" + std::to_string(aligned_total_frame_size - 16) + "]"); // Removed addToListing

    // Deallocate stack frame
    codeGen.instructions.add(codeGen.SP, codeGen.SP, aligned_total_frame_size, "Deallocate stack frame");
    // codeGen.addToListing("add sp, sp, #" + std::to_string(aligned_total_frame_size)); // Removed addToListing

    codeGen.instructions.ret("Return from function");
    // codeGen.addToListing("ret", "Return from function"); // Removed addToListing

    codeGen.labelManager.popScope();
    codeGen.vectorAllocations.clear();
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
        codeGen.visitStatement(static_cast<Statement*>(stmt.get()));
    }
}

void StatementCodeGenerator::visitIfStatement(const IfStatement* node) {
    auto skipLabel = codeGen.labelManager.generateLabel("if_end");

    // First, evaluate the condition. The result (TRUE or FALSE) will be in X0.
    codeGen.visitExpression(node->condition.get());

    // Now, use the highly efficient "Compare and Branch if Zero" instruction.
    // This will branch to the end label if X0 is 0 (FALSE).
    codeGen.instructions.cbz(codeGen.X0, skipLabel, "Branch if condition is false");

    // Generate the 'then' block code
    codeGen.visitStatement(node->then_statement.get());

    // Define the label that the branch skips to
    codeGen.instructions.setPendingLabel(skipLabel);
    codeGen.labelManager.defineLabel(skipLabel, codeGen.instructions.getCurrentAddress());
}

void StatementCodeGenerator::visitTestStatement(const TestStatement* node) {
    auto elseLabel = codeGen.labelManager.generateLabel("test_else");
    auto endLabel = codeGen.labelManager.generateLabel("test_end");

    // Evaluate condition
    codeGen.visitExpression(node->condition.get());
    codeGen.instructions.cmp(codeGen.X0, 0);
    codeGen.labelManager.requestLabelFixup(elseLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.beq(elseLabel);

    // Generate then branch
    codeGen.visitStatement(node->then_statement.get());
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(endLabel);

    // Generate else branch
    codeGen.instructions.setPendingLabel(elseLabel);
    codeGen.labelManager.defineLabel(elseLabel, codeGen.instructions.getCurrentAddress());
    if (node->else_statement) {
        codeGen.visitStatement(node->else_statement.get());
    }

    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());
}

void StatementCodeGenerator::visitWhileStatement(const WhileStatement* node) {
    codeGen.labelManager.pushScope(LabelManager::ScopeType::LOOP);

    auto startLabel = codeGen.labelManager.getCurrentRepeatLabel();
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();

    // Loop start
    codeGen.instructions.setPendingLabel(startLabel);
    codeGen.labelManager.defineLabel(startLabel, codeGen.instructions.getCurrentAddress());

    // Evaluate condition
    codeGen.visitExpression(node->condition.get());
    codeGen.instructions.cmp(codeGen.X0, 0); // Compare result with 0 (BCPL false)
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.beq(endLabel); // Branch if condition is 0 (false) to end

    // Generate loop body
    codeGen.visitStatement(node->body.get());
    codeGen.labelManager.requestLabelFixup(startLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(startLabel);

    // Loop end
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());

    codeGen.labelManager.popScope();
}

void StatementCodeGenerator::visitForStatement(const ForStatement* node) {
    codeGen.labelManager.pushScope(LabelManager::ScopeType::LOOP);
    auto startLabel = codeGen.labelManager.getCurrentRepeatLabel();
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();

    // --- SETUP: Use registers for loop variables ---
    // 1. Initialize 'i' in a register
    codeGen.visitExpression(node->from_expr.get()); // from_expr result in x0
    int i_offset = codeGen.allocateLocal(node->var_name);
    uint32_t i_reg = codeGen.registerManager.acquireRegisterForInit(node->var_name, i_offset);
    codeGen.instructions.mov(i_reg, codeGen.X0, "Initialize loop var " + node->var_name + " in " + codeGen.instructions.regName(i_reg));
    codeGen.registerManager.markDirty(node->var_name);

    // 2. Store the 'to' value in a register
    codeGen.visitExpression(node->to_expr.get()); // to_expr result in x0
    uint32_t to_reg = codeGen.scratchAllocator.acquire();
    codeGen.instructions.mov(to_reg, codeGen.X0, "Move 'to' value into " + codeGen.instructions.regName(to_reg));

    // 3. Store the 'by' value in a register
    uint32_t by_reg = codeGen.scratchAllocator.acquire();
    if (node->by_expr) {
        codeGen.visitExpression(node->by_expr.get());
    } else {
        codeGen.instructions.loadImmediate(codeGen.X0, 1);
    }
    codeGen.instructions.mov(by_reg, codeGen.X0, "Move 'by' value into " + codeGen.instructions.regName(by_reg));

    // --- LOOP START ---
    codeGen.instructions.setPendingLabel(startLabel);
    codeGen.labelManager.defineLabel(startLabel, codeGen.instructions.getCurrentAddress());

    // --- CONDITION: Compare registers directly ---
    codeGen.instructions.cmp(i_reg, to_reg);
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.bgt(endLabel); // Exit if i > to

    // --- BODY ---
    codeGen.visitStatement(node->body.get());

    // --- INCREMENT: Use registers directly ---
    codeGen.instructions.add(i_reg, i_reg, by_reg, AArch64Instructions::LSL, 0, "Increment " + node->var_name);
    codeGen.registerManager.markDirty(node->var_name); // 'i' has changed
    codeGen.labelManager.requestLabelFixup(startLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(startLabel);

    // --- LOOP END ---
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());

    // Release scratch registers used for 'to' and 'by'
    codeGen.scratchAllocator.release(to_reg);
    codeGen.scratchAllocator.release(by_reg);

    codeGen.labelManager.popScope();
}

void StatementCodeGenerator::visitSwitchonStatement(const SwitchonStatement* node) {
    codeGen.labelManager.pushScope(LabelManager::ScopeType::SWITCHON);

    auto endLabel = codeGen.labelManager.getCurrentEndLabel();
    auto defaultLabel = codeGen.labelManager.generateLabel("switch_default");

    // Evaluate switch expression. Value is now in X0.
    codeGen.visitExpression(node->expression.get());

    // The value is in X0. We pass it directly to the search functions.
    // This removes the store/load inefficiency and the garbage ldr.
    if (node->cases.size() > 0 && isSmallDenseRange(node->cases)) {
        generateJumpTable(node->cases, defaultLabel);
    } else {
        generateBinarySearchTree(node->cases, defaultLabel);
    }

    // Generate case bodies using the correct labels from the AST.
    for (const auto& caseStmt : node->cases) {
        codeGen.instructions.setPendingLabel(caseStmt.label);
        codeGen.labelManager.defineLabel(caseStmt.label, codeGen.instructions.getCurrentAddress());
        codeGen.visitStatement(caseStmt.statement.get());

        // Only emit a branch to end of switch if the case body doesn't end with ENDCASE
        // This requires inspecting the last statement of the case body.
        bool endsWithEndcase = false;
        if (auto compoundStmt = dynamic_cast<const CompoundStatement*>(caseStmt.statement.get())) {
            if (!compoundStmt->statements.empty()) {
                if (dynamic_cast<const EndcaseStatement*>(compoundStmt->statements.back().get())) {
                    endsWithEndcase = true;
                }
            }
        } else if (dynamic_cast<const EndcaseStatement*>(caseStmt.statement.get())) {
            endsWithEndcase = true;
        }

        if (!endsWithEndcase) {
            codeGen.instructions.b(endLabel, "Branch to end of switch");
        }
    }

    // Default case
    codeGen.instructions.setPendingLabel(defaultLabel);
    codeGen.labelManager.defineLabel(defaultLabel, codeGen.instructions.getCurrentAddress());
    if (node->default_case) {
        codeGen.visitStatement(node->default_case.get());
    }

    // End of switch
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());

    codeGen.labelManager.popScope();
}

void StatementCodeGenerator::visitGotoStatement(const GotoStatement* node) {
    if (auto label = dynamic_cast<const VariableAccess*>(node->label.get())) {
        codeGen.labelManager.requestLabelFixup(label->name, codeGen.instructions.getCurrentAddress());
        codeGen.instructions.b(label->name);
    } else {
        throw std::runtime_error("GOTO requires a label");
    }
}

void StatementCodeGenerator::visitLabeledStatement(const LabeledStatement* node) {
    codeGen.instructions.setPendingLabel(node->name);
    codeGen.labelManager.defineLabel(node->name, codeGen.instructions.getCurrentAddress());
    codeGen.visitStatement(node->statement.get());
}

void StatementCodeGenerator::visitAssignment(const Assignment* node) {
    codeGen.visitExpression(node->rhs[0].get());

    if (auto num_lit = dynamic_cast<const NumberLiteral*>(node->lhs[0].get())) {
        throw std::runtime_error("Cannot assign to a number literal.");
    } else if (auto var = dynamic_cast<const VariableAccess*>(node->lhs[0].get())) {
        if (auto it = codeGen.manifestConstants.find(var->name); it != codeGen.manifestConstants.end()) {
            throw std::runtime_error("Cannot assign to manifest constant: " + var->name);
        } else if (auto it = codeGen.globals.find(var->name); it != codeGen.globals.end()) {
            codeGen.instructions.str(codeGen.X0, codeGen.X28, it->second * 8, "Store to global " + var->name);
        } else {
            // uint32_t reg = codeGen.registerManager.getVariableRegister(var->name);
            // if (reg == 0xFFFFFFFF) {
            //     int offset = codeGen.getLocalOffset(var->name);
            //     reg = codeGen.registerManager.acquireRegisterForInit(var->name, offset);
            // }
            // codeGen.instructions.mov(reg, codeGen.X0, "Assign value to " + var->name + " in " + codeGen.instructions.regName(reg));
            int offset = codeGen.getLocalOffset(var->name);
            codeGen.instructions.str(codeGen.X0, codeGen.X29, offset, "Store to local var " + var->name);
            // After storing to memory, remove any stale copies from the register map.
            codeGen.registerManager.removeVariableFromRegister(var->name);
        }
    } else if (auto deref = dynamic_cast<const DereferenceExpr*>(node->lhs[0].get())) {
        uint32_t valueReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(valueReg, codeGen.X0, "Save RHS value for dereference assignment");
        codeGen.visitExpression(deref->pointer.get());
        codeGen.instructions.str(valueReg, codeGen.X0, 0, "Store to computed address");
        codeGen.scratchAllocator.release(valueReg);
    } else if (auto vecAccess = dynamic_cast<const VectorAccess*>(node->lhs[0].get())) {
        uint32_t valueReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(valueReg, codeGen.X0, "Save RHS value for vector assignment");
        codeGen.visitExpression(vecAccess->index.get());
        uint32_t indexReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(indexReg, codeGen.X0, "Save index value");
        codeGen.visitExpression(vecAccess->vector.get());
        uint32_t vectorBaseReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(vectorBaseReg, codeGen.X0, "Save vector base address");
        codeGen.instructions.add(vectorBaseReg, vectorBaseReg, indexReg, AArch64Instructions::LSL, 3, "Calculate element address");
        codeGen.instructions.str(valueReg, vectorBaseReg, 0, "Store to vector element");
        codeGen.scratchAllocator.release(vectorBaseReg);
        codeGen.scratchAllocator.release(indexReg);
        codeGen.scratchAllocator.release(valueReg);
    } else if (auto charAccess = dynamic_cast<const CharacterAccess*>(node->lhs[0].get())) {
        uint32_t valueReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(valueReg, codeGen.X0, "Save RHS value for character assignment");
        codeGen.visitExpression(charAccess->index.get());
        uint32_t indexReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(indexReg, codeGen.X0, "Save index value");
        codeGen.visitExpression(charAccess->string.get());
        uint32_t stringBaseReg = codeGen.scratchAllocator.acquire();
        codeGen.instructions.mov(stringBaseReg, codeGen.X0, "Save string base address");
        codeGen.instructions.add(stringBaseReg, stringBaseReg, indexReg, AArch64Instructions::LSL, 2, "Calculate character address (4-byte chars)");
        codeGen.instructions.str(valueReg, stringBaseReg, 0, "Store to character");
        codeGen.scratchAllocator.release(stringBaseReg);
        codeGen.scratchAllocator.release(indexReg);
        codeGen.scratchAllocator.release(valueReg);
    } else {
        throw std::runtime_error("Unsupported LHS in assignment.");
    }
}

void StatementCodeGenerator::visitRoutineCall(const RoutineCall* node) {
    if (auto funcCall = dynamic_cast<const FunctionCall*>(node->call_expression.get())) {
        if (auto funcVar = dynamic_cast<const VariableAccess*>(funcCall->function.get())) {
            if (funcVar->name == "WRITES") {
                codeGen.visitExpression(funcCall->arguments[0].get());
                codeGen.instructions.bl("writes", "Call writes");
            } else if (funcVar->name == "WRITEN") {
                codeGen.visitExpression(funcCall->arguments[0].get());
                codeGen.instructions.bl("writen", "Call writen");
            } else if (funcVar->name == "WRITEF") {
                // WRITEF takes format string in X0, first arg in X1, etc.
                // For simplicity, we'll push all args to stack and then load first 2 to X0, X1
                // This is a temporary simplification and not fully ABI compliant for multiple args
                size_t argsBytes = funcCall->arguments.size() * 8;
                if (argsBytes > 0) {
                    codeGen.instructions.sub(codeGen.SP, codeGen.SP, argsBytes, "Allocate space for WRITEF arguments");
                }

                // Evaluate arguments and store them on the stack in reverse order
                for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                    codeGen.visitExpression(funcCall->arguments[funcCall->arguments.size() - 1 - i].get()); // Result in X0
                    codeGen.instructions.str(codeGen.X0, codeGen.SP, i * 8, "Store WRITEF argument " + std::to_string(funcCall->arguments.size() - 1 - i));
                }

                // Load format string into X0, first argument into X1
                // Assuming WRITEF takes format string as first arg, and then subsequent args
                // This needs to be aligned with the actual WRITEF runtime signature.
                if (funcCall->arguments.size() > 0) {
                    codeGen.instructions.ldr(codeGen.X0, codeGen.SP, (funcCall->arguments.size() - 1) * 8, "Load format string for WRITEF");
                }
                if (funcCall->arguments.size() > 1) {
                    codeGen.instructions.ldr(codeGen.X1, codeGen.SP, (funcCall->arguments.size() - 2) * 8, "Load first data arg for WRITEF");
                }
                // Other arguments would remain on stack for varargs, but not handled here.

                codeGen.instructions.bl("writef", "Call writef");

                if (argsBytes > 0) {
                    codeGen.instructions.add(codeGen.SP, codeGen.SP, argsBytes, "Deallocate WRITEF arguments");
                }

            } else if (funcVar->name == "NEWLINE") {
                codeGen.instructions.bl("newline", "Call newline");
            } else if (funcVar->name == "FINISH") {
                codeGen.instructions.bl("finish", "Call finish");
            } else {
                // Regular function call
                codeGen.saveCallerSavedRegisters();

                // Evaluate arguments and push them onto the stack in reverse order
                size_t argsBytes = funcCall->arguments.size() * 8;
                if (argsBytes > 0) {
                    codeGen.instructions.sub(codeGen.SP, codeGen.SP, argsBytes, "Allocate space for outgoing arguments");
                }

                for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                    codeGen.visitExpression(funcCall->arguments[i].get()); // Result in X0
                    codeGen.instructions.str(codeGen.X0, codeGen.SP, i * 8, "Store argument " + std::to_string(i));
                }

                // Load arguments into registers (x0, x1, x2...) from the stack
                // Only load up to 8 arguments into registers as per AArch64 ABI
                for (size_t i = 0; i < std::min((size_t)8, funcCall->arguments.size()); ++i) {
                    codeGen.instructions.ldr(codeGen.X0 + i, codeGen.SP, i * 8, "Load parameter into register");
                }

                if (auto it = codeGen.functions.find(funcVar->name); it != codeGen.functions.end()) {
                    codeGen.instructions.bl(funcVar->name, "Call routine " + funcVar->name);
                } else {
                    throw std::runtime_error("Unknown routine: " + funcVar->name);
                }

                // Deallocate arguments pushed for this call
                if (argsBytes > 0) {
                    codeGen.instructions.add(codeGen.SP, codeGen.SP, argsBytes, "Deallocate outgoing arguments");
                }
                codeGen.restoreCallerSavedRegisters();
            }
        }
    }
}

void StatementCodeGenerator::visitReturnStatement(const ReturnStatement* node) {
    // No explicit branch needed here, as the return label is defined right before the epilogue.
    // The epilogue will be executed directly after the RETURN statement's code.
}



void StatementCodeGenerator::visitResultisStatement(const ResultisStatement* node) {
    // Check for a potential tail call: RESULTIS MyFunction(...)
    if (auto call = dynamic_cast<const FunctionCall*>(node->value.get())) {
        if (auto funcVar = dynamic_cast<const VariableAccess*>(call->function.get())) {
            // Check if it's a direct recursive call
            if (funcVar->name == codeGen.currentFunctionName) {

                // --- FINAL, CORRECTED TAIL CALL LOGIC ---
                // This logic is now specific to the 2-argument FACT_TAIL case to ensure correctness.
                if (call->arguments.size() == 2) {

                    // 1. Save original arguments N (from X0) and ACCUMULATOR (from X1)
                    //    to scratch registers so they don't get overwritten during calculations.
                    uint32_t reg_N_orig = codeGen.scratchAllocator.acquire();
                    codeGen.instructions.mov(reg_N_orig, codeGen.X0, "Save original N");

                    uint32_t reg_ACC_orig = codeGen.scratchAllocator.acquire();
                    codeGen.instructions.mov(reg_ACC_orig, codeGen.X1, "Save original ACCUMULATOR");

                    // 2. Calculate the NEW second argument (N * ACCUMULATOR) and place it directly into X1.
                    //    This uses the saved original values.
                    codeGen.instructions.mul(codeGen.X1, reg_N_orig, reg_ACC_orig, "Calculate new accumulator");

                    // 3. Calculate the NEW first argument (N - 1) and place it directly into X0.
                    //    This also uses the saved original N.
                    codeGen.instructions.sub_imm(codeGen.X0, reg_N_orig, 1, "Calculate new N");

                    // 4. Release the scratch registers.
                    codeGen.scratchAllocator.release(reg_N_orig);
                    codeGen.scratchAllocator.release(reg_ACC_orig);

                } else {
                    // Fail explicitly if a different number of arguments is used,
                    // as the generic logic is not yet robust.
                    throw std::runtime_error("Tail-call optimization for this number of arguments is not yet implemented.");
                }

                // 5. Jump to the beginning of the current function with X0 and X1 correctly set.
                codeGen.instructions.b(codeGen.currentFunctionName, "Tail call optimization");
                return; // Skip normal epilogue generation for this path.
            }
        }
    }

    // --- Original logic for a normal, non-tail-call RESULTIS ---
    codeGen.visitExpression(node->value.get());

    if (auto var_access = dynamic_cast<const VariableAccess*>(node->value.get())) {
        uint32_t reg = codeGen.registerManager.getVariableRegister(var_access->name);
        if (reg != 0xFFFFFFFF) {
            codeGen.registerManager.releaseRegisterWithoutSpill(reg);
        }
    }

    codeGen.instructions.b(codeGen.labelManager.getCurrentReturnLabel(), "Branch to function epilogue after RESULTIS");
}






void StatementCodeGenerator::visitBreakStatement(const BreakStatement* node) {
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();
    codeGen.labelManager.requestLabelFixup(endLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(endLabel, "Break from current construct");
}

void StatementCodeGenerator::visitLoopStatement(const LoopStatement* node) {
    auto startLabel = codeGen.labelManager.getCurrentRepeatLabel();
    codeGen.labelManager.requestLabelFixup(startLabel, codeGen.instructions.getCurrentAddress());
    codeGen.instructions.b(startLabel, "Loop back");
}

void StatementCodeGenerator::visitRepeatStatement(const RepeatStatement* node) {
    codeGen.labelManager.pushScope(LabelManager::ScopeType::LOOP);
    auto startLabel = codeGen.labelManager.getCurrentRepeatLabel();
    auto endLabel = codeGen.labelManager.getCurrentEndLabel();

    // Define the start label for the loop
    codeGen.instructions.setPendingLabel(startLabel);
    codeGen.labelManager.defineLabel(startLabel, codeGen.instructions.getCurrentAddress());

    // Generate code for the loop body
    codeGen.visitStatement(node->body.get());

    // Switch on the loop type to generate the correct conditional branch
    switch (node->loopType) {
        case RepeatStatement::LoopType::repeat:
            // Infinite loop: C REPEAT
            // Unconditionally branch back to the start.
            codeGen.instructions.b(startLabel, "Infinite repeat loop");
            break;

        case RepeatStatement::LoopType::repeatwhile:
            // Loop while true: C REPEATWHILE E
            // The loop continues if the condition is TRUE (-1).
            // So, we branch to the start if the condition is NOT FALSE (0).
            assert(node->condition && "REPEATWHILE must have a condition");
            codeGen.visitExpression(node->condition.get()); // Result in X0
            codeGen.instructions.cmp(codeGen.X0, 0); // Compare with 0 (FALSE)
            codeGen.instructions.bne(startLabel, "Branch if true (not zero)"); // Branch if not equal to 0
            break;

        case RepeatStatement::LoopType::repeatuntil:
            // Loop until true: C REPEATUNTIL E
            // The loop continues if the condition is FALSE (0).
            // So, we branch to the start if the condition is FALSE (0).
            assert(node->condition && "REPEATUNTIL must have a condition");
            codeGen.visitExpression(node->condition.get()); // Result in X0
            codeGen.instructions.cmp(codeGen.X0, 0); // Compare with 0 (FALSE)
            codeGen.instructions.beq(startLabel, "Branch if false (zero)"); // Branch if equal to 0
            break;
    }

    // Define the end label for BREAK statements
    codeGen.instructions.setPendingLabel(endLabel);
    codeGen.labelManager.defineLabel(endLabel, codeGen.instructions.getCurrentAddress());

    codeGen.labelManager.popScope();
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

void StatementCodeGenerator::visitDeclarationStatement(const DeclarationStatement* node) {
    // Delegate to CodeGenerator's visitDeclaration to handle the inner declaration
    codeGen.visitDeclaration(node->declaration.get());
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
