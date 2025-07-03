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
     */
    void build(const ProgramPtr& program);

    // Getter for functionEntryBlocks
    const std::map<std::string, BasicBlock::Ptr>& getFunctionEntryBlocks() const {
        return functionEntryBlocks;
    }

private:
    int nextBlockId;
    std::map<std::string, BasicBlock::Ptr> functionEntryBlocks; // Maps function names to their entry block
    std::map<std::string, BasicBlock::Ptr> labels; // Maps label names to their basic blocks

    // Helper to create a new basic block
    BasicBlock::Ptr createNewBlock();

    // Recursive function to build CFG for a statement
    BasicBlock::Ptr buildCFGForStatement(Statement* stmt, BasicBlock::Ptr currentBlock);

    // Specific statement handlers
    BasicBlock::Ptr handleCompoundStatement(CompoundStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleIfStatement(IfStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleWhileStatement(WhileStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleForStatement(ForStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleRoutineCall(RoutineCall* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleReturnStatement(ReturnStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleLoopStatement(LoopStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleRepeatStatement(RepeatStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleSwitchonStatement(SwitchonStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleGotoStatement(GotoStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleLabeledStatement(LabeledStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleDeclarationStatement(DeclarationStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleAssignment(Assignment* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleTestStatement(TestStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleResultisStatement(ResultisStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleEndcaseStatement(EndcaseStatement* stmt, BasicBlock::Ptr currentBlock);
    BasicBlock::Ptr handleFinishStatement(FinishStatement* stmt, BasicBlock::Ptr currentBlock);

    // Helper to add a statement to the current block or start a new one if needed
    BasicBlock::Ptr addStatementToBlock(Statement* stmt, BasicBlock::Ptr currentBlock);
};

#endif // CFG_BUILDER_H
