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
    ProgramPtr visit(Program* node);
    DeclPtr visit(Declaration* node);
    DeclPtr visit(FunctionDeclaration* node);
    DeclPtr visit(LetDeclaration* node);
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
    StmtPtr visit(Assignment* node);
    StmtPtr visit(RoutineCall* node);
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

    // We'll need to add hoisted declarations before the statements that use them.
    // This requires modifying how we visit CompoundStatements.
    StmtPtr visit(CompoundStatement* node);
};

#endif // CSE_PASS_H
