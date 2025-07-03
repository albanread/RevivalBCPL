#include "DeadCodeEliminationPass.h"
#include "Utils.h" // Include common utility functions
#include <vector>
#include <iostream>

// --- Public Methods ---
std::string DeadCodeEliminationPass::getName() const {
    return "Dead Code Elimination Pass";
}

ProgramPtr DeadCodeEliminationPass::apply(ProgramPtr program) {
    std::cout << "\n=== Dead Code Elimination Pass: Starting ===\n";
    ProgramPtr optimizedProgram = visit(program.get());
    std::cout << "\n=== Dead Code Elimination Pass: Finished ===\n";
    return optimizedProgram;
}

// --- Transformation Stage Implementation ---
DeclPtr DeadCodeEliminationPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> liveInits;
    for (const auto& init : node->initializers) {
        std::cout << "DCE: Processing LET declaration for variable: " << init.name << "\n";
        // Recursively visit the initializer expression
        ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;

        // Check if the declared variable is live after this declaration statement.
        // This requires the LivenessAnalysisPass to have populated live-out for statements.
        // If the variable is part of the live-out set of the statement containing this declaration, keep it.
        // Note: This logic assumes the 'node' (LetDeclaration) can be mapped to a Statement* for liveness lookup.
        // In a real scenario, you might need to find the enclosing Statement for this declaration.
        // For simplicity, we'll check if the variable is live-out of the *function* containing this declaration.
        // A more precise check would be against the live-out of the specific DeclarationStatement.
        
        // For now, we'll keep all LET declarations to avoid incorrect removal until liveness is fully integrated
        // and the mapping from LetDeclaration to its containing Statement is clear.
        // The actual DCE logic for LET will go here once liveness information is reliably available.
        
        // Temporarily, always keep LET declarations to avoid breaking compilation.
        liveInits.push_back({init.name, std::move(optimized_init)});
        std::cout << "DCE: Keeping LET declaration for " << init.name << " (DCE for LET not fully implemented yet).\n";
    }

    if (liveInits.empty()) {
        std::cout << "DCE: Removing empty LET declaration.\n";
        return nullptr; // Remove the entire LET statement if all its variables are dead.
    }
    return std::make_unique<LetDeclaration>(std::move(liveInits));
}

StmtPtr DeadCodeEliminationPass::visit(Assignment* node) {
    std::cout << "DCE: Processing Assignment statement.\n";
    // First, optimize the RHS expressions
    std::vector<ExprPtr> new_lhs;
    for (const auto& expr : node->lhs) {
        new_lhs.push_back(visit(expr.get()));
    }
    std::vector<ExprPtr> new_rhs;
    for (const auto& expr : node->rhs) {
        new_rhs.push_back(visit(expr.get()));
    }

    // Check if the assignment is dead
    if (node->lhs.size() == 1) {
        if (auto* var_access = dynamic_cast<VariableAccess*>(node->lhs[0].get())) {
            std::string assigned_var_name = var_access->name;
            std::cout << "DCE: Checking assignment to variable: " << assigned_var_name << "\n";

            const std::set<std::string>& live_out = livenessAnalysis->getLiveOut(node);
            printSet("DCE: Live-Out for this statement", live_out);

            if (live_out.find(assigned_var_name) == live_out.end()) {
                // The assigned variable is not live, so this assignment is dead.
                std::cout << "DCE: Variable " << assigned_var_name << " is not live. Eliminating assignment.\n";
                // Replace it with an empty compound statement.
                return std::make_unique<CompoundStatement>(std::vector<std::unique_ptr<Node>>());
            } else {
                std::cout << "DCE: Variable " << assigned_var_name << " is live. Keeping assignment.\n";
            }
        }
    }
    // If not dead, return the (potentially optimized) assignment
    return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
}

// Other visit methods would be implemented to traverse the tree...

ProgramPtr DeadCodeEliminationPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr DeadCodeEliminationPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<LetDeclaration*>(node)) return visit(n);
    // For other declarations, just clone them for now
    return node->cloneDecl();
}

DeclPtr DeadCodeEliminationPass::visit(FunctionDeclaration* node) {
    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

ExprPtr DeadCodeEliminationPass::visit(Expression* node) {
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
    throw std::runtime_error("DCE Pass: Unsupported Expression node.");
}

ExprPtr DeadCodeEliminationPass::visit(NumberLiteral* node) { return node->cloneExpr(); }
ExprPtr DeadCodeEliminationPass::visit(FloatLiteral* node) { return node->cloneExpr(); }
ExprPtr DeadCodeEliminationPass::visit(StringLiteral* node) { return node->cloneExpr(); }
ExprPtr DeadCodeEliminationPass::visit(CharLiteral* node) { return node->cloneExpr(); }
ExprPtr DeadCodeEliminationPass::visit(VariableAccess* node) { return node->cloneExpr(); }

ExprPtr DeadCodeEliminationPass::visit(UnaryOp* node) {
    auto new_rhs = visit(node->rhs.get());
    return std::make_unique<UnaryOp>(node->op, std::move(new_rhs));
}

ExprPtr DeadCodeEliminationPass::visit(BinaryOp* node) {
    auto new_left = visit(node->left.get());
    auto new_right = visit(node->right.get());
    return std::make_unique<BinaryOp>(node->op, std::move(new_left), std::move(new_right));
}

ExprPtr DeadCodeEliminationPass::visit(FunctionCall* node) {
    auto new_func = visit(node->function.get());
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    return std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
}

ExprPtr DeadCodeEliminationPass::visit(ConditionalExpression* node) {
    auto new_cond = visit(node->condition.get());
    auto new_true = visit(node->trueExpr.get());
    auto new_false = visit(node->falseExpr.get());
    return std::make_unique<ConditionalExpression>(std::move(new_cond), std::move(new_true), std::move(new_false));
}

ExprPtr DeadCodeEliminationPass::visit(Valof* node) {
    auto new_body = visit(node->body.get());
    return std::make_unique<Valof>(std::move(new_body));
}

ExprPtr DeadCodeEliminationPass::visit(VectorConstructor* node) {
    auto new_size = visit(node->size.get());
    return std::make_unique<VectorConstructor>(std::move(new_size));
}

ExprPtr DeadCodeEliminationPass::visit(VectorAccess* node) {
    auto new_vec = visit(node->vector.get());
    auto new_idx = visit(node->index.get());
    return std::make_unique<VectorAccess>(std::move(new_vec), std::move(new_idx));
}

StmtPtr DeadCodeEliminationPass::visit(Statement* node) {
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
    throw std::runtime_error("DCE Pass: Unsupported Statement node.");
}

StmtPtr DeadCodeEliminationPass::visit(CompoundStatement* node) {
    std::vector<std::unique_ptr<Node>> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(static_cast<Statement*>(stmt.get()))) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr DeadCodeEliminationPass::visit(RoutineCall* node) {
    auto new_call_expr = visit(node->call_expression.get());
    return std::make_unique<RoutineCall>(std::move(new_call_expr));
}

StmtPtr DeadCodeEliminationPass::visit(IfStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    return std::make_unique<IfStatement>(std::move(new_cond), std::move(new_then));
}

StmtPtr DeadCodeEliminationPass::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr DeadCodeEliminationPass::visit(WhileStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_body = visit(node->body.get());
    return std::make_unique<WhileStatement>(std::move(new_cond), std::move(new_body));
}

StmtPtr DeadCodeEliminationPass::visit(ForStatement* node) {
    auto new_from = visit(node->from_expr.get());
    auto new_to = visit(node->to_expr.get());
    auto new_by = node->by_expr ? visit(node->by_expr.get()) : nullptr;
    auto new_body = visit(node->body.get());
    return std::make_unique<ForStatement>(node->var_name, std::move(new_from), std::move(new_to), std::move(new_by), std::move(new_body));
}

StmtPtr DeadCodeEliminationPass::visit(GotoStatement* node) {
    auto new_label = visit(node->label.get());
    return std::make_unique<GotoStatement>(std::move(new_label));
}

StmtPtr DeadCodeEliminationPass::visit(LabeledStatement* node) {
    auto new_stmt = visit(node->statement.get());
    return std::make_unique<LabeledStatement>(node->name, std::move(new_stmt));
}

StmtPtr DeadCodeEliminationPass::visit(ReturnStatement* node) {
    return std::make_unique<ReturnStatement>();
}

StmtPtr DeadCodeEliminationPass::visit(FinishStatement* node) {
    return std::make_unique<FinishStatement>();
}

StmtPtr DeadCodeEliminationPass::visit(ResultisStatement* node) {
    auto new_value = visit(node->value.get());
    return std::make_unique<ResultisStatement>(std::move(new_value));
}

StmtPtr DeadCodeEliminationPass::visit(RepeatStatement* node) {
    auto new_body = visit(node->body.get());
    auto new_cond = visit(node->condition.get());
    return std::make_unique<RepeatStatement>(std::move(new_body), std::move(new_cond));
}

StmtPtr DeadCodeEliminationPass::visit(SwitchonStatement* node) {
    auto new_expr = visit(node->expression.get());
    std::vector<SwitchonStatement::SwitchCase> new_cases;
    for (auto& scase : node->cases) {
        new_cases.push_back({scase.value, scase.label, visit(scase.statement.get())});
    }
    auto new_default = node->default_case ? visit(node->default_case.get()) : nullptr;
    return std::make_unique<SwitchonStatement>(std::move(new_expr), std::move(new_cases), std::move(new_default));
}

StmtPtr DeadCodeEliminationPass::visit(EndcaseStatement* node) {
    return std::make_unique<EndcaseStatement>();
}

StmtPtr DeadCodeEliminationPass::visit(DeclarationStatement* node) {
    // Optimize the declaration itself, then wrap it back in a DeclarationStatement
    if (DeclPtr optimized_decl = visit(node->declaration.get())) {
        return std::make_unique<DeclarationStatement>(std::move(optimized_decl));
    }
    return nullptr; // If the declaration is optimized away, return nullptr
}