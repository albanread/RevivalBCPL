#ifndef CONSTANT_FOLDING_PASS_H
#define CONSTANT_FOLDING_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <unordered_map>
#include <memory>

/**
 * @class ConstantFoldingPass
 * @brief Optimization pass that performs constant folding and basic algebraic simplifications.
 * 
 * This pass identifies constant expressions and evaluates them at compile time.
 * It also performs basic algebraic simplifications like:
 * - x + 0 = x
 * - x * 1 = x  
 * - x * 0 = 0
 * - Conditional expressions with constant conditions
 */
class ConstantFoldingPass : public OptimizationPass {
public:
    ConstantFoldingPass(std::unordered_map<std::string, int64_t>& manifests);
    
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    std::unordered_map<std::string, int64_t>& manifests;
    
    // Visitor pattern for transforming nodes
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    ProgramPtr visit(Program* node);
    
    // Expression visitors
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
    StmtPtr visit(RepeatStatement* node);
    StmtPtr visit(SwitchonStatement* node);
    StmtPtr visit(EndcaseStatement* node);
    
    // Declaration visitors
    DeclPtr visit(LetDeclaration* node);
    DeclPtr visit(FunctionDeclaration* node);
};

#endif // CONSTANT_FOLDING_PASS_H