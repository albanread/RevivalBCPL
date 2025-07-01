#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "AST.h"
#include "PassManager.h"
#include <memory>
#include <set>
#include <unordered_map>

/**
 * @class Optimizer
 * @brief Main optimization coordinator using a pass manager system.
 * 
 * The Optimizer class has been refactored to use a modular pass manager system.
 * Instead of implementing optimizations directly, it now:
 * - Sets up and configures optimization passes
 * - Delegates optimization work to a PassManager
 * - Maintains compatibility with existing code that expects the singleton pattern
 * 
 * The class still maintains the manifests map for sharing constant information
 * between passes, and provides the same public interface as before.
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
     * @brief Main optimization entry point using the pass manager system.
     * @param ast The program AST to optimize
     * @return Optimized program AST
     */
    ProgramPtr optimize(ProgramPtr ast);

    // Legacy visitor interface - kept for compatibility with LoopOptimizer
    // These methods are used by LoopOptimizer::process and delegate to passes
    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);

private:
    Optimizer() = default;
    
    /**
     * @brief Set up the optimization passes in the pass manager.
     * This method configures which passes to run and in what order.
     */
    void setupPasses();

    std::unique_ptr<PassManager> passManager;
    bool passesConfigured = false;

    // The following visitor methods are kept for compatibility with LoopOptimizer
    // They implement a simple pass-through visitor that doesn't do optimization
    
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
    
    // Top-level visitor
    ProgramPtr visit(Program* node);
};

#endif // OPTIMIZER_H
