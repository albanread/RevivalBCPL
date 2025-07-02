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

    StmtPtr visit(Statement* node);
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
    StmtPtr visit(RepeatStatement* node);
    StmtPtr visit(SwitchonStatement* node);
    StmtPtr visit(EndcaseStatement* node);
    StmtPtr visit(DeclarationStatement* node);

    DeclPtr visit(Declaration* node);
    DeclPtr visit(FunctionDeclaration* node);
    ProgramPtr visit(Program* node);

    // Key transformation logic
    DeclPtr visit(LetDeclaration* node);
    StmtPtr visit(Assignment* node);

    // Other visitor declarations would follow the standard pattern...
};

#endif // DEAD_CODE_ELIMINATION_PASS_H
