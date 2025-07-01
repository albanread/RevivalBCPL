#ifndef LOOPINVARIANTCODEMOTIONPASS_H
#define LOOPINVARIANTCODEMOTIONPASS_H

#include "OptimizationPass.h"
#include <unordered_map>

// Forward declaration to avoid circular includes
class Optimizer;

/**
 * @class LoopInvariantCodeMotionPass
 * @brief Optimization pass that performs loop-invariant code motion (LICM).
 * 
 * This pass identifies expressions within loops that are invariant (don't change
 * during loop execution) and hoists them outside the loop to avoid redundant
 * computation. It wraps the existing LoopOptimizer functionality in the new
 * pass manager system.
 * 
 * The pass analyzes for-loops and moves loop-invariant expressions into
 * temporary variables declared before the loop.
 */
class LoopInvariantCodeMotionPass : public OptimizationPass {
public:
    /**
     * @brief Constructor that takes manifest constants for the optimization.
     * @param manifests Map of manifest constant names to values
     */
    explicit LoopInvariantCodeMotionPass(const std::unordered_map<std::string, int64_t>& manifests);

    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    const std::unordered_map<std::string, int64_t>& manifests;

    // AST visitors that delegate to LoopOptimizer for ForStatement nodes
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    ProgramPtr visit(Program* node);

    // Expression visitors (pass-through)
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
    StmtPtr visit(ForStatement* node);  // This is where LICM happens
    StmtPtr visit(GotoStatement* node);
    StmtPtr visit(LabeledStatement* node);
    StmtPtr visit(ReturnStatement* node);
    StmtPtr visit(FinishStatement* node);
    StmtPtr visit(ResultisStatement* node);
    StmtPtr visit(RepeatStatement* node);
    StmtPtr visit(SwitchonStatement* node);
    StmtPtr visit(EndcaseStatement* node);
    
    // Declaration visitors (pass-through)
    DeclPtr visit(LetDeclaration* node);
    DeclPtr visit(FunctionDeclaration* node);

    // Helper to create a temporary Optimizer instance for LoopOptimizer
    std::unique_ptr<Optimizer> createOptimizerHelper();
};

#endif // LOOPINVARIANTCODEMOTIONPASS_H