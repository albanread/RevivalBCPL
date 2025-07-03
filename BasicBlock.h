#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "AST.h" // Include AST.h to get StatementPtr and other AST types
#include <vector>
#include <set>
#include <memory>

/**
 * @class BasicBlock
 * @brief Represents a basic block in a Control Flow Graph (CFG).
 *
 * A basic block is a sequence of statements that is entered only at the
 * beginning and exited only at the end.
 */
class BasicBlock : public std::enable_shared_from_this<BasicBlock> {
public:
    using Ptr = std::shared_ptr<BasicBlock>;

    // Unique identifier for the basic block
    int id;

    // Statements contained within this basic block (now raw pointers)
    std::vector<Statement*> statements;

    // Successor basic blocks
    std::set<Ptr> successors;

    // Predecessor basic blocks
    std::set<Ptr> predecessors;

    // Constructor
    BasicBlock(int block_id) : id(block_id) {}

    // Add a statement to the basic block (now takes raw pointer)
    void addStatement(Statement* stmt) {
        statements.push_back(stmt);
    }

    // Add a successor basic block
    void addSuccessor(Ptr succ) {
        if (succ) {
            successors.insert(succ);
            succ->predecessors.insert(shared_from_this());
        }
    }

    // For debugging/visualization
    std::string toString() const {
        return "BB" + std::to_string(id);
    }
};

#endif // BASIC_BLOCK_H