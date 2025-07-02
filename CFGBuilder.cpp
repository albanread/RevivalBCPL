#include "CFGBuilder.h"
#include <iostream>

BasicBlock::Ptr CFGBuilder::createNewBlock() {
    return std::make_shared<BasicBlock>(nextBlockId++);
}

std::map<std::string, BasicBlock::Ptr> CFGBuilder::build(ProgramPtr&& program) {
    functionEntryBlocks.clear();
    nextBlockId = 0;

    for (auto& decl : program->declarations) { // Iterate by reference to allow moving out of unique_ptr
        if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            // Create an entry block for each function
            BasicBlock::Ptr entryBlock = createNewBlock();
            functionEntryBlocks[funcDecl->name] = entryBlock;

            // Build CFG for the function body
            if (funcDecl->body_stmt) {
                // We need to release the unique_ptr from funcDecl and pass it to buildCFGForStatement
                buildCFGForStatement(std::move(funcDecl->body_stmt), entryBlock);
            } 
            // else if (funcDecl->body_expr) {
            //     // For now, expressions as function bodies are treated as single statements
            //     // This might need refinement depending on how complex expressions are handled.
            //     entryBlock->addStatement(std::move(funcDecl->body_expr)); // Type mismatch: ExprPtr to StmtPtr
            // }
        }
        // TODO: Handle other top-level declarations if they contain executable code
    }
    return functionEntryBlocks;
}

BasicBlock::Ptr CFGBuilder::addStatementToBlock(std::unique_ptr<Statement> stmt, BasicBlock::Ptr currentBlock) {
    if (!currentBlock) {
        currentBlock = createNewBlock();
    }
    currentBlock->addStatement(std::move(stmt));
    return currentBlock;
}

BasicBlock::Ptr CFGBuilder::buildCFGForStatement(std::unique_ptr<Statement> stmt, BasicBlock::Ptr currentBlock) {
    if (!stmt) return currentBlock;

    // Handle different statement types
    if (CompoundStatement* compound = dynamic_cast<CompoundStatement*>(stmt.get())) {
        return handleCompoundStatement(std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(stmt.release())), currentBlock);
    } else if (IfStatement* ifStmt = dynamic_cast<IfStatement*>(stmt.get())) {
        return handleIfStatement(std::unique_ptr<IfStatement>(static_cast<IfStatement*>(stmt.release())), currentBlock);
    } else if (WhileStatement* whileStmt = dynamic_cast<WhileStatement*>(stmt.get())) {
        return handleWhileStatement(std::unique_ptr<WhileStatement>(static_cast<WhileStatement*>(stmt.release())), currentBlock);
    } else if (ForStatement* forStmt = dynamic_cast<ForStatement*>(stmt.get())) {
        return handleForStatement(std::unique_ptr<ForStatement>(static_cast<ForStatement*>(stmt.release())), currentBlock);
    } else if (RoutineCall* routineCall = dynamic_cast<RoutineCall*>(stmt.get())) {
        return handleRoutineCall(std::unique_ptr<RoutineCall>(static_cast<RoutineCall*>(stmt.release())), currentBlock);
    } else if (ReturnStatement* ret = dynamic_cast<ReturnStatement*>(stmt.get())) {
        return handleReturnStatement(std::unique_ptr<ReturnStatement>(static_cast<ReturnStatement*>(stmt.release())), currentBlock);
    } else if (LoopStatement* loop = dynamic_cast<LoopStatement*>(stmt.get())) {
        return handleLoopStatement(std::unique_ptr<LoopStatement>(static_cast<LoopStatement*>(stmt.release())), currentBlock);
    } else if (RepeatStatement* repeat = dynamic_cast<RepeatStatement*>(stmt.get())) {
        return handleRepeatStatement(std::unique_ptr<RepeatStatement>(static_cast<RepeatStatement*>(stmt.release())), currentBlock);
    } else if (SwitchonStatement* switchon = dynamic_cast<SwitchonStatement*>(stmt.get())) {
        return handleSwitchonStatement(std::unique_ptr<SwitchonStatement>(static_cast<SwitchonStatement*>(stmt.release())), currentBlock);
    } else if (GotoStatement* gotoStmt = dynamic_cast<GotoStatement*>(stmt.get())) {
        return handleGotoStatement(std::unique_ptr<GotoStatement>(static_cast<GotoStatement*>(stmt.release())), currentBlock);
    } else if (LabeledStatement* labeledStmt = dynamic_cast<LabeledStatement*>(stmt.get())) {
        return handleLabeledStatement(std::unique_ptr<LabeledStatement>(static_cast<LabeledStatement*>(stmt.release())), currentBlock);
    } else if (DeclarationStatement* declStmt = dynamic_cast<DeclarationStatement*>(stmt.get())) {
        return handleDeclarationStatement(std::unique_ptr<DeclarationStatement>(static_cast<DeclarationStatement*>(stmt.release())), currentBlock);
    } else if (Assignment* assign = dynamic_cast<Assignment*>(stmt.get())) {
        return handleAssignment(std::unique_ptr<Assignment>(static_cast<Assignment*>(stmt.release())), currentBlock);
    } else if (TestStatement* testStmt = dynamic_cast<TestStatement*>(stmt.get())) {
        return handleTestStatement(std::unique_ptr<TestStatement>(static_cast<TestStatement*>(stmt.release())), currentBlock);
    } else if (ResultisStatement* resultis = dynamic_cast<ResultisStatement*>(stmt.get())) {
        return handleResultisStatement(std::unique_ptr<ResultisStatement>(static_cast<ResultisStatement*>(stmt.release())), currentBlock);
    } else if (EndcaseStatement* endcase = dynamic_cast<EndcaseStatement*>(stmt.get())) {
        return handleEndcaseStatement(std::unique_ptr<EndcaseStatement>(static_cast<EndcaseStatement*>(stmt.release())), currentBlock);
    } else if (FinishStatement* finish = dynamic_cast<FinishStatement*>(stmt.get())) {
        return handleFinishStatement(std::unique_ptr<FinishStatement>(static_cast<FinishStatement*>(stmt.release())), currentBlock);
    } else {
        // Default: add statement to current block and continue
        return addStatementToBlock(std::move(stmt), currentBlock);
    }
}

// --- Specific Statement Handlers ---

BasicBlock::Ptr CFGBuilder::handleCompoundStatement(std::unique_ptr<CompoundStatement> stmt, BasicBlock::Ptr currentBlock) {
    CompoundStatement* rawStmt = stmt.get();

    for (auto& s_node_ptr : rawStmt->statements) { // s_node_ptr is std::unique_ptr<Node>&
        std::unique_ptr<Statement> s_stmt_ptr(static_cast<Statement*>(s_node_ptr.release()));
        currentBlock = buildCFGForStatement(std::move(s_stmt_ptr), currentBlock);
    }
    return currentBlock;
}

BasicBlock::Ptr CFGBuilder::handleIfStatement(std::unique_ptr<IfStatement> stmt, BasicBlock::Ptr currentBlock) {
    // Add the if condition to the current block
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);

    // Create blocks for then branch
    BasicBlock::Ptr thenBlock = createNewBlock();

    // Connect current block to then block
    currentBlock->addSuccessor(thenBlock);

    // Build CFG for then branch
    BasicBlock::Ptr thenEndBlock = buildCFGForStatement(std::move(stmt->then_statement), thenBlock);

    // Create a merge block
    BasicBlock::Ptr mergeBlock = createNewBlock();

    // Connect end of then branch to merge block
    thenEndBlock->addSuccessor(mergeBlock);

    // If no else branch, current block also connects to merge block
    currentBlock->addSuccessor(mergeBlock);

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleWhileStatement(std::unique_ptr<WhileStatement> stmt, BasicBlock::Ptr currentBlock) {
    // Create a block for the loop header (condition)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);

    // Add condition to loop header
    currentBlock = addStatementToBlock(std::move(stmt), loopHeaderBlock);

    // Create block for loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(loopBodyBlock);

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(std::move(stmt->body), loopBodyBlock);

    // Connect end of body back to loop header
    bodyEndBlock->addSuccessor(loopHeaderBlock);

    // Create exit block for the loop
    BasicBlock::Ptr exitBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(exitBlock); // Condition can lead to exit

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleForStatement(std::unique_ptr<ForStatement> stmt, BasicBlock::Ptr currentBlock) {
    // For statement is complex: initialization, condition, increment, body

    // Initialization part (part of current block or new block)
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock); // Add the for statement itself

    // Loop header (condition check)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);

    // Loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(loopBodyBlock);

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(std::move(stmt->body), loopBodyBlock);

    // Increment part (after body, before next condition check)
    // For simplicity, we'll assume increment is part of the body's end block or a new block leading to header
    // A more precise CFG would have a dedicated increment block.
    bodyEndBlock->addSuccessor(loopHeaderBlock);

    // Exit block
    BasicBlock::Ptr exitBlock = createNewBlock();
    loopHeaderBlock->addSuccessor(exitBlock);

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleRoutineCall(std::unique_ptr<RoutineCall> stmt, BasicBlock::Ptr currentBlock) {
    return addStatementToBlock(std::move(stmt), currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleReturnStatement(std::unique_ptr<ReturnStatement> stmt, BasicBlock::Ptr currentBlock) {
    // Return statement terminates the current control flow path
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);
    // No successors from a return statement within the function's CFG
    return nullptr; // Indicates that this path is terminated
}

BasicBlock::Ptr CFGBuilder::handleLoopStatement(std::unique_ptr<LoopStatement> stmt, BasicBlock::Ptr currentBlock) {
    // Add the LOOP statement itself to the current block
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);

    // Create a block for the loop header (where control re-enters)
    BasicBlock::Ptr loopHeaderBlock = createNewBlock();
    currentBlock->addSuccessor(loopHeaderBlock);

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

BasicBlock::Ptr CFGBuilder::handleRepeatStatement(std::unique_ptr<RepeatStatement> stmt, BasicBlock::Ptr currentBlock) {
    // REPEAT ... UNTIL is a post-test loop

    // Create block for loop body
    BasicBlock::Ptr loopBodyBlock = createNewBlock();
    currentBlock->addSuccessor(loopBodyBlock);

    // Build CFG for loop body
    BasicBlock::Ptr bodyEndBlock = buildCFGForStatement(std::move(stmt->body), loopBodyBlock);

    // Add condition to bodyEndBlock
    bodyEndBlock = addStatementToBlock(std::move(stmt), bodyEndBlock); // The UNTIL condition is part of the last block of the body

    // Create exit block
    BasicBlock::Ptr exitBlock = createNewBlock();

    // Connect bodyEndBlock to itself (for repeat) and to exitBlock (for until condition met)
    bodyEndBlock->addSuccessor(loopBodyBlock); // Loop back
    bodyEndBlock->addSuccessor(exitBlock);     // Exit condition

    return exitBlock;
}

BasicBlock::Ptr CFGBuilder::handleSwitchonStatement(std::unique_ptr<SwitchonStatement> stmt, BasicBlock::Ptr currentBlock) {
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);

    BasicBlock::Ptr mergeBlock = createNewBlock();

    // Access cases and default_case from the raw pointer after moving stmt
    SwitchonStatement* rawStmt = stmt.get();

    for (auto& scase : rawStmt->cases) {
        BasicBlock::Ptr caseBlock = createNewBlock();
        currentBlock->addSuccessor(caseBlock);
        BasicBlock::Ptr caseEndBlock = buildCFGForStatement(std::move(scase.statement), caseBlock);
        caseEndBlock->addSuccessor(mergeBlock);
    }

    if (rawStmt->default_case) {
        BasicBlock::Ptr defaultBlock = createNewBlock();
        currentBlock->addSuccessor(defaultBlock);
        BasicBlock::Ptr defaultEndBlock = buildCFGForStatement(std::move(rawStmt->default_case), defaultBlock);
        defaultEndBlock->addSuccessor(mergeBlock);
    }

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleGotoStatement(std::unique_ptr<GotoStatement> stmt, BasicBlock::Ptr currentBlock) {
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);
    // GOTO requires resolution of labels after all blocks are created.
    // For now, it terminates the current path. Label resolution will connect it later.
    return nullptr;
}

BasicBlock::Ptr CFGBuilder::handleLabeledStatement(std::unique_ptr<LabeledStatement> stmt, BasicBlock::Ptr currentBlock) {
    // A labeled statement can be a target of a GOTO.
    // Create a new block for the labeled statement if the current block is not empty
    // or if it's the start of a function.
    BasicBlock::Ptr labeledBlock = createNewBlock();
    // TODO: Store mapping from label name to labeledBlock for GOTO resolution

    if (currentBlock) {
        currentBlock->addSuccessor(labeledBlock);
    }

    // Continue building CFG from the labeled statement's body
    return buildCFGForStatement(std::move(stmt->statement), labeledBlock);
}

BasicBlock::Ptr CFGBuilder::handleDeclarationStatement(std::unique_ptr<DeclarationStatement> stmt, BasicBlock::Ptr currentBlock) {
    // Declarations are typically sequential and don't alter control flow directly.
    // However, initializers might contain expressions that need to be evaluated.
    // For simplicity, treat as a regular statement within the current block.
    return addStatementToBlock(std::move(stmt), currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleAssignment(std::unique_ptr<Assignment> stmt, BasicBlock::Ptr currentBlock) {
    return addStatementToBlock(std::move(stmt), currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleTestStatement(std::unique_ptr<TestStatement> stmt, BasicBlock::Ptr currentBlock) {
    // TEST is similar to IF, but with an OR branch
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);

    BasicBlock::Ptr thenBlock = createNewBlock();
    BasicBlock::Ptr elseBlock = nullptr;
    if (stmt->else_statement) {
        elseBlock = createNewBlock();
    }

    currentBlock->addSuccessor(thenBlock);
    if (elseBlock) {
        currentBlock->addSuccessor(elseBlock);
    }

    BasicBlock::Ptr thenEndBlock = buildCFGForStatement(std::move(stmt->then_statement), thenBlock);
    BasicBlock::Ptr elseEndBlock = nullptr;
    if (stmt->else_statement) {
        elseEndBlock = buildCFGForStatement(std::move(stmt->else_statement), elseBlock);
    }

    BasicBlock::Ptr mergeBlock = createNewBlock();

    thenEndBlock->addSuccessor(mergeBlock);
    if (elseEndBlock) {
        elseEndBlock->addSuccessor(mergeBlock);
    } else {
        currentBlock->addSuccessor(mergeBlock);
    }

    return mergeBlock;
}

BasicBlock::Ptr CFGBuilder::handleResultisStatement(std::unique_ptr<ResultisStatement> stmt, BasicBlock::Ptr currentBlock) {
    return addStatementToBlock(std::move(stmt), currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleEndcaseStatement(std::unique_ptr<EndcaseStatement> stmt, BasicBlock::Ptr currentBlock) {
    // ENDCASE typically jumps to the end of the enclosing SWITCHON statement.
    // This requires context of the enclosing SWITCHON, which is not directly available here.
    // For now, treat it as terminating the current path, similar to GOTO.
    return addStatementToBlock(std::move(stmt), currentBlock);
}

BasicBlock::Ptr CFGBuilder::handleFinishStatement(std::unique_ptr<FinishStatement> stmt, BasicBlock::Ptr currentBlock) {
    // FINISH terminates the program or current routine.
    currentBlock = addStatementToBlock(std::move(stmt), currentBlock);
    return nullptr; // Terminates the path
}
