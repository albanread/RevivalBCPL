#ifndef FUNCTION_INLINING_PASS_H
#define FUNCTION_INLINING_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <map>
#include <string>
#include <memory>

// A struct to hold information about an inlinable function.
struct InlinableFunction {
    const FunctionDeclaration* declaration;
    // You could add other metadata here, like function size.
};

/**
 * @class FunctionInliningPass
 * @brief Replaces calls to small, non-recursive functions with the function's body.
 */
class FunctionInliningPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    std::map<std::string, InlinableFunction> inlinableFunctions;

    // Stage 1: Analyze and find functions suitable for inlining.
    void findInlinableFunctions(Program* program);

    // Stage 2: Transform the AST by inlining calls.
    ProgramPtr visit(Program* node);
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    DeclPtr visit(FunctionDeclaration* node);
    StmtPtr visit(CompoundStatement* node);
    // ... other visitors

    ExprPtr visit(FunctionCall* node); // Core transformation logic here.
};

#endif // FUNCTION_INLINING_PASS_H
