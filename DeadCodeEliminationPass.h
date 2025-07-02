#ifndef DEAD_CODE_ELIMINATION_PASS_H
#define DEAD_CODE_ELIMINATION_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <set>
#include <memory>

/**
 * @class DeadCodeEliminationPass
 * @brief Removes unused variable declarations and assignments.
 *
 * This pass operates in two stages:
 * 1. An analysis stage that traverses the AST to find all "live" variables.
 * 2. A transformation stage that removes declarations and assignments to "dead" variables.
 */
class DeadCodeEliminationPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    // --- Analysis Stage ---
    std::set<std::string> liveVariables;
    void analyzeNode(Node* node);

    // --- Transformation Stage ---
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    ProgramPtr visit(Program* node);

    // Key transformation logic
    DeclPtr visit(LetDeclaration* node);
    StmtPtr visit(Assignment* node);

    // Other visitor declarations would follow the standard pattern...
};

#endif // DEAD_CODE_ELIMINATION_PASS_H
