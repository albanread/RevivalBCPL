#ifndef CFG_BUILDER_H
#define CFG_BUILDER_H

#include "AST.h" // Include AST.h to get StatementPtr and other AST types
#include "BasicBlock.h"
#include <map>
#include <vector>
#include <memory>

/**
 * @class CFGBuilder
 * @brief Builds a Control Flow Graph (CFG) from an Abstract Syntax Tree (AST).
 *
 * This class traverses the AST and constructs basic blocks, identifying control
 * flow edges (successors and predecessors) based on the semantics of statements
 * like if, while, for, routine calls, and jumps.
 */
class CFGBuilder {
public:
    CFGBuilder() : nextBlockId(0) {}

    /**
     * @brief Builds the CFG for a given program.
     * @param program The root of the AST.
     * @return A map of function names to their entry basic blocks.
     */
    std::map<std::string, BasicBlock::Ptr> build(ProgramPtr&& program);

private:
    int nextBlockId;
    std::map<std::string, BasicBlock::Ptr> functionEntryBlocks; // Maps function names to their entry block
    std::map<std::string, BasicBlock::Ptr> labels; // Maps label names to their basic blocks

    // Helper to create a new basic block
    BasicBlock::Ptr createNewBlock();

    // Recursive function to build CFG for a statement
    BasicBlock::Ptr buildCFGForStatement(std::unique_ptr<Statement> stmt, BasicBlock::Ptr currentBlock);

    // Specific statement handlers
    BasicBlock::Ptr handleCompoundStatement(std::unique_ptr<CompoundStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleIfStatement(std::unique_ptr<IfStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleWhileStatement(std::unique_ptr<WhileStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleForStatement(std::unique_ptr<ForStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleRoutineCall(std::unique_ptr<RoutineCall> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleReturnStatement(std::unique_ptr<ReturnStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleLoopStatement(std::unique_ptr<LoopStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleRepeatStatement(std::unique_ptr<RepeatStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleSwitchonStatement(std::unique_ptr<SwitchonStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleGotoStatement(std::unique_ptr<GotoStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleLabeledStatement(std::unique_ptr<LabeledStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleDeclarationStatement(std::unique_ptr<DeclarationStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleAssignment(std::unique_ptr<Assignment> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleTestStatement(std::unique_ptr<TestStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleResultisStatement(std::unique_ptr<ResultisStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleEndcaseStatement(std::unique_ptr<EndcaseStatement> stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleFinishStatement(std::unique_ptr<FinishStatement> stmt, BasicBlock::Ptr currentBlock);

    // Helper to add a statement to the current block or start a new one if needed
    BasicBlock::Ptr addStatementToBlock(std::unique_ptr<Statement> stmt, BasicBlock::Ptr currentBlock);
};

#endif // CFG_BUILDER_H