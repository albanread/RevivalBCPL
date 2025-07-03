#include "RepeatUntilOptimizationPass.h"
#include <stdexcept>
#include <vector>

RepeatUntilOptimizationPass::RepeatUntilOptimizationPass(std::unordered_map<std::string, int64_t>& manifests)
    : manifests(manifests) {}

ProgramPtr RepeatUntilOptimizationPass::apply(ProgramPtr program) {
    return visit(program.get());
}

std::string RepeatUntilOptimizationPass::getName() const {
    return "Repeat Until Optimization Pass";
}

// --- Key Optimization Logic ---
// Fix for RepeatUntilOptimizationPass.cpp
StmtPtr RepeatUntilOptimizationPass::visit(RepeatStatement* node) {
    // First, optimize the body and condition expressions themselves.
    auto new_body = visit(node->body.get());
    auto new_cond = node->condition ? visit(node->condition.get()) : nullptr;

    // Check if the optimized condition is a constant.
    if (auto* cond_lit = dynamic_cast<NumberLiteral*>(new_cond.get())) {
        // Condition is UNTIL <true> (non-zero in BCPL)
        if (cond_lit->value != 0) {
            // The loop runs exactly once. Replace the loop with its body.
            return new_body;
        } else {
            // Condition is UNTIL <false> (zero). This is an infinite loop.
            // Transform into 'WHILE true DO <body>' to preserve semantics.
            // BCPL 'true' is represented by -1.
            auto true_condition = std::make_unique<NumberLiteral>(-1);
            return std::make_unique<WhileStatement>(std::move(true_condition), std::move(new_body));
        }
    }

    // If the condition is not a constant, just return the optimized loop.
    return std::make_unique<RepeatStatement>(
        std::move(new_body),
        std::move(new_cond),
        node->loopType  // Pass through the original loop type
    );
}

// --- Boilerplate Visitor Implementation (Pass-through) ---

// Most of the following visitors are boilerplate pass-through implementations
// that recursively visit children and rebuild the AST nodes. This pattern
// is copied from your existing ConstantFoldingPass.

ExprPtr RepeatUntilOptimizationPass::visit(Expression* node) {
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
    throw std::runtime_error("RepeatUntilOptimizationPass: Unsupported Expression node.");
}

StmtPtr RepeatUntilOptimizationPass::visit(Statement* node) {
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
    if (auto* n = dynamic_cast<SwitchonStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<EndcaseStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<DeclarationStatement*>(node)) return visit(n);

    throw std::runtime_error("RepeatUntilOptimizationPass: Unsupported Statement node.");
}

DeclPtr RepeatUntilOptimizationPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<LetDeclaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    if (dynamic_cast<GlobalDeclaration*>(node) || dynamic_cast<ManifestDeclaration*>(node) || dynamic_cast<GetDirective*>(node)) {
        return nullptr;
    }
    throw std::runtime_error("RepeatUntilOptimizationPass: Unsupported Declaration node.");
}

ProgramPtr RepeatUntilOptimizationPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr RepeatUntilOptimizationPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> new_inits;
    for (const auto& init : node->initializers) {
        ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;
        new_inits.push_back({init.name, std::move(optimized_init)});
    }
    return std::make_unique<LetDeclaration>(std::move(new_inits));
}

DeclPtr RepeatUntilOptimizationPass::visit(FunctionDeclaration* node) {
    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

ExprPtr RepeatUntilOptimizationPass::visit(NumberLiteral* node) { return std::make_unique<NumberLiteral>(*node); }
ExprPtr RepeatUntilOptimizationPass::visit(FloatLiteral* node) { return std::make_unique<FloatLiteral>(*node); }
ExprPtr RepeatUntilOptimizationPass::visit(StringLiteral* node) { return std::make_unique<StringLiteral>(*node); }
ExprPtr RepeatUntilOptimizationPass::visit(CharLiteral* node) { return std::make_unique<CharLiteral>(*node); }

ExprPtr RepeatUntilOptimizationPass::visit(VariableAccess* node) {
    auto it = manifests.find(node->name);
    if (it != manifests.end()) {
        return std::make_unique<NumberLiteral>(it->second);
    }
    return std::make_unique<VariableAccess>(*node);
}

ExprPtr RepeatUntilOptimizationPass::visit(UnaryOp* node) {
    return std::make_unique<UnaryOp>(node->op, visit(node->rhs.get()));
}

ExprPtr RepeatUntilOptimizationPass::visit(BinaryOp* node) {
    return std::make_unique<BinaryOp>(node->op, visit(node->left.get()), visit(node->right.get()));
}

ExprPtr RepeatUntilOptimizationPass::visit(FunctionCall* node) {
    auto new_func = visit(node->function.get());
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    return std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
}

ExprPtr RepeatUntilOptimizationPass::visit(ConditionalExpression* node) {
    return std::make_unique<ConditionalExpression>(visit(node->condition.get()), visit(node->trueExpr.get()), visit(node->falseExpr.get()));
}

ExprPtr RepeatUntilOptimizationPass::visit(Valof* node) { return std::make_unique<Valof>(visit(node->body.get())); }
ExprPtr RepeatUntilOptimizationPass::visit(VectorConstructor* node) { return std::make_unique<VectorConstructor>(visit(node->size.get())); }
ExprPtr RepeatUntilOptimizationPass::visit(VectorAccess* node) { return std::make_unique<VectorAccess>(visit(node->vector.get()), visit(node->index.get())); }

StmtPtr RepeatUntilOptimizationPass::visit(CompoundStatement* node) {
    std::vector<std::unique_ptr<Node>> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(static_cast<Statement*>(stmt.get()))) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr RepeatUntilOptimizationPass::visit(Assignment* node) {
    std::vector<ExprPtr> new_lhs;
    for (const auto& expr : node->lhs) new_lhs.push_back(visit(expr.get()));
    std::vector<ExprPtr> new_rhs;
    for (const auto& expr : node->rhs) new_rhs.push_back(visit(expr.get()));
    return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
}

StmtPtr RepeatUntilOptimizationPass::visit(IfStatement* node) {
    return std::make_unique<IfStatement>(visit(node->condition.get()), visit(node->then_statement.get()));
}

StmtPtr RepeatUntilOptimizationPass::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr RepeatUntilOptimizationPass::visit(WhileStatement* node) {
    return std::make_unique<WhileStatement>(visit(node->condition.get()), visit(node->body.get()));
}

StmtPtr RepeatUntilOptimizationPass::visit(ForStatement* node) {
    auto new_from = visit(node->from_expr.get());
    auto new_to = visit(node->to_expr.get());
    auto new_by = node->by_expr ? visit(node->by_expr.get()) : nullptr;
    auto new_body = visit(node->body.get());
    return std::make_unique<ForStatement>(node->var_name, std::move(new_from), std::move(new_to), std::move(new_by), std::move(new_body));
}

StmtPtr RepeatUntilOptimizationPass::visit(RoutineCall* node) { return std::make_unique<RoutineCall>(visit(node->call_expression.get())); }
StmtPtr RepeatUntilOptimizationPass::visit(LabeledStatement* node) { return std::make_unique<LabeledStatement>(node->name, visit(node->statement.get())); }
StmtPtr RepeatUntilOptimizationPass::visit(GotoStatement* node) { return std::make_unique<GotoStatement>(visit(node->label.get())); }
StmtPtr RepeatUntilOptimizationPass::visit(ResultisStatement* node) { return std::make_unique<ResultisStatement>(visit(node->value.get())); }
StmtPtr RepeatUntilOptimizationPass::visit(ReturnStatement* node) { return std::make_unique<ReturnStatement>(); }
StmtPtr RepeatUntilOptimizationPass::visit(FinishStatement* node) { return std::make_unique<FinishStatement>(); }

StmtPtr RepeatUntilOptimizationPass::visit(SwitchonStatement* node) {
    auto new_expr = visit(node->expression.get());
    std::vector<SwitchonStatement::SwitchCase> new_cases;
    for (auto& scase : node->cases) {
        new_cases.push_back({scase.value, scase.label, visit(scase.statement.get())});
    }
    auto new_default = node->default_case ? visit(node->default_case.get()) : nullptr;
    return std::make_unique<SwitchonStatement>(std::move(new_expr), std::move(new_cases), std::move(new_default));
}

StmtPtr RepeatUntilOptimizationPass::visit(EndcaseStatement* node) {
    return std::make_unique<EndcaseStatement>();
}

StmtPtr RepeatUntilOptimizationPass::visit(DeclarationStatement* node) {
    // Optimize the declaration itself, then wrap it back in a DeclarationStatement
    if (DeclPtr optimized_decl = visit(node->declaration.get())) {
        return std::make_unique<DeclarationStatement>(std::move(optimized_decl));
    }
    return nullptr; // If the declaration is optimized away, return nullptr
}
