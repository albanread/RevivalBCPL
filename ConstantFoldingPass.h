#ifndef CONSTANTFOLDINGPASS_H
#define CONSTANTFOLDINGPASS_H

#include "OptimizationPass.h"
#include <unordered_map>

/**
 * @class ConstantFoldingPass
 * @brief Optimization pass that performs constant folding throughout the AST.
 * 
 * This pass identifies constant expressions and evaluates them at compile-time,
 * replacing them with their computed values. It handles:
 * - Arithmetic operations on number and float literals
 * - Comparison operations
 * - Conditional expressions with constant conditions
 * - If/Test statements with constant conditions
 * - Identity operations (x + 0, x * 1, etc.)
 * - Strength reduction (multiply by 2 -> left shift by 1)
 */
class ConstantFoldingPass : public OptimizationPass {
public:
    /**
     * @brief Constructor that takes manifest constants for constant propagation.
     * @param manifests Map of manifest constant names to values
     */
    explicit ConstantFoldingPass(const std::unordered_map<std::string, int64_t>& manifests);

    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    const std::unordered_map<std::string, int64_t>& manifests;

    // AST visitors - these implement the actual constant folding logic
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

#endif // CONSTANTFOLDINGPASS_H