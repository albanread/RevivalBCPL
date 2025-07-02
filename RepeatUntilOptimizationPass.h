#ifndef REPEAT_UNTIL_OPTIMIZATION_PASS_H
#define REPEAT_UNTIL_OPTIMIZATION_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <unordered_map>
#include <memory>

/**
 * @class RepeatUntilOptimizationPass
 * @brief Optimizes REPEAT UNTIL statements with constant conditions.
 *
 * This pass performs the following transformations:
 * - REPEAT <body> UNTIL <true>  => <body>
 * - REPEAT <body> UNTIL <false> => WHILE <true> DO <body>
 */
class RepeatUntilOptimizationPass : public OptimizationPass {
public:
    RepeatUntilOptimizationPass(std::unordered_map<std::string, int64_t>& manifests);

    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    std::unordered_map<std::string, int64_t>& manifests;

    // Visitor pattern methods
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    ProgramPtr visit(Program* node);

    // Expression visitors (mostly pass-through)
    ExprPtr visit(NumberLiteral* node);
    ExprPtr visit(FloatLiteral* node);
    ExprPtr visit(StringLiteral* node);
    ExprPtr visit(CharLiteral* node);
    ExprPtr visit(VariableAccess* node);
    ExprPtr visit(UnaryOp* node);
    ExprPtr visit(BinaryOp* node);
    ExprPtr visit(FunctionCall* node);
    ExprPtr visit(ConditionalExpression* node);
    ExprPtr visit(Valof* node);
    ExprPtr visit(VectorConstructor* node);
    ExprPtr visit(VectorAccess* node);

    // Statement visitors
    StmtPtr visit(Assignment* node);
    StmtPtr visit(RoutineCall* node);
    StmtPtr visit(CompoundStatement* node);
    StmtPtr visit(IfStatement* node);
    StmtPtr visit(TestStatement* node);
    StmtPtr visit(WhileStatement* node);
    StmtPtr visit(ForStatement* node);
    StmtPtr visit(GotoStatement* node);
    StmtPtr visit(LabeledStatement* node);
    StmtPtr visit(ReturnStatement* node);
    StmtPtr visit(FinishStatement* node);
    StmtPtr visit(ResultisStatement* node);
    StmtPtr visit(RepeatStatement* node); // Key optimization logic is here
    StmtPtr visit(SwitchonStatement* node);
    StmtPtr visit(EndcaseStatement* node);
    StmtPtr visit(DeclarationStatement* node);

    // Declaration visitors
    DeclPtr visit(LetDeclaration* node);
    DeclPtr visit(FunctionDeclaration* node);
};

#endif // REPEAT_UNTIL_OPTIMIZATION_PASS_H
