#include "CFGBuilder.h"
#include <iostream>

BasicBlock::Ptr CFGBuilder::createNewBlock() {
    BasicBlock::Ptr newBlock = std::make_shared<BasicBlock>(nextBlockId++);
    std::cout << "CFGBuilder: Created new block " << newBlock->toString() << "\n";
    return newBlock;
}

void CFGBuilder::build(const ProgramPtr& program) {
    functionEntryBlocks.clear();
    nextBlockId = 0;

    std::cout << "CFGBuilder: Starting CFG construction...\n";

    for (auto& decl : program->declarations) { 
        if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            std::cout << "CFGBuilder: Processing function: " << funcDecl->name << "\n";
            // Create an entry block for each function
            BasicBlock::Ptr entryBlock = createNewBlock();
            functionEntryBlocks[funcDecl->name] = entryBlock;

            // Build CFG for the function body
            if (funcDecl->body_stmt) {
                std::cout << "CFGBuilder: Building CFG for function body...\n";
                // Pass raw pointer to buildCFGForStatement
                buildCFGForStatement(funcDecl->body_stmt.get(), entryBlock);
            } 
        }
    }
    std::cout << "CFGBuilder: CFG construction complete.\n";
}

BasicBlock::Ptr CFGBuilder::addStatementToBlock(Statement* stmt, BasicBlock::Ptr currentBlock) {
    if (!currentBlock) {
        currentBlock = createNewBlock();
    }
    // std::cout << "CFGBuilder: Adding statement to " << currentBlock->toString() << ": " << stmt->toString() << "\n"; // Removed: Statement does not have toString()
    currentBlock->addStatement(stmt);
    return currentBlock;
}

BasicBlock::Ptr CFGBuilder::buildCFGForStatement(Statement* stmt, BasicBlock::Ptr currentBlock) {
    if (!stmt) return currentBlock;

    // std::cout << "CFGBuilder: Handling statement type: " << stmt->toString() << "\n"; // Removed: Statement does not have toString()

    // Handle different statement types
    if (CompoundStatement* compound = dynamic_cast<CompoundStatement*>(stmt)) {
        return handleCompoundStatement(compound, currentBlock);
    } else if (IfStatement* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        return handleIfStatement(ifStmt, currentBlock);
    } else if (WhileStatement* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        return handleWhileStatement(whileStmt, currentBlock);
    } else if (ForStatement* forStmt = dynamic_cast<ForStatement*>(stmt)) {
        return handleForStatement(forStmt, currentBlock);
    } else if (RoutineCall* routineCall = dynamic_cast<RoutineCall*>(stmt)) {
        return handleRoutineCall(routineCall, currentBlock);
    } else if (ReturnStatement* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        return handleReturnStatement(ret, currentBlock);
    } else if (LoopStatement* loop = dynamic_cast<LoopStatement*>(stmt)) {
        return handleLoopStatement(loop, currentBlock);
    } else if (RepeatStatement* repeat = dynamic_cast<RepeatStatement*>(stmt)) {
        return handleRepeatStatement(repeat, currentBlock);
    } else if (SwitchonStatement* switchon = dynamic_cast<SwitchonStatement*>(stmt)) {
        return handleSwitchonStatement(switchon, currentBlock);
    } else if (GotoStatement* gotoStmt = dynamic_cast<GotoStatement*>(stmt)) {
        return handleGotoStatement(gotoStmt, currentBlock);
    } else if (LabeledStatement* labeledStmt = dynamic_cast<LabeledStatement*>(stmt)) {
        return handleLabeledStatement(labeledStmt, currentBlock);
    } else if (DeclarationStatement* declStmt = dynamic_cast<DeclarationStatement*>(stmt)) {
        return handleDeclarationStatement(declStmt, currentBlock);
    } else if (Assignment* assign = dynamic_cast<Assignment*>(stmt)) {
        return handleAssignment(assign, currentBlock);
    } else if (TestStatement* testStmt = dynamic_cast<TestStatement*>(stmt)) {
        return handleTestStatement(testStmt, currentBlock);
    } else if (ResultisStatement* resultis = dynamic_cast<ResultisStatement*>(stmt)) {
        return handleResultisStatement(resultis, currentBlock);
    } else if (EndcaseStatement* endcase = dynamic_cast<EndcaseStatement*>(stmt)) {
        return handleEndcaseStatement(endcase, currentBlock);
    } else if (FinishStatement* finish = dynamic_cast<FinishStatement*>(stmt)) {
        return handleFinishStatement(finish, currentBlock);
    } else {
        // Default: add statement to current block and continue
        return addStatementToBlock(stmt, currentBlock);
    }
}

// --- Specific Statement Handlers ---

BasicBlock::Ptr CFGBuilder::handleCompoundStatement(CompoundStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling CompoundStatement in " << currentBlock->toString() << "\n";

    for (auto& s_node_ptr : stmt->statements) { 
        currentBlock = buildCFGForStatement(static_cast<Statement*>(s_node_ptr.get()), currentBlock);
    }
    return currentBlock;
}

BasicBlock::Ptr CFGBuilder::handleIfStatement(IfStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling IfStatement in " << currentBlock->toString() << "\n";
    // Add the if condition to the current block
    currentBlock = addStatementToBlock(stmt, currentBlock);

    // Create blocks for then branch
    BasicBlock::Ptr thenBlock = createNewBlock();

    // Connect current block to then block
    currentBlock->addSuccessor(thenBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << thenBlock->toString() << " (then branch)\n";

    // Build CFG for then branch
    BasicBlock::Ptr thenEndBlock = buildCFGForStatement(stmt->then_statement.get(), thenBlock);

    // Create a merge block
    BasicBlock::Ptr mergeBlock = createNewBlock();

    // Connect end of then branch to merge block
    thenEndBlock->addSuccessor(mergeBlock);
    std::cout << "CFGBuilder: " << thenEndBlock->toString() << " -> " << mergeBlock->toString() << " (merge)\n";

    // If no else branch, current block also connects to merge block
    currentBlock->addSuccessor(mergeBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << mergeBlock->toString() << " (no else branch)\n";

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleWhileStatement(WhileStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling WhileStatement in " << currentBlock->toString() << "\n";
    // Create a block for the loop header (condition)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << loopHeaderBlock->toString() << " (loop header)\n";

    // Add condition to loop header
    currentBlock = addStatementToBlock(stmt, loopHeaderBlock);

    // Create block for loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(loopBodyBlock);
    std::cout << "CFGBuilder: " << loopHeaderBlock->toString() << " -> " << loopBodyBlock->toString() << " (loop body)\n";

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(stmt->body.get(), loopBodyBlock);

    // Connect end of body back to loop header
    bodyEndBlock->addSuccessor(loopHeaderBlock);
    std::cout << "CFGBuilder: " << bodyEndBlock->toString() << " -> " << loopHeaderBlock->toString() << " (loop backedge)\n";

    // Create exit block for the loop
    BasicBlock::Ptr exitBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(exitBlock); // Condition can lead to exit
    std::cout << "CFGBuilder: " << loopHeaderBlock->toString() << " -> " << exitBlock->toString() << " (loop exit)\n";

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleForStatement(ForStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling ForStatement in " << currentBlock->toString() << "\n";
    // For statement is complex: initialization, condition, increment, body

    // Initialization part (part of current block or new block)
    currentBlock = addStatementToBlock(stmt, currentBlock); // Add the for statement itself

    // Loop header (condition check)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << loopHeaderBlock->toString() << " (for loop header)\n";

    // Loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(loopBodyBlock);
    std::cout << "CFGBuilder: " << loopHeaderBlock->toString() << " -> " << loopBodyBlock->toString() << " (for loop body)\n";

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(stmt->body.get(), loopBodyBlock);

    // Increment part (after body, before next condition check)
    // For simplicity, we'll assume increment is part of the body's end block or a new block leading to header
    // A more precise CFG would have a dedicated increment block.
    bodyEndBlock->addSuccessor(loopHeaderBlock);
    std::cout << "CFGBuilder: " << bodyEndBlock->toString() << " -> " << loopHeaderBlock->toString() << " (for loop backedge)\n";

    // Exit block
    BasicBlock::Ptr exitBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(exitBlock);
    std::cout << "CFGBuilder: " << loopHeaderBlock->toString() << " -> " << exitBlock->toString() << " (for loop exit)\n";

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleRoutineCall(RoutineCall* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling RoutineCall in " << currentBlock->toString() << "\n";
    return addStatementToBlock(stmt, currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleReturnStatement(ReturnStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling ReturnStatement in " << currentBlock->toString() << "\n";
    // Return statement terminates the current control flow path
    currentBlock = addStatementToBlock(stmt, currentBlock);
    // No successors from a return statement within the function's CFG
    return nullptr; // Indicates that this path is terminated
}

BasicBlock::Ptr CFGBuilder::handleLoopStatement(LoopStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling LoopStatement in " << currentBlock->toString() << "\n";
    // Add the LOOP statement itself to the current block
    currentBlock = addStatementToBlock(stmt, currentBlock);

    // Create a block for the loop header (where control re-enters)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << loopHeaderBlock->toString() << " (loop header)\n";

    // The body of the LOOP statement is not directly part of the LoopStatement AST node.
    // It's usually the next statement in the sequence.
    // For now, we'll assume the control flow continues to the next statement after the LOOP.
    // A more sophisticated CFG builder would need to know the structure of the BCPL LOOP construct
    // and how it relates to its body (e.g., if it's followed by a compound statement).
    // For a simple LOOP, control flows back to the header.
    // This part needs careful consideration based on BCPL semantics.
    // For now, we'll just return the loopHeaderBlock, implying control flows into it.
    // The actual loop body will be processed by subsequent calls to buildCFGForStatement.

    // LOOP statements typically require explicit EXIT/FINISH to break out.
    // For now, assume no direct fall-through successor unless explicitly handled by EXIT/FINISH.
    // This means the block returned from here might not have a direct successor unless an EXIT is encountered.
    return loopHeaderBlock; // The loop itself is a continuous block, control flow exits via EXIT/FINISH
}

BasicBlock::Ptr CFGBuilder::handleRepeatStatement(RepeatStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling RepeatStatement in " << currentBlock->toString() << "\n";
    // REPEAT ... UNTIL is a post-test loop

    // Create block for loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    currentBlock->addSuccessor(loopBodyBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << loopBodyBlock->toString() << " (repeat loop body)\n";

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(stmt->body.get(), loopBodyBlock);

    // Add condition to bodyEndBlock
    bodyEndBlock = addStatementToBlock(stmt, bodyEndBlock); // The UNTIL condition is part of the last block of the body

    // Create exit block
    BasicBlock::Ptr exitBlock = createNewBlock();

    // Connect bodyEndBlock to itself (for repeat) and to exitBlock (for until condition met)
    bodyEndBlock->addSuccessor(loopBodyBlock); // Loop back
    std::cout << "CFGBuilder: " << bodyEndBlock->toString() << " -> " << loopBodyBlock->toString() << " (repeat loop backedge)\n";
    bodyEndBlock->addSuccessor(exitBlock);     // Exit condition
    std::cout << "CFGBuilder: " << bodyEndBlock->toString() << " -> " << exitBlock->toString() << " (repeat loop exit)\n";

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleSwitchonStatement(SwitchonStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling SwitchonStatement in " << currentBlock->toString() << "\n";
    currentBlock = addStatementToBlock(stmt, currentBlock);

    BasicBlock::Ptr mergeBlock = createNewBlock();

    // Access cases and default_case from the raw pointer after moving stmt
    SwitchonStatement* rawStmt = stmt;

    for (auto& scase : rawStmt->cases) {
        BasicBlock::Ptr caseBlock = createNewBlock();
        currentBlock->addSuccessor(caseBlock);
        std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << caseBlock->toString() << " (switch case)\n";
        BasicBlock::Ptr caseEndBlock = buildCFGForStatement(scase.statement.get(), caseBlock);
        caseEndBlock->addSuccessor(mergeBlock);
        std::cout << "CFGBuilder: " << caseEndBlock->toString() << " -> " << mergeBlock->toString() << " (switch merge)\n";
    }

    if (rawStmt->default_case) {
        BasicBlock::Ptr defaultBlock = createNewBlock();
        currentBlock->addSuccessor(defaultBlock);
        std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << defaultBlock->toString() << " (switch default)\n";
        BasicBlock::Ptr defaultEndBlock = buildCFGForStatement(rawStmt->default_case.get(), defaultBlock);
        defaultEndBlock->addSuccessor(mergeBlock);
        std::cout << "CFGBuilder: " << defaultEndBlock->toString() << " -> " << mergeBlock->toString() << " (switch merge)\n";
    }

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleGotoStatement(GotoStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling GotoStatement in " << currentBlock->toString() << "\n";
    currentBlock = addStatementToBlock(stmt, currentBlock);
    // GOTO requires resolution of labels after all blocks are created.
    // For now, it terminates the current path. Label resolution will connect it later.
    return nullptr;
}

BasicBlock::Ptr CFGBuilder::handleLabeledStatement(LabeledStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling LabeledStatement in " << currentBlock->toString() << "\n";
    // A labeled statement can be a target of a GOTO.
    // Create a new block for the labeled statement if the current block is not empty
    // or if it's the start of a function.
    BasicBlock::Ptr labeledBlock = createNewBlock();
    // TODO: Store mapping from label name to labeledBlock for GOTO resolution

    if (currentBlock) {
        currentBlock->addSuccessor(labeledBlock);
        std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << labeledBlock->toString() << " (labeled statement)\n";
    }

    // Continue building CFG from the labeled statement's body
    return buildCFGForStatement(stmt->statement.get(), labeledBlock);
}

BasicBlock::Ptr CFGBuilder::handleDeclarationStatement(DeclarationStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling DeclarationStatement in " << currentBlock->toString() << "\n";
    // Declarations are typically sequential and don't alter control flow directly.
    // However, initializers might contain expressions that need to be evaluated.
    // For simplicity, treat as a regular statement within the current block.
    return addStatementToBlock(stmt, currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleAssignment(Assignment* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling Assignment in " << currentBlock->toString() << "\n";
    return addStatementToBlock(stmt, currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleTestStatement(TestStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling TestStatement in " << currentBlock->toString() << "\n";
    // TEST is similar to IF, but with an OR branch
    currentBlock = addStatementToBlock(stmt, currentBlock);

    BasicBlock::Ptr thenBlock = createNewBlock();
    BasicBlock::Ptr elseBlock = nullptr;
    if (stmt->else_statement) {
        elseBlock = createNewBlock();
    }

    currentBlock->addSuccessor(thenBlock);
    std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << thenBlock->toString() << " (test then branch)\n";
    if (elseBlock) {
        currentBlock->addSuccessor(elseBlock);
        std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << elseBlock->toString() << " (test else branch)\n";
    }

    BasicBlock::Ptr thenEndBlock = buildCFGForStatement(stmt->then_statement.get(), thenBlock);
    BasicBlock::Ptr elseEndBlock = nullptr;
    if (stmt->else_statement) {
        elseEndBlock = buildCFGForStatement(stmt->else_statement.get(), elseBlock);
    }

    BasicBlock::Ptr mergeBlock = createNewBlock();

    thenEndBlock->addSuccessor(mergeBlock);
    std::cout << "CFGBuilder: " << thenEndBlock->toString() << " -> " << mergeBlock->toString() << " (test merge)\n";
    if (elseEndBlock) {
        elseEndBlock->addSuccessor(mergeBlock);
        std::cout << "CFGBuilder: " << elseEndBlock->toString() << " -> " << mergeBlock->toString() << " (test merge)\n";
    } else {
        currentBlock->addSuccessor(mergeBlock);
        std::cout << "CFGBuilder: " << currentBlock->toString() << " -> " << mergeBlock->toString() << " (test merge, no else)\n";
    }

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleResultisStatement(ResultisStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling ResultisStatement in " << currentBlock->toString() << "\n";
    return addStatementToBlock(stmt, currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleEndcaseStatement(EndcaseStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling EndcaseStatement in " << currentBlock->toString() << "\n";
    // ENDCASE typically jumps to the end of the enclosing SWITCHON statement.
    // This requires context of the enclosing SWITCHON, which is not directly available here.
    // For now, treat it as terminating the current path, similar to GOTO.
    return addStatementToBlock(stmt, currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleFinishStatement(FinishStatement* stmt, BasicBlock::Ptr currentBlock) {
    std::cout << "CFGBuilder: Handling FinishStatement in " << currentBlock->toString() << "\n";
    // FINISH terminates the program or current routine.
    currentBlock = addStatementToBlock(stmt, currentBlock);
    return nullptr; // Terminates the path
}