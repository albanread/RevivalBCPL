#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "AST.h"
#include "PassManager.h"
#include <memory>
#include <set>
#include <unordered_map>

/**
 * @class Optimizer
 * @brief Main optimizer interface that uses a PassManager to apply optimization passes.
 * 
 * The Optimizer class has been refactored to use a modular pass-based architecture.
 * Instead of embedding all optimization logic directly, it delegates to a PassManager
 * that coordinates individual optimization passes.
 * 
 * This class retains the singleton pattern and visitor methods for compatibility
 * with existing code (like LoopOptimizer) but now uses passes for the main optimization.
 */
class Optimizer {
public:
    std::unordered_map<std::string, int64_t> manifests;

    static Optimizer& getInstance() {
        static Optimizer instance;
        return instance;
    }

    Optimizer(const Optimizer&) = delete;
    Optimizer& operator=(const Optimizer&) = delete;

    /**
     * @brief Optimize a program using the configured pass manager.
     * @param ast The program to optimize
     * @return The optimized program
     */
    ProgramPtr optimize(ProgramPtr ast);

    // Visitor methods retained for compatibility with existing code (e.g., LoopOptimizer)
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);

private:
    Optimizer();
    PassManager passManager;

    void setupDefaultPasses();

    // Expression visitors - kept for compatibility with LoopOptimizer
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
    
    // Statement visitors - kept for compatibility with LoopOptimizer
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
    StmtPtr visit(DeclarationStatement* node);
    
    // Declaration visitors - kept for compatibility with LoopOptimizer
    DeclPtr visit(LetDeclaration* node);
    DeclPtr visit(FunctionDeclaration* node);
    
    // Top-level visitor - kept for compatibility with LoopOptimizer
    ProgramPtr visit(Program* node);
};

#endif // OPTIMIZER_H
