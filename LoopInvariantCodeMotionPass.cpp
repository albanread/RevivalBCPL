#include "LoopInvariantCodeMotionPass.h"
#include "LoopOptimizer.h"
#include "Optimizer.h"
#include <stdexcept>
#include <vector>

LoopInvariantCodeMotionPass::LoopInvariantCodeMotionPass(std::unordered_map<std::string, int64_t>& manifests)
    : manifests(manifests) {}

ProgramPtr LoopInvariantCodeMotionPass::apply(ProgramPtr program) {
    return visit(program.get());
}

std::string LoopInvariantCodeMotionPass::getName() const {
    return "Loop Invariant Code Motion Pass";
}

// --- Visitor Dispatchers ---

ExprPtr LoopInvariantCodeMotionPass::visit(Expression* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<NumberLiteral*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FloatLiteral*>(node)) return visit(n);
    if (auto* n = dynamic_cast<StringLiteral*>(node)) return visit(n);
    if (auto* n = dynamic_cast<CharLiteral*>(node)) return visit(n);
    if (auto* n = dynamic_cast<VariableAccess*>(node)) return visit(n);
    if (auto* n = dynamic_cast<UnaryOp*>(node)) return visit(n);
    if (auto* n = dynamic_cast<BinaryOp*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FunctionCall*>(node)) return visit(n);
    if (auto* n = dynamic_cast<ConditionalExpression*>(node)) return visit(n);
    if (auto* n = dynamic_cast<Valof*>(node)) return visit(n);
    if (auto* n = dynamic_cast<VectorConstructor*>(node)) return visit(n);
    if (auto* n = dynamic_cast<VectorAccess*>(node)) return visit(n);
    throw std::runtime_error("LoopInvariantCodeMotionPass: Unsupported Expression node.");
}

StmtPtr LoopInvariantCodeMotionPass::visit(Statement* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<Assignment*>(node)) return visit(n);
    if (auto* n = dynamic_cast<RoutineCall*>(node)) return visit(n);
    if (auto* n = dynamic_cast<CompoundStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<IfStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<TestStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<WhileStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<ForStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<GotoStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<LabeledStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<ReturnStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FinishStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<ResultisStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<RepeatStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<Declaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<SwitchonStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<EndcaseStatement*>(node)) return visit(n);
    throw std::runtime_error("LoopInvariantCodeMotionPass: Unsupported Statement node.");
}

DeclPtr LoopInvariantCodeMotionPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<LetDeclaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    if (dynamic_cast<GlobalDeclaration*>(node) || dynamic_cast<ManifestDeclaration*>(node) || dynamic_cast<GetDirective*>(node)) {
        return nullptr;
    }
    throw std::runtime_error("LoopInvariantCodeMotionPass: Unsupported Declaration node.");
}

// --- Top-Level and Declarations ---

ProgramPtr LoopInvariantCodeMotionPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr LoopInvariantCodeMotionPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> new_inits;
    for (const auto& init : node->initializers) {
        ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;
        new_inits.push_back({init.name, std::move(optimized_init)});
    }
    return std::make_unique<LetDeclaration>(std::move(new_inits));
}

DeclPtr LoopInvariantCodeMotionPass::visit(FunctionDeclaration* node) {
    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

// --- Expression Visitors (just copy nodes) ---

ExprPtr LoopInvariantCodeMotionPass::visit(NumberLiteral* node) { 
    return std::make_unique<NumberLiteral>(*node); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(FloatLiteral* node) { 
    return std::make_unique<FloatLiteral>(*node); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(StringLiteral* node) { 
    return std::make_unique<StringLiteral>(*node); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(CharLiteral* node) { 
    return std::make_unique<CharLiteral>(*node); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(VariableAccess* node) {
    return std::make_unique<VariableAccess>(*node);
}

ExprPtr LoopInvariantCodeMotionPass::visit(UnaryOp* node) {
    auto new_rhs = visit(node->rhs.get());
    return std::make_unique<UnaryOp>(node->op, std::move(new_rhs));
}

ExprPtr LoopInvariantCodeMotionPass::visit(BinaryOp* node) {
    auto left = visit(node->left.get());
    auto right = visit(node->right.get());
    return std::make_unique<BinaryOp>(node->op, std::move(left), std::move(right));
}

ExprPtr LoopInvariantCodeMotionPass::visit(ConditionalExpression* node) {
    auto new_cond = visit(node->condition.get());
    return std::make_unique<ConditionalExpression>(std::move(new_cond), visit(node->trueExpr.get()), visit(node->falseExpr.get()));
}

ExprPtr LoopInvariantCodeMotionPass::visit(FunctionCall* node) {
    auto new_func = visit(node->function.get());
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    return std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
}

ExprPtr LoopInvariantCodeMotionPass::visit(Valof* node) { 
    return std::make_unique<Valof>(visit(node->body.get())); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(VectorConstructor* node) { 
    return std::make_unique<VectorConstructor>(visit(node->size.get())); 
}

ExprPtr LoopInvariantCodeMotionPass::visit(VectorAccess* node) { 
    return std::make_unique<VectorAccess>(visit(node->vector.get()), visit(node->index.get())); 
}

// --- Statement Visitors ---

StmtPtr LoopInvariantCodeMotionPass::visit(CompoundStatement* node) {
    std::vector<StmtPtr> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(stmt.get())) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr LoopInvariantCodeMotionPass::visit(Assignment* node) {
    std::vector<ExprPtr> new_lhs;
    for (const auto& expr : node->lhs) {
        new_lhs.push_back(visit(expr.get()));
    }
    std::vector<ExprPtr> new_rhs;
    for (const auto& expr : node->rhs) {
        new_rhs.push_back(visit(expr.get()));
    }
    return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
}

StmtPtr LoopInvariantCodeMotionPass::visit(IfStatement* node) {
    auto new_cond = visit(node->condition.get());
    return std::make_unique<IfStatement>(std::move(new_cond), visit(node->then_statement.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr LoopInvariantCodeMotionPass::visit(WhileStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_body = visit(node->body.get());
    return std::make_unique<WhileStatement>(std::move(new_cond), std::move(new_body));
}

StmtPtr LoopInvariantCodeMotionPass::visit(RepeatStatement* node) {
    return std::make_unique<RepeatStatement>(visit(node->body.get()), visit(node->condition.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(ForStatement* node) {
    // This is where the actual LICM happens - delegate to the existing LoopOptimizer
    // We need to create a temporary Optimizer instance to use the existing logic
    Optimizer& optimizer = Optimizer::getInstance();
    optimizer.manifests = manifests;  // Set the manifests
    return LoopOptimizer::process(node, &optimizer);
}

StmtPtr LoopInvariantCodeMotionPass::visit(RoutineCall* node) {
    return std::make_unique<RoutineCall>(visit(node->call_expression.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(LabeledStatement* node) {
    return std::make_unique<LabeledStatement>(node->name, visit(node->statement.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(GotoStatement* node) {
    return std::make_unique<GotoStatement>(visit(node->label.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(ResultisStatement* node) {
    return std::make_unique<ResultisStatement>(visit(node->value.get()));
}

StmtPtr LoopInvariantCodeMotionPass::visit(ReturnStatement* node) {
    return std::make_unique<ReturnStatement>();
}

StmtPtr LoopInvariantCodeMotionPass::visit(FinishStatement* node) {
    return std::make_unique<FinishStatement>();
}

StmtPtr LoopInvariantCodeMotionPass::visit(SwitchonStatement* node) {
    auto new_expr = visit(node->expression.get());
    std::vector<SwitchonStatement::SwitchCase> new_cases;
    for (auto& scase : node->cases) {
        new_cases.push_back({scase.value, scase.label, visit(scase.statement.get())});
    }
    auto new_default = node->default_case ? visit(node->default_case.get()) : nullptr;
    return std::make_unique<SwitchonStatement>(std::move(new_expr), std::move(new_cases), std::move(new_default));
}

StmtPtr LoopInvariantCodeMotionPass::visit(EndcaseStatement* node) {
    return std::make_unique<EndcaseStatement>(); // No children to optimize
}