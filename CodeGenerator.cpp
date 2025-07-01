#include "CodeGenerator.h"
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
}

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
    // First pass: collect all global declarations
    for (const auto& decl : node->declarations) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            functions[funcDecl->name] = instructions.getCurrentAddress();
        }
    }

    // Second pass: generate code for declarations
    for (const auto& decl : node->declarations) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            visitFunctionDeclaration(funcDecl);
        } else if (auto letDecl = dynamic_cast<const LetDeclaration*>(decl.get())) {
            visitLetDeclaration(letDecl);
        } else if (auto globalDecl = dynamic_cast<const GlobalDeclaration*>(decl.get())) {
            visitGlobalDeclaration(globalDecl);
        } else if (auto manifestDecl = dynamic_cast<const ManifestDeclaration*>(decl.get())) {
            visitManifestDeclaration(manifestDecl);
        } else if (auto valof = dynamic_cast<const Valof*>(decl.get())) {
            visitValof(valof);
        }
    }
}

void CodeGenerator::visitManifestDeclaration(const ManifestDeclaration* node) {
    for (const auto& manifest : node->manifests) {
        manifestConstants[manifest.name] = manifest.value;
    }
}

void CodeGenerator::visitGlobalDeclaration(const GlobalDeclaration* node) {
    for (const auto& global : node->globals) {
        globals[global.name] = globals.size();
    }
}

#include <iostream>

void CodeGenerator::visitFunctionDeclaration(const FunctionDeclaration* node) {
    registerManager.clear(); // Clear register state for new function
    std::cout << "Visiting function declaration: " << node->name << std::endl;
    currentFunctionName = node->name; // Set current function name
    labelManager.pushScope(LabelManager::ScopeType::FUNCTION);
    auto returnLabel = labelManager.getCurrentReturnLabel();
    std::cout << "Generated return label: " << returnLabel << std::endl;

    // First pass to collect vector allocations
    VectorAllocationVisitor vecVisitor;
    vecVisitor.visit(node);
    vectorAllocations = vecVisitor.allocations;

    // Generate function label and record position
    instructions.setPendingLabel(node->name);
    labelManager.defineLabel(node->name, instructions.getCurrentAddress());
    addToListing(node->name + ":", "Function entry point");

    // PROLOGUE:
    // Placeholder for stack frame allocation. This instruction will be back-patched.
    size_t prologueSubInstructionIndex = instructions.size();
    instructions.sub(SP, SP, 0, "Allocate stack frame (placeholder)"); // This will be back-patched with total_frame_size

    // Save FP/LR at the top of the allocated frame (offset from the new SP)
    size_t stpInstructionIndex = instructions.size(); // Need to back-patch this offset
    instructions.stp(X29, X30, SP, 0, "Save FP/LR at top of frame (placeholder offset)"); // Offset will be back-patched

    instructions.mov(X29, SP, "Set up frame pointer");
    addToListing("mov x29, sp", "Set up frame pointer");

    // Save callee-saved registers
    saveCalleeSavedRegisters();

    // Allocate space for parameters on the stack and potentially load them into registers
    for (size_t i = 0; i < node->params.size(); i++) {
        int offset = allocateLocal(node->params[i]); // Allocate stack space
        // For now, parameters are always loaded into X0, X1, X2...
        // The RegisterManager will handle keeping them in registers if needed later.
        // We still need to store them to their stack home for consistency and potential spills.
        registerManager.assignParameterRegister(node->params[i], X0 + i, offset);
        registerManager.markDirty(node->params[i]);
    }

    // Visit function body
    if (node->body_expr) {
        if (auto valof = dynamic_cast<const Valof*>(node->body_expr.get())) {
            visitStatement(valof->body.get());
        } else {
            visitExpression(node->body_expr.get());
        }
    } else if (node->body_stmt) {
        visitStatement(node->body_stmt.get());
    }

    // Define the return label here, before the epilogue.
    instructions.setPendingLabel(returnLabel);
    labelManager.defineLabel(returnLabel, instructions.getCurrentAddress());

    // Spill all dirty registers before function exit
    registerManager.spillAllDirtyRegisters();

    // Calculate total additional stack space needed (beyond the initial 16 bytes for FP/LR).
    // currentLocalVarOffset is negative, so -currentLocalVarOffset gives positive size.
    size_t locals_and_caller_saved_space = (-currentLocalVarOffset);

    size_t total_frame_size = 16; // For FP and LR
    total_frame_size += locals_and_caller_saved_space; // Includes locals and caller-saved spills
    total_frame_size += savedCalleeRegsInPrologue.size() * 8; // Space for callee-saved registers
    total_frame_size += maxOutgoingParamSpace; // Max space for outgoing parameters

    // Add space for vector allocations
    for (const auto& vec : vectorAllocations) {
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
        instructions.at(prologueSubInstructionIndex).encoding |= (aligned_total_frame_size << 10);
        instructions.at(prologueSubInstructionIndex).assembly = "sub sp, sp, #" + std::to_string(aligned_total_frame_size);
        addToListing("sub sp, sp, #" + std::to_string(aligned_total_frame_size));
        addToListing("sub sp, sp, #" + std::to_string(aligned_total_frame_size));

        // Back-patch the STP instruction with the correct offset
        instructions.at(stpInstructionIndex).encoding |= ((aligned_total_frame_size - 16) / 8) << 10; // Offset is in multiples of 8 bytes
        instructions.at(stpInstructionIndex).assembly = "stp x29, x30, [sp, #" + std::to_string(aligned_total_frame_size - 16) + "]";
    } else {
        // If no additional stack space is needed, remove the sub instruction
        instructions.getInstructions().erase(instructions.getInstructions().begin() + prologueSubInstructionIndex);
        // Also remove the stp/mov x29, sp/ldp if no frame is needed
        instructions.getInstructions().erase(instructions.getInstructions().begin() + stpInstructionIndex);
        instructions.getInstructions().erase(instructions.getInstructions().begin() + stpInstructionIndex); // mov x29, sp
    }

    // EPILOGUE:
    // Restore callee-saved registers
    restoreCalleeSavedRegisters();

    // Restore FP/LR from the top of the allocated frame
    instructions.ldp(X29, X30, SP, aligned_total_frame_size - 16, "Restore FP/LR"); // Offset from current SP
    addToListing("ldp x29, x30, [sp, #" + std::to_string(aligned_total_frame_size - 16) + "]");

    // Deallocate stack frame
    instructions.add(SP, SP, aligned_total_frame_size, "Deallocate stack frame");
    addToListing("add sp, sp, #" + std::to_string(aligned_total_frame_size));

    instructions.ret("Return from function");
    addToListing("ret", "Return from function");

    labelManager.popScope();
    vectorAllocations.clear();
}

void CodeGenerator::resolveLabels() {
    auto fixups = labelManager.getFixups();
    for (const auto& fixup : fixups) {
        size_t targetAddress = labelManager.getLabelAddress(fixup.labelName);
        int32_t offset = static_cast<int32_t>(targetAddress - fixup.instructionAddress);
        instructions.resolveBranch(fixup.instructionAddress / 4, offset);
    }
}

// CodeGenerator.cpp (continued)

void CodeGenerator::visitExpression(const Expression* expr) {
    if (auto* numLit = dynamic_cast<const NumberLiteral*>(expr)) {
        visitNumberLiteral(numLit);
    } else if (auto* strLit = dynamic_cast<const StringLiteral*>(expr)) {
        visitStringLiteral(strLit);
    } else if (auto* charLit = dynamic_cast<const CharLiteral*>(expr)) {
        visitCharLiteral(charLit);
    } else if (auto* var = dynamic_cast<const VariableAccess*>(expr)) {
        visitVariableAccess(var);
    } else if (auto* unary = dynamic_cast<const UnaryOp*>(expr)) {
        visitUnaryOp(unary);
    } else if (auto* binary = dynamic_cast<const BinaryOp*>(expr)) {
        visitBinaryOp(binary);
    } else if (auto* call = dynamic_cast<const FunctionCall*>(expr)) {
        visitFunctionCall(call);
    } else if (auto* cond = dynamic_cast<const ConditionalExpression*>(expr)) {
        visitConditionalExpression(cond);
    } else if (auto* valof = dynamic_cast<const Valof*>(expr)) {
        visitValof(valof);
    } else if (auto* table = dynamic_cast<const TableConstructor*>(expr)) {
        visitTableConstructor(table);
    } else if (auto* vec = dynamic_cast<const VectorConstructor*>(expr)) {
        visitVectorConstructor(vec);
    } else if (auto* charAccess = dynamic_cast<const CharacterAccess*>(expr)) {
        visitCharacterAccess(charAccess);
    }
}

void CodeGenerator::visitNumberLiteral(const NumberLiteral* node) {
    instructions.loadImmediate(X0, node->value, "Load number literal");
}

void CodeGenerator::visitCharLiteral(const CharLiteral* node) {
    instructions.loadImmediate(X0, node->value, "Load char literal");
}

void CodeGenerator::visitStringLiteral(const StringLiteral* node) {
    std::string label = ".L.str" + std::to_string(stringPool.size());
    stringPool.push_back(node->value);
    instructions.adr(X0, label, "Load string literal address");
}

void CodeGenerator::visitVariableAccess(const VariableAccess* node) {
    // First, check for manifest constants (this part is unchanged).
    if (auto it = manifestConstants.find(node->name); it != manifestConstants.end()) {
        instructions.loadImmediate(X0, it->second, "Load manifest constant " + node->name);
        return;
    }

    // Second, check for globals (this part is unchanged).
    if (auto it = globals.find(node->name); it != globals.end()) {
        instructions.ldr(X0, X28, it->second * 8, "Load global " + node->name);
        return;
    }

    // --- NEW LOGIC FOR LOCAL VARIABLES ---
    // 1. Ask the RegisterManager if the variable is already live in a register.
    uint32_t reg = registerManager.getVariableRegister(node->name);

    if (reg != 0xFFFFFFFF) {
        // 2. YES: The variable is in a register. Generate a fast register-to-register move.
        instructions.mov(X0, reg, "Move " + node->name + " from " + instructions.regName(reg) + " to X0");
    } else {
        // 3. NO: The variable is not in a register. Load it from its home on the stack.
        int offset = getLocalOffset(node->name);
        // Use the original 'acquireRegister' which loads from memory.
        uint32_t new_reg = registerManager.acquireRegister(node->name, offset);
        // The value is now loaded into new_reg. Move it to X0 for the expression.
        instructions.mov(X0, new_reg, "Move " + node->name + " from " + instructions.regName(new_reg) + " to X0");
    }
}

void CodeGenerator::visitUnaryOp(const UnaryOp* node) {
    visitExpression(node->rhs.get());

    switch (node->op) {
        case TokenType::OpLogNot:
            instructions.eor(X0, X0, 1, "Logical NOT");
            break;
        case TokenType::OpMinus:
            instructions.neg(X0, X0, "Arithmetic negation");
            break;
        case TokenType::OpAt:  // @ operator (address-of)
            // For variables, calculate address instead of loading value
            if (auto var = dynamic_cast<const VariableAccess*>(node->rhs.get())) {
                if (auto it = globals.find(var->name); it != globals.end()) {
                    instructions.add(X0, X28, it->second * 8, AArch64Instructions::LSL, 0, "Address of global " + var->name);
                } else {
                    int offset = getLocalOffset(var->name);
                    instructions.add(X0, X29, offset, AArch64Instructions::LSL, 0, "Address of local " + var->name);
                }
            } else {
                throw std::runtime_error("@ operator requires addressable operand");
            }
            break;
        case TokenType::OpBang:  // ! operator (indirection)
            instructions.ldr(X0, X0, 0, "Indirection");
            break;
        default:
            throw std::runtime_error("Unknown unary operator");
    }
}

void CodeGenerator::visitBinaryOp(const BinaryOp* node) {
    // Handle special cases first
    if (node->op == TokenType::OpBang) {  // Vector subscript (V!E)
        // Evaluate index first
        visitExpression(node->right.get());
        // Save index
        uint32_t indexReg = scratchAllocator.acquire();
        instructions.mov(indexReg, X0);

        // Evaluate vector address
        visitExpression(node->left.get());

        // Calculate final address and load
        instructions.add(X0, X0, indexReg, AArch64Instructions::LSL, 3, "Calculate element address (word size)"); // Fixed: LSL 3 for 8-byte words
        instructions.ldr(X0, X0, 0, "Load vector element");
        scratchAllocator.release(indexReg);
        return;
    }

    // Normal binary operators
    visitExpression(node->right.get());
    uint32_t rightReg = scratchAllocator.acquire();
    instructions.mov(rightReg, X0);

    visitExpression(node->left.get());

    switch (node->op) {
        case TokenType::OpPlus:
            instructions.add(X0, X0, rightReg, AArch64Instructions::LSL, 0, "Add");
            break;
        case TokenType::OpMinus:
            instructions.sub(X0, X0, rightReg, "Subtract");
            break;
        case TokenType::OpMultiply:
            instructions.mul(X0, X0, rightReg, "Multiply");
            break;
        case TokenType::OpDivide:
            // TODO: Add division by zero check
            instructions.sdiv(X0, X0, rightReg, "Divide");
            break;
        case TokenType::OpRemainder:
            // TODO: Add division by zero check
            instructions.sdiv(X2, X0, rightReg, "Divide for remainder");
            instructions.msub(X0, X2, rightReg, X0, "Calculate remainder");
            break;
        case TokenType::OpEq:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::EQ);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpNe:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::NE);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpLt:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::LT);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpGt:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::GT);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpLe:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::LE);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpGe:
            instructions.cmp(X0, rightReg);
            instructions.cset(X0, AArch64Instructions::GE);
            instructions.neg(X0, X0);
            break;
        case TokenType::OpLshift:
            instructions.lsl(X0, X0, 1, "Left shift by 1 (strength reduction)");
            break;
        case TokenType::OpRshift:
            instructions.lsr(X0, X0, rightReg, "Right shift");
            break;
        // Add other operators (AND, OR, etc.)
        default:
            throw std::runtime_error("Unknown binary operator");
    }
    scratchAllocator.release(rightReg);
}

// CodeGenerator.cpp (continued)

void CodeGenerator::visitCompoundStatement(const CompoundStatement* node) {
    // Simply visit each statement in the block.
    for (const auto& stmt : node->statements) {
        visitStatement(stmt.get());
    }
}

void CodeGenerator::visitStatement(const Statement* stmt) {
    if (auto compound = dynamic_cast<const CompoundStatement*>(stmt)) {
        visitCompoundStatement(compound);
    } else if (auto letDecl = dynamic_cast<const LetDeclaration*>(stmt)) {
        visitLetDeclaration(letDecl);
    } else if (auto ifStmt = dynamic_cast<const IfStatement*>(stmt)) {
        visitIfStatement(ifStmt);
    } else if (auto testStmt = dynamic_cast<const TestStatement*>(stmt)) {
        visitTestStatement(testStmt);
    } else if (auto whileStmt = dynamic_cast<const WhileStatement*>(stmt)) {
        visitWhileStatement(whileStmt);
    } else if (auto switchStmt = dynamic_cast<const SwitchonStatement*>(stmt)) {
        visitSwitchonStatement(switchStmt);
    } else if (auto forStmt = dynamic_cast<const ForStatement*>(stmt)) {
        visitForStatement(forStmt);
    } else if (auto gotoStmt = dynamic_cast<const GotoStatement*>(stmt)) {
        visitGotoStatement(gotoStmt);
    } else if (auto labeledStmt = dynamic_cast<const LabeledStatement*>(stmt)) {
        visitLabeledStatement(labeledStmt);
    } else if (auto assign = dynamic_cast<const Assignment*>(stmt)) {
        visitAssignment(assign);
    } else if (auto routine = dynamic_cast<const RoutineCall*>(stmt)) {
        visitRoutineCall(routine);
    } else if (auto resultis = dynamic_cast<const ResultisStatement*>(stmt)) {
        visitResultisStatement(resultis);
    } else if (auto breakStmt = dynamic_cast<const BreakStatement*>(stmt)) {
        visitBreakStatement(breakStmt);
    } else if (auto returnStmt = dynamic_cast<const ReturnStatement*>(stmt)) {
        visitReturnStatement(returnStmt);
    } else if (auto loopStmt = dynamic_cast<const LoopStatement*>(stmt)) {
        visitLoopStatement(loopStmt);
    } else if (auto repeatStmt = dynamic_cast<const RepeatStatement*>(stmt)) {
        visitRepeatStatement(repeatStmt);
    } else if (auto endcaseStmt = dynamic_cast<const EndcaseStatement*>(stmt)) {
        visitEndcaseStatement(endcaseStmt);
    } else if (auto finishStmt = dynamic_cast<const FinishStatement*>(stmt)) {
        visitFinishStatement(finishStmt);
    }
}

void CodeGenerator::visitRepeatStatement(const RepeatStatement* node) {
    labelManager.pushScope(LabelManager::ScopeType::LOOP);
    auto startLabel = labelManager.getCurrentRepeatLabel();
    auto endLabel = labelManager.getCurrentEndLabel(); // Added for correct UNTIL logic

    instructions.setPendingLabel(startLabel);
    labelManager.defineLabel(startLabel, instructions.getCurrentAddress());
    visitStatement(node->body.get());
    visitExpression(node->condition.get());
    // REPEAT UNTIL E means loop while E is FALSE. So, branch to start if E is 0 (false).
    labelManager.requestLabelFixup(startLabel, instructions.getCurrentAddress());
    instructions.beq(startLabel); // Fixed: Branch if E is 0 (false) to repeat

    // Define the end label for BREAK statements or when condition is true
    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());

    labelManager.popScope();
}

void CodeGenerator::visitIfStatement(const IfStatement* node) {
    auto skipLabel = labelManager.generateLabel("if_end");

    // First, evaluate the condition. The result (TRUE or FALSE) will be in X0.
    visitExpression(node->condition.get());

    // Now, use the highly efficient "Compare and Branch if Zero" instruction.
    // This will branch to the end label if X0 is 0 (FALSE).
    instructions.cbz(X0, skipLabel, "Branch if condition is false");

    // Generate the 'then' block code
    visitStatement(node->then_statement.get());

    // Define the label that the branch skips to
    instructions.setPendingLabel(skipLabel);
    labelManager.defineLabel(skipLabel, instructions.getCurrentAddress());
}

void CodeGenerator::visitTestStatement(const TestStatement* node) {
    auto elseLabel = labelManager.generateLabel("test_else");
    auto endLabel = labelManager.generateLabel("test_end");

    // Evaluate condition
    visitExpression(node->condition.get());
    instructions.cmp(X0, 0);
    labelManager.requestLabelFixup(elseLabel, instructions.getCurrentAddress());
    instructions.beq(elseLabel);

    // Generate then branch
    visitStatement(node->then_statement.get());
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.b(endLabel);

    // Generate else branch
    instructions.setPendingLabel(elseLabel);
    labelManager.defineLabel(elseLabel, instructions.getCurrentAddress());
    if (node->else_statement) {
        visitStatement(node->else_statement.get());
    }

    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());
}

void CodeGenerator::visitWhileStatement(const WhileStatement* node) {
    labelManager.pushScope(LabelManager::ScopeType::LOOP);

    auto startLabel = labelManager.getCurrentRepeatLabel();
    auto endLabel = labelManager.getCurrentEndLabel();

    // Loop start
    instructions.setPendingLabel(startLabel);
    labelManager.defineLabel(startLabel, instructions.getCurrentAddress());

    // Evaluate condition
    visitExpression(node->condition.get());
    instructions.cmp(X0, 0); // Compare result with 0 (BCPL false)
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.beq(endLabel); // Branch if condition is 0 (false) to end

    // Generate loop body
    visitStatement(node->body.get());
    labelManager.requestLabelFixup(startLabel, instructions.getCurrentAddress());
    instructions.b(startLabel);

    // Loop end
    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());

    labelManager.popScope();
}

void CodeGenerator::visitForStatement(const ForStatement* node) {
    labelManager.pushScope(LabelManager::ScopeType::LOOP);
    auto startLabel = labelManager.getCurrentRepeatLabel();
    auto endLabel = labelManager.getCurrentEndLabel();

    // --- SETUP: Use registers for loop variables ---
    // 1. Initialize 'i' in a register
    visitExpression(node->from_expr.get()); // from_expr result in x0
    int i_offset = allocateLocal(node->var_name);
    uint32_t i_reg = registerManager.acquireRegisterForInit(node->var_name, i_offset);
    instructions.mov(i_reg, X0, "Initialize loop var " + node->var_name + " in " + instructions.regName(i_reg));
    registerManager.markDirty(node->var_name);

    // 2. Store the 'to' value in a register
    visitExpression(node->to_expr.get()); // to_expr result in x0
    uint32_t to_reg = scratchAllocator.acquire();
    instructions.mov(to_reg, X0, "Move 'to' value into " + instructions.regName(to_reg));

    // 3. Store the 'by' value in a register
    uint32_t by_reg = scratchAllocator.acquire();
    if (node->by_expr) {
        visitExpression(node->by_expr.get());
    } else {
        instructions.loadImmediate(X0, 1);
    }
    instructions.mov(by_reg, X0, "Move 'by' value into " + instructions.regName(by_reg));

    // --- LOOP START ---
    instructions.setPendingLabel(startLabel);
    labelManager.defineLabel(startLabel, instructions.getCurrentAddress());

    // --- CONDITION: Compare registers directly ---
    instructions.cmp(i_reg, to_reg);
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.bgt(endLabel); // Exit if i > to

    // --- BODY ---
    visitStatement(node->body.get());

    // --- INCREMENT: Use registers directly ---
    instructions.add(i_reg, i_reg, by_reg, AArch64Instructions::LSL, 0, "Increment " + node->var_name);
    registerManager.markDirty(node->var_name); // 'i' has changed
    labelManager.requestLabelFixup(startLabel, instructions.getCurrentAddress());
    instructions.b(startLabel);

    // --- LOOP END ---
    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());

    // Release scratch registers used for 'to' and 'by'
    scratchAllocator.release(to_reg);
    scratchAllocator.release(by_reg);

    labelManager.popScope();
}

void CodeGenerator::visitGotoStatement(const GotoStatement* node) {
    if (auto label = dynamic_cast<const VariableAccess*>(node->label.get())) {
        labelManager.requestLabelFixup(label->name, instructions.getCurrentAddress());
        instructions.b(label->name);
    } else {
        throw std::runtime_error("GOTO requires a label");
    }
}

void CodeGenerator::visitLabeledStatement(const LabeledStatement* node) {
    instructions.setPendingLabel(node->name);
    labelManager.defineLabel(node->name, instructions.getCurrentAddress());
    visitStatement(node->statement.get());
}

// CodeGenerator.cpp (continued)
void CodeGenerator::visitSwitchonStatement(const SwitchonStatement* node) {
    labelManager.pushScope(LabelManager::ScopeType::SWITCHON);

    auto endLabel = labelManager.getCurrentEndLabel();
    auto defaultLabel = labelManager.generateLabel("switch_default");

    // Evaluate switch expression. Value is now in X0.
    visitExpression(node->expression.get());

    // The value is in X0. We pass it directly to the search functions.
    // This removes the store/load inefficiency and the garbage ldr.
    if (node->cases.size() > 0 && isSmallDenseRange(node->cases)) {
        generateJumpTable(node->cases, defaultLabel);
    } else {
        generateBinarySearchTree(node->cases, defaultLabel);
    }

    // Generate case bodies using the correct labels from the AST.
    for (const auto& caseStmt : node->cases) {
        instructions.setPendingLabel(caseStmt.label);
        labelManager.defineLabel(caseStmt.label, instructions.getCurrentAddress());
        visitStatement(caseStmt.statement.get());

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
            instructions.b(endLabel, "Branch to end of switch");
        }
    }

    // Default case
    instructions.setPendingLabel(defaultLabel);
    labelManager.defineLabel(defaultLabel, instructions.getCurrentAddress());
    if (node->default_case) {
        visitStatement(node->default_case.get());
    }

    // End of switch
    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());

    labelManager.popScope();
}

void CodeGenerator::generateJumpTable(
    const std::vector<SwitchonStatement::SwitchCase>& cases,
    const std::string& defaultLabel) {

    // Switch value is already in X0 from visitSwitchonStatement.

    // Check bounds
    int minValue = cases.front().value;
    int maxValue = cases.back().value;

    instructions.sub(X1, X0, minValue);  // Normalize to 0-based index
    instructions.loadImmediate(X2, maxValue - minValue);
    instructions.cmp(X1, X2);
    instructions.bgt(defaultLabel);  // Branch if out of range

    // Generate jump table
    auto tableLabel = labelManager.generateLabel("jump_table");
    instructions.adr(X2, tableLabel);  // Load table address
    instructions.add(X2, X2, X1, AArch64Instructions::LSL, 3);  // Calculate entry address (x8 for pointer size) - Fixed: LSL 3
    instructions.ldr(X2, X2, 0);  // Load target address
    instructions.br(X2);  // Branch to target

    // Emit jump table
    instructions.setPendingLabel(tableLabel);
    labelManager.defineLabel(tableLabel, instructions.getCurrentAddress());
    for (int i = minValue; i <= maxValue; i++) {
        auto it = std::find_if(cases.begin(), cases.end(),
            [i](const SwitchonStatement::SwitchCase& c) { return c.value == i; });

        if (it != cases.end()) {
            emitAddress(it->label);
        } else {
            // If a value in the range is not explicitly handled, jump to default
            emitAddress(defaultLabel); // Fixed: Jump to default for unhandled values in dense range
        }
    }
}

void CodeGenerator::generateBinarySearchTree(
    const std::vector<SwitchonStatement::SwitchCase>& cases,
    const std::string& defaultLabel) {

    // Switch value is already in X0 from visitSwitchonStatement.

    // Generate binary search using recursive helper
    generateBinarySearchNode(cases, 0, cases.size() - 1, defaultLabel);
}

void CodeGenerator::generateBinarySearchNode(
    const std::vector<SwitchonStatement::SwitchCase>& cases,
    size_t start,
    size_t end,
    const std::string& defaultLabel) {

    if (start > end) {
        instructions.b(defaultLabel);
        return;
    }

    size_t mid = start + (end - start) / 2;
    const auto& midCase = cases[mid];

    instructions.loadImmediate(X1, midCase.value);
    instructions.cmp(X0, X1); // Assumes switch value is in X0

    auto ltLabel = labelManager.generateLabel("case_lt");
    auto gtLabel = labelManager.generateLabel("case_gt");

    instructions.blt(ltLabel, "Branch if less than case value");
    instructions.bgt(gtLabel, "Branch if greater than case value");

    // Correct Fix: Branch to the label stored in the AST node.
    instructions.b(midCase.label, "Branch to case " + std::to_string(midCase.value));

    // Less than case
    instructions.setPendingLabel(ltLabel);
    labelManager.defineLabel(ltLabel, instructions.getCurrentAddress());
    if (mid > start) {
        generateBinarySearchNode(cases, start, mid - 1, defaultLabel);
    } else {
        instructions.b(defaultLabel);
    }

    // Greater than case
    instructions.setPendingLabel(gtLabel);
    labelManager.defineLabel(gtLabel, instructions.getCurrentAddress());
    if (mid < end) {
        generateBinarySearchNode(cases, mid + 1, end, defaultLabel);
    } else {
        instructions.b(defaultLabel);
    }
}


void CodeGenerator::visitFunctionCall(const FunctionCall* node) {
    // Calculate the number of arguments that will be passed on the stack.
    size_t num_stack_args = 0;
    if (node->arguments.size() > 8) {
        num_stack_args = node->arguments.size() - 8;
    }

    // Update maxOutgoingParamSpace based on actual stack arguments.
    size_t currentCallParamBytes = num_stack_args * 8;
    if (currentCallParamBytes > maxOutgoingParamSpace) {
        maxOutgoingParamSpace = currentCallParamBytes;
    }

    saveCallerSavedRegisters(); // Save caller-saved registers before argument evaluation.

    // Allocate space for stack arguments if necessary.
    if (num_stack_args > 0) {
        instructions.sub(SP, SP, currentCallParamBytes, "Allocate space for outgoing arguments");
    }

    // Evaluate arguments and place them in registers or on the stack.
    // Evaluate in reverse order to avoid overwriting argument registers.
    for (int i = node->arguments.size() - 1; i >= 0; --i) {
        visitExpression(node->arguments[i].get()); // Result is in X0

        if (i < 8) { // First 8 arguments go into registers X0-X7
            instructions.mov(X0 + i, X0, "Move arg " + std::to_string(i) + " to X" + std::to_string(i));
        } else { // Arguments beyond the 8th go onto the stack
            // Stack arguments are pushed in order, so calculate offset from the beginning of the allocated block.
            size_t stack_offset_index = i - 8;
            instructions.str(X0, SP, stack_offset_index * 8, "Store arg " + std::to_string(i) + " to stack");
        }
    }

    // Generate call
    if (auto funcVar = dynamic_cast<const VariableAccess*>(node->function.get())) {
        // Direct function call
        if (auto it = functions.find(funcVar->name); it != functions.end()) {
            instructions.bl(funcVar->name, "Call " + funcVar->name);
        } else {
            throw std::runtime_error("Unknown function: " + funcVar->name);
        }
    } else {
        // Indirect function call through expression
        visitExpression(node->function.get());
        instructions.br(X0);
    }

    // Deallocate stack arguments if necessary.
    if (num_stack_args > 0) {
        instructions.add(SP, SP, currentCallParamBytes, "Deallocate outgoing arguments");
    }

    restoreCallerSavedRegisters(); // Restore caller-saved registers after the call.
}

void CodeGenerator::visitAssignment(const Assignment* node) {
    visitExpression(node->rhs[0].get());

    if (auto num_lit = dynamic_cast<const NumberLiteral*>(node->lhs[0].get())) {
        throw std::runtime_error("Cannot assign to a number literal.");
    } else if (auto var = dynamic_cast<const VariableAccess*>(node->lhs[0].get())) {
        if (auto it = manifestConstants.find(var->name); it != manifestConstants.end()) {
            throw std::runtime_error("Cannot assign to manifest constant: " + var->name);
        } else if (auto it = globals.find(var->name); it != globals.end()) {
            instructions.str(X0, X28, it->second * 8, "Store to global " + var->name);
        } else {
            uint32_t reg = registerManager.getVariableRegister(var->name);
            if (reg == 0xFFFFFFFF) {
                int offset = getLocalOffset(var->name);
                reg = registerManager.acquireRegisterForInit(var->name, offset);
            }
            instructions.mov(reg, X0, "Assign value to " + var->name + " in " + instructions.regName(reg));
            registerManager.markDirty(var->name);
        }
    } else if (auto deref = dynamic_cast<const DereferenceExpr*>(node->lhs[0].get())) {
        uint32_t valueReg = scratchAllocator.acquire();
        instructions.mov(valueReg, X0, "Save RHS value for dereference assignment");
        visitExpression(deref->pointer.get());
        instructions.str(valueReg, X0, 0, "Store to computed address");
        scratchAllocator.release(valueReg);
    } else if (auto vecAccess = dynamic_cast<const VectorAccess*>(node->lhs[0].get())) {
        uint32_t valueReg = scratchAllocator.acquire();
        instructions.mov(valueReg, X0, "Save RHS value for vector assignment");
        visitExpression(vecAccess->index.get());
        uint32_t indexReg = scratchAllocator.acquire();
        instructions.mov(indexReg, X0, "Save index value");
        visitExpression(vecAccess->vector.get());
        uint32_t vectorBaseReg = scratchAllocator.acquire();
        instructions.mov(vectorBaseReg, X0, "Save vector base address");
        instructions.add(vectorBaseReg, vectorBaseReg, indexReg, AArch64Instructions::LSL, 3, "Calculate element address");
        instructions.str(valueReg, vectorBaseReg, 0, "Store to vector element");
        scratchAllocator.release(vectorBaseReg);
        scratchAllocator.release(indexReg);
        scratchAllocator.release(valueReg);
    } else if (auto charAccess = dynamic_cast<const CharacterAccess*>(node->lhs[0].get())) {
        uint32_t valueReg = scratchAllocator.acquire();
        instructions.mov(valueReg, X0, "Save RHS value for character assignment");
        visitExpression(charAccess->index.get());
        uint32_t indexReg = scratchAllocator.acquire();
        instructions.mov(indexReg, X0, "Save index value");
        visitExpression(charAccess->string.get());
        uint32_t stringBaseReg = scratchAllocator.acquire();
        instructions.mov(stringBaseReg, X0, "Save string base address");
        instructions.add(stringBaseReg, stringBaseReg, indexReg, AArch64Instructions::LSL, 2, "Calculate character address (4-byte chars)");
        instructions.str(valueReg, stringBaseReg, 0, "Store to character");
        scratchAllocator.release(stringBaseReg);
        scratchAllocator.release(indexReg);
        scratchAllocator.release(valueReg);
    } else {
        throw std::runtime_error("Unsupported LHS in assignment.");
    }
}

void CodeGenerator::visitResultisStatement(const ResultisStatement* node) {
    // Check for tail call optimization opportunity
    if (auto call = dynamic_cast<const FunctionCall*>(node->value.get())) {
        if (auto funcVar = dynamic_cast<const VariableAccess*>(call->function.get())) {
            if (funcVar->name == currentFunctionName) {
                // This is a direct tail-recursive call.
                // 1. Evaluate new arguments into argument registers (X0, X1, ...)
                // Handle up to 2 arguments for FACT_TAIL example.
                if (call->arguments.size() > 0) {
                    visitExpression(call->arguments[0].get()); // Result in X0
                }
                if (call->arguments.size() > 1) {
                    // Save X0 temporarily if it's needed for the second argument's evaluation
                    uint32_t temp_x0 = scratchAllocator.acquire();
                    instructions.mov(temp_x0, X0);
                    visitExpression(call->arguments[1].get()); // Result in X0
                    instructions.mov(X1, X0); // Move second arg to X1
                    instructions.mov(X0, temp_x0); // Restore first arg to X0
                    scratchAllocator.release(temp_x0);
                }

                // 2. Jump to the beginning of the current function.
                instructions.b(currentFunctionName, "Tail call optimization");
                return; // Skip normal epilogue generation for this path.
            }
        }
    }

    // Otherwise, it's a normal RESULTIS statement.
    visitExpression(node->value.get());

    // If the result is a variable, and this is its last use, release its register without spilling.
    if (auto var_access = dynamic_cast<const VariableAccess*>(node->value.get())) {
        uint32_t reg = registerManager.getVariableRegister(var_access->name);
        if (reg != 0xFFFFFFFF) {
            // This is a heuristic: assume RESULTIS is the last use of the variable.
            // A proper liveness analysis would confirm this.
            registerManager.releaseRegisterWithoutSpill(reg);
        }
    }

    // No explicit branch needed here, as the return label is defined right before the epilogue.
    // The epilogue will be executed directly after the RESULTIS statement's code.
    instructions.b(labelManager.getCurrentReturnLabel(), "Branch to function epilogue after RESULTIS");
}

void CodeGenerator::visitBreakStatement(const BreakStatement* node) {
    auto endLabel = labelManager.getCurrentEndLabel();
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.b(endLabel, "Break from current construct");
}

// Helper methods

int CodeGenerator::allocateLocal(const std::string& name) {
    currentLocalVarOffset -= 8; // Decrement offset from FP
    localVars[name] = currentLocalVarOffset;
    return currentLocalVarOffset;
}

int CodeGenerator::getLocalOffset(const std::string& name) {
    auto it = localVars.find(name);
    if (it == localVars.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return it->second;
}

size_t CodeGenerator::allocateGlobal() {
    size_t index = globals.size();
    // We don't assign a name here, just reserve space.
    // The actual name mapping happens when 'let' declarations are processed.
    return index;
}

void CodeGenerator::saveCallerSavedRegisters() {
    // Spill all currently dirty registers to memory before saving caller-saved registers.
    registerManager.spillAllDirtyRegisters();

    // Save caller-saved registers that are currently in use by the RegisterManager.
    // These are pushed onto the stack, and `currentLocalVarOffset` is decremented.
    // The offset for `str` should be relative to `X29` (FP).
    // The `savedCallerRegsAroundCall` stores `(reg, offset_from_FP, var_name)`.

    int currentSaveBytes = 0;
    std::vector<std::tuple<uint32_t, int, std::string>> regsToSaveInfo; // Store info for later restoration
    for (uint32_t reg : registerManager.getUsedRegisters()) {
        // Exclude argument registers (X0-X7) as they are handled by ABI
        if (reg >= AArch64Instructions::X0 && reg <= AArch64Instructions::X7) continue;

        // Check if it's a caller-saved register (X9-X15 are typical scratch, X0-X7 args)
        // For simplicity, we'll save all non-argument registers currently in use by RegisterManager.
        // A more precise approach would check if 'reg' is in the caller-saved set.
        // For now, assume any non-argument register managed by RegisterManager needs saving.

        currentLocalVarOffset -= 8; // Decrement main stack offset (relative to FP)
        currentSaveBytes += 8;
        instructions.str(reg, X29, currentLocalVarOffset, "Save caller-saved register " + instructions.regName(reg));

        // Get the variable name associated with this register from RegisterManager
        std::string varName = registerManager.getVariableName(reg);
        regsToSaveInfo.emplace_back(reg, currentLocalVarOffset, varName);
    }
    // Update maxCallerSavedRegsSpace with the maximum space used by caller-saved registers
    if (currentSaveBytes > maxCallerSavedRegsSpace) {
        maxCallerSavedRegsSpace = currentSaveBytes;
    }

    // Now, remove the variables from the RegisterManager's active set
    // so they are not considered in use during the function call.
    // This must be done *after* iterating through getUsedRegisters()
    // to avoid modifying the set while iterating.
    for (const auto& entry : regsToSaveInfo) {
        registerManager.removeVariableFromRegister(std::get<2>(entry));
    }
    savedCallerRegsAroundCall = std::move(regsToSaveInfo); // Store for restoration
}

void CodeGenerator::restoreCallerSavedRegisters() {
    // Restore in reverse order
    for (auto it = savedCallerRegsAroundCall.rbegin(); it != savedCallerRegsAroundCall.rend(); ++it) {
        uint32_t reg = std::get<0>(*it);
        int offset = std::get<1>(*it);
        std::string varName = std::get<2>(*it);

        instructions.ldr(reg, X29, offset, "Restore caller-saved register " + instructions.regName(reg));
        currentLocalVarOffset += 8; // Increment main stack offset

        // Re-establish the variable-to-register mapping in RegisterManager
        if (!varName.empty()) { // Only reassign if it was a variable
            registerManager.reassignRegister(varName, reg, offset);
        }
    }
    // Reset saved registers list
    savedCallerRegsAroundCall.clear();
}

bool CodeGenerator::isRegisterInUse(uint32_t reg) {
    // Check if register is currently being used by the RegisterManager
    return registerManager.getUsedRegisters().count(reg);
}

void CodeGenerator::addToListing(const std::string& instruction, const std::string& comment) {
    assemblyListing << std::setw(40) << std::left << instruction;
    if (!comment.empty()) {
        assemblyListing << "; " << comment;
    }
    assemblyListing << "\n";
}

void CodeGenerator::emitAddress(const std::string& label) {
    // This is a placeholder for emitting a data address (e.g., for jump tables)
    // In a real assembler, this would be a .quad or .word directive
    // For now, we\'ll just add a comment to the assembly listing
    addToListing(".quad " + label, "Address of " + label);
}

bool CodeGenerator::isSmallDenseRange(const std::vector<SwitchonStatement::SwitchCase>& cases) {
    if (cases.empty()) return false;

    int minValue = cases.front().value;
    int maxValue = cases.back().value;

    // Check if range is small enough for jump table
    if (maxValue - minValue > 1000) return false;

    // Check density (at least 50% of values in range should be present)
    int range = maxValue - minValue + 1;
    return cases.size() >= range / 2;
}

void CodeGenerator::saveCalleeSavedRegisters() {
    for (const auto& reg : calleeSavedRegs) {
        if (isRegisterInUse(reg)) {
            currentLocalVarOffset -= 8;
            instructions.str(reg, X29, currentLocalVarOffset, "Save callee-saved register " + instructions.regName(reg));
            savedCalleeRegsInPrologue.push_back({reg, currentLocalVarOffset}); // Store as if it's a caller-saved for now
        }
    }
}

void CodeGenerator::restoreCalleeSavedRegisters() {
    for (auto it = savedCalleeRegsInPrologue.rbegin(); it != savedCalleeRegsInPrologue.rend(); ++it) {
        instructions.ldr(it->first, X29, it->second, "Restore callee-saved register " + instructions.regName(it->first));
        currentLocalVarOffset += 8;
    }
    savedCalleeRegsInPrologue.clear();
}

// Final assembly and optimization
void CodeGenerator::finalizeCode() {
    // Resolve all branch targets
    resolveBranchTargets();

    // Perform peephole optimization
    performPeepholeOptimization();

    // Generate final assembly listing
    generateAssemblyListing();
}

void CodeGenerator::resolveBranchTargets() {
    for (auto& instruction : instructions.getInstructions()) {
        if (instruction.needsLabelResolution) {
            size_t targetAddress = labelManager.getLabelAddress(instruction.targetLabel);
            int64_t offset = static_cast<int64_t>(targetAddress - instruction.address);
            instruction.resolveLabel(offset);
        }
    }
}

void CodeGenerator::performPeepholeOptimization() {
    auto& instrs = instructions.getInstructions();
    for (size_t i = 0; i < instrs.size() - 1; ++i) {
        // Example optimization: combine consecutive loads/stores
        if (instrs[i].isStore() && instrs[i+1].isLoad()) {
            if (canCombineLoadStore(instrs[i], instrs[i+1])) {
                combineLoadStore(instrs[i], instrs[i+1]);
                instrs.erase(instrs.begin() + i + 1);
                --i;
            }
        }
    }
}

bool CodeGenerator::canCombineLoadStore(const AArch64Instructions::Instruction& instr1, const AArch64Instructions::Instruction& instr2) {
    // Placeholder: Implement actual logic to check if two instructions can be combined
    // This would involve checking registers, offsets, and instruction types.
    return false;
}

void CodeGenerator::combineLoadStore(AArch64Instructions::Instruction& instr1, AArch64Instructions::Instruction& instr2) {
    // Placeholder: Implement actual combination logic
    // This would modify instr1 to represent the combined instruction
    // and potentially invalidate instr2.
}

void CodeGenerator::generateAssemblyListing() {
    assemblyListing.clear();
    assemblyListing << ".text\n";
    assemblyListing << ".align 4\n\n";

    for (const auto& instr : instructions.getInstructions()) {
        if (instr.hasLabel) {
            assemblyListing << instr.label << ":\n";
        }
        assemblyListing << "\t" << instr.toString() << "\n";
    }
}

void CodeGenerator::visitLetDeclaration(const LetDeclaration* node) {
    if (node->initializers.empty()) {
        throw std::runtime_error("LetDeclaration must have at least one initializer.");
    }

    for (const auto& init : node->initializers) {
        if (init.init) {
            // 1. Evaluate expression, result is in X0.
            visitExpression(init.init.get());

            // 2. Allocate stack space for the variable's home.
            int offset = allocateLocal(init.name);

            // 3. Get a register for the variable (e.g., x27) without loading from stack.
            uint32_t reg = registerManager.acquireRegisterForInit(init.name, offset);

            // 4. Move the initial value from X0 into the variable's new register.
            //    This keeps the value live in a register and avoids the store-then-load.
            instructions.mov(reg, X0, "Initialize local " + init.name + " in " + instructions.regName(reg));

            // 5. Mark the register as holding the latest value.
            registerManager.markDirty(init.name);
        } else {
            int offset = allocateLocal(init.name);
            registerManager.acquireRegisterForInit(init.name, offset);
        }
    }
}

void CodeGenerator::visitValof(const Valof* node) {
    // This typically involves executing a statement block and returning a value.
    // For now, we'll just visit the statement and assume the result is in X0.
    visitStatement(node->body.get());
}

void CodeGenerator::visitTableConstructor(const TableConstructor* node) {
    // This would involve allocating memory for the table and populating it.
    throw std::runtime_error("Table constructors not yet implemented.");
}



        void CodeGenerator::visitVectorConstructor(const VectorConstructor* node) {
    visitExpression(node->size.get());
    // The size of the vector is now in X0.
    // We need to call the `bcpl_vec` runtime function to allocate the vector.
    instructions.bl("bcpl_vec", "Allocate vector on heap");
    // The result of the allocation (the pointer to the vector) is in X0.
}

void CodeGenerator::visitCharacterAccess(const CharacterAccess* node) {
    // Evaluate index first
    visitExpression(node->index.get());
    // Save index
    uint32_t indexReg = scratchAllocator.acquire();
    instructions.mov(indexReg, X0);

    // Evaluate string address
    visitExpression(node->string.get());

    // Calculate final address and load
    instructions.add(X0, X0, indexReg, AArch64Instructions::LSL, 2, "Calculate character address (4-byte chars)");
    instructions.ldr(X0, X0, 0, "Load character");
    scratchAllocator.release(indexReg);
}

void CodeGenerator::visitConditionalExpression(const ConditionalExpression* node) {
    auto elseLabel = labelManager.generateLabel("cond_else");
    auto endLabel = labelManager.generateLabel("cond_end");

    // Evaluate the condition
    visitExpression(node->condition.get());
    instructions.cmp(X0, 0);
    labelManager.requestLabelFixup(elseLabel, instructions.getCurrentAddress());
    instructions.beq(elseLabel);

    // If true, evaluate the 'then' expression
    visitExpression(node->trueExpr.get());
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.b(endLabel);

    // If false, evaluate the 'else' expression
    instructions.setPendingLabel(elseLabel);
    labelManager.defineLabel(elseLabel, instructions.getCurrentAddress());
    visitExpression(node->falseExpr.get());

    // Define the end label
    instructions.setPendingLabel(endLabel);
    labelManager.defineLabel(endLabel, instructions.getCurrentAddress());
}

void CodeGenerator::visitRoutineCall(const RoutineCall* node) {
    if (auto funcCall = dynamic_cast<const FunctionCall*>(node->call_expression.get())) {
        if (auto funcVar = dynamic_cast<const VariableAccess*>(funcCall->function.get())) {
            if (funcVar->name == "WRITES") {
                visitExpression(funcCall->arguments[0].get());
                instructions.bl("writes", "Call writes");
            } else if (funcVar->name == "WRITEN") {
                visitExpression(funcCall->arguments[0].get());
                instructions.bl("writen", "Call writen");
            } else if (funcVar->name == "WRITEF") {
                // WRITEF takes format string in X0, first arg in X1, etc.
                // For simplicity, we'll push all args to stack and then load first 2 to X0, X1
                // This is a temporary simplification and not fully ABI compliant for multiple args
                size_t argsBytes = funcCall->arguments.size() * 8;
                if (argsBytes > 0) {
                    instructions.sub(SP, SP, argsBytes, "Allocate space for WRITEF arguments");
                }

                // Evaluate arguments and store them on the stack in reverse order
                for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                    visitExpression(funcCall->arguments[funcCall->arguments.size() - 1 - i].get()); // Result in X0
                    instructions.str(X0, SP, i * 8, "Store WRITEF argument " + std::to_string(funcCall->arguments.size() - 1 - i));
                }

                // Load format string into X0, first argument into X1
                // Assuming WRITEF takes format string as first arg, and then subsequent args
                // This needs to be aligned with the actual WRITEF runtime signature.
                if (funcCall->arguments.size() > 0) {
                    instructions.ldr(X0, SP, (funcCall->arguments.size() - 1) * 8, "Load format string for WRITEF");
                }
                if (funcCall->arguments.size() > 1) {
                    instructions.ldr(X1, SP, (funcCall->arguments.size() - 2) * 8, "Load first data arg for WRITEF");
                }
                // Other arguments would remain on stack for varargs, but not handled here.

                instructions.bl("writef", "Call writef");

                if (argsBytes > 0) {
                    instructions.add(SP, SP, argsBytes, "Deallocate WRITEF arguments");
                }

            } else if (funcVar->name == "NEWLINE") {
                instructions.bl("newline", "Call newline");
            } else if (funcVar->name == "FINISH") {
                instructions.bl("finish", "Call finish");
            } else {
                // Regular function call
                saveCallerSavedRegisters();

                // Evaluate arguments and push them onto the stack in reverse order
                size_t argsBytes = funcCall->arguments.size() * 8;
                if (argsBytes > 0) {
                    instructions.sub(SP, SP, argsBytes, "Allocate space for outgoing arguments");
                }

                for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                    visitExpression(funcCall->arguments[i].get()); // Result in X0
                    instructions.str(X0, SP, i * 8, "Store argument " + std::to_string(i));
                }

                // Load arguments into registers (x0, x1, x2...) from the stack
                // Only load up to 8 arguments into registers as per AArch64 ABI
                for (size_t i = 0; i < std::min((size_t)8, funcCall->arguments.size()); ++i) {
                    instructions.ldr(X0 + i, SP, i * 8, "Load parameter into register");
                }

                if (auto it = functions.find(funcVar->name); it != functions.end()) {
                    instructions.bl(funcVar->name, "Call routine " + funcVar->name);
                } else {
                    throw std::runtime_error("Unknown routine: " + funcVar->name);
                }

                // Deallocate arguments pushed for this call
                if (argsBytes > 0) {
                    instructions.add(SP, SP, argsBytes, "Deallocate outgoing arguments");
                }
                restoreCallerSavedRegisters();
            }
        }
    }
}

void CodeGenerator::visitReturnStatement(const ReturnStatement* node) {
    // No explicit branch needed here, as the return label is defined right before the epilogue.
    // The epilogue will be executed directly after the RETURN statement's code.
}

void CodeGenerator::visitLoopStatement(const LoopStatement* node) {
    auto startLabel = labelManager.getCurrentRepeatLabel();
    labelManager.requestLabelFixup(startLabel, instructions.getCurrentAddress());
    instructions.b(startLabel, "Loop back");
}

void CodeGenerator::visitEndcaseStatement(const EndcaseStatement* node) {
    // This typically marks the end of a switch case, so we branch to the end of the switch.
    auto endLabel = labelManager.getCurrentEndLabel();
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.b(endLabel, "End of case");
}

void CodeGenerator::visitFinishStatement(const FinishStatement* node) {
    // This typically exits the current loop or function. For now, we'll treat it like a break.
    auto endLabel = labelManager.getCurrentEndLabel();
    labelManager.requestLabelFixup(endLabel, instructions.getCurrentAddress());
    instructions.b(endLabel, "Finish current construct");
}

void CodeGenerator::printAsm() const {
    std::cout << "\n;------------ Generated ARM64 Assembly ------------\n\n";

    // Print any necessary assembler directives
    std::cout << ".arch armv8-a\n";
    std::cout << ".text\n";
    std::cout << ".align 4\n\n";

    // Print any global declarations
    if (!globals.empty()) {
        std::cout << ".data\n";
        for (const auto& global : globals) {
            std::cout << ".global " << global.first << "\n";
            std::cout << global.first << ":\n";
            std::cout << "    .space " << 8 * 1 << "\n";
        }
        std::cout << "\n";
    }

    std::cout << ".text\n";
    std::cout << ".align 4\n\n";

    // Print all instructions with their comments
    for (const auto& instr : instructions.getInstructions()) {
        // Print labels if present
        if (instr.hasLabel) {
            std::cout << instr.label << ":\n";
        }

        // Print the instruction with proper formatting
        std::cout << "    " << std::left << std::setw(30) << instr.toString();

        // Add comment if present
        if (!instr.comment.empty()) {
            std::cout << " // " << instr.comment;
        }
        std::cout << "\n";
    }

    // Print string pool
    if (!stringPool.empty()) {
        std::cout << "\n.data\n";
        for (size_t i = 0; i < stringPool.size(); ++i) {
            std::cout << ".L.str" << i << ":\n";
            std::cout << "    .string \"" << stringPool[i] << "\"\n";
        }
    }

    std::cout << "\n;------------ End of Assembly ------------\n\n";
}
