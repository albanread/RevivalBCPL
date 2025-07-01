#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "AST.h"
#include <memory>
#include <set>
#include <unordered_map>

class Optimizer {
public:
    std::unordered_map<std::string, int64_t> manifests;

    static Optimizer& getInstance() {
        static Optimizer instance;
        return instance;
    }

    Optimizer(const Optimizer&) = delete;
    Optimizer& operator=(const Optimizer&) = delete;

    ProgramPtr optimize(ProgramPtr ast);

    ExprPtr visit(Expression* node);
    StmtPtr visit(Statement* node);
    DeclPtr visit(Declaration* node);

private:
    Optimizer() = default;

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
