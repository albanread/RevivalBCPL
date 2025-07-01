#ifndef LOOP_INVARIANT_CODE_MOTION_PASS_H
#define LOOP_INVARIANT_CODE_MOTION_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include <unordered_map>
#include <memory>

// Forward declaration
class Optimizer;

/**
 * @class LoopInvariantCodeMotionPass
 * @brief Optimization pass that performs loop-invariant code motion (LICM).
 * 
 * This pass identifies expressions within loops that don't depend on the loop variable
 * or variables modified within the loop, and moves them outside the loop to reduce
 * redundant computation.
 */
class LoopInvariantCodeMotionPass : public OptimizationPass {
public:
    LoopInvariantCodeMotionPass(std::unordered_map<std::string, int64_t>& manifests);
    
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

private:
    std::unordered_map<std::string, int64_t>& manifests;
    
    // Visitor pattern for transforming nodes - only transforms ForStatements
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);
    ProgramPtr visit(Program* node);
    
    // Expression visitors - just copy nodes
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
    
    // Statement visitors - only ForStatement does LICM
    StmtPtr visit(Assignment* node);
    StmtPtr visit(RoutineCall* node);
    StmtPtr visit(CompoundStatement* node);
    StmtPtr visit(IfStatement* node);
    StmtPtr visit(TestStatement* node);
    StmtPtr visit(WhileStatement* node);
    StmtPtr visit(ForStatement* node);  // This one does the actual LICM
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

#endif // LOOP_INVARIANT_CODE_MOTION_PASS_H