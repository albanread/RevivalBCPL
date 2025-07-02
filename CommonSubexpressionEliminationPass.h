#ifndef CSE_PASS_H
#define CSE_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <map>
#include <string>

/**
 * @class CommonSubexpressionEliminationPass
 * @brief Performs common subexpression elimination within basic blocks.
 */
class CommonSubexpressionEliminationPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    // Maps a string representation of an expression to the temp variable name
    std::map<std::string, std::string> availableExpressions;
    int tempVarCounter = 0;

    std::string generateTempVarName();
    std::string expressionToString(Expression* expr);

    // Main visitor methods
    StmtPtr visit(Statement* node);
    // ... other visitor declarations

    // We'll need to add hoisted declarations before the statements that use them.
    // This requires modifying how we visit CompoundStatements.
    StmtPtr visit(CompoundStatement* node);
};

#endif // CSE_PASS_H
