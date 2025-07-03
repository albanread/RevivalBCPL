#include "CommonSubexpressionEliminationPass.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

std::string CommonSubexpressionEliminationPass::getName() const {
    return "Common Subexpression Elimination Pass";
}

std::string CommonSubexpressionEliminationPass::generateTempVarName() {
    return "_cse_temp_" + std::to_string(tempVarCounter++);
}

// A helper to create a canonical string for an expression
std::string CommonSubexpressionEliminationPass::expressionToString(Expression* expr) {
    if (!expr) return "null";

    if (auto* num = dynamic_cast<NumberLiteral*>(expr)) {
        return std::to_string(num->value);
    } else if (auto* var = dynamic_cast<VariableAccess*>(expr)) {
        return var->name;
    } else if (auto* unary = dynamic_cast<UnaryOp*>(expr)) {
        std::stringstream ss;
        ss << "(" << Token::tokenTypeToString(unary->op) << " " << expressionToString(unary->rhs.get()) << ")";
        return ss.str();
    } else if (auto* binary = dynamic_cast<BinaryOp*>(expr)) {
        std::stringstream ss;
        ss << "(" << Token::tokenTypeToString(binary->op) << " "
           << expressionToString(binary->left.get()) << " "
           << expressionToString(binary->right.get()) << ")";
        return ss.str();
    }
    // Add more expression types as needed
    return "unsupported_expr";
}

ProgramPtr CommonSubexpressionEliminationPass::apply(ProgramPtr program) {
    // Reset for each application of the pass
    availableExpressions.clear();
    tempVarCounter = 0;
    return visit(program.get());
}

// --- Visitor Implementations ---

ProgramPtr CommonSubexpressionEliminationPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr CommonSubexpressionEliminationPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<LetDeclaration*>(node)) return visit(n);
    // For other declarations, just clone them for now
    return node->cloneDecl();
}

DeclPtr CommonSubexpressionEliminationPass::visit(FunctionDeclaration* node) {
    // Reset available expressions for each function
    availableExpressions.clear();
    tempVarCounter = 0;

    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

DeclPtr CommonSubexpressionEliminationPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> new_inits;
    for (const auto& init : node->initializers) {
        ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;
        new_inits.push_back({init.name, std::move(optimized_init)});
    }
    return std::make_unique<LetDeclaration>(std::move(new_inits));
}

ExprPtr CommonSubexpressionEliminationPass::visit(Expression* node) {
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
    throw std::runtime_error("CSE Pass: Unsupported Expression node.");
}

ExprPtr CommonSubexpressionEliminationPass::visit(NumberLiteral* node) { return node->cloneExpr(); }
ExprPtr CommonSubexpressionEliminationPass::visit(FloatLiteral* node) { return node->cloneExpr(); }
ExprPtr CommonSubexpressionEliminationPass::visit(StringLiteral* node) { return node->cloneExpr(); }
ExprPtr CommonSubexpressionEliminationPass::visit(CharLiteral* node) { return node->cloneExpr(); }
ExprPtr CommonSubexpressionEliminationPass::visit(VariableAccess* node) { return node->cloneExpr(); }

ExprPtr CommonSubexpressionEliminationPass::visit(UnaryOp* node) {
    auto new_rhs = visit(node->rhs.get());
    return std::make_unique<UnaryOp>(node->op, std::move(new_rhs));
}

ExprPtr CommonSubexpressionEliminationPass::visit(BinaryOp* node) {
    auto new_left = visit(node->left.get());
    auto new_right = visit(node->right.get());
    return std::make_unique<BinaryOp>(node->op, std::move(new_left), std::move(new_right));
}

ExprPtr CommonSubexpressionEliminationPass::visit(FunctionCall* node) {
    auto new_func = visit(node->function.get());
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    return std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
}

ExprPtr CommonSubexpressionEliminationPass::visit(ConditionalExpression* node) {
    auto new_cond = visit(node->condition.get());
    auto new_true = visit(node->trueExpr.get());
    auto new_false = visit(node->falseExpr.get());
    return std::make_unique<ConditionalExpression>(std::move(new_cond), std::move(new_true), std::move(new_false));
}

ExprPtr CommonSubexpressionEliminationPass::visit(Valof* node) {
    auto new_body = visit(node->body.get());
    return std::make_unique<Valof>(std::move(new_body));
}

ExprPtr CommonSubexpressionEliminationPass::visit(VectorConstructor* node) {
    auto new_size = visit(node->size.get());
    return std::make_unique<VectorConstructor>(std::move(new_size));
}

ExprPtr CommonSubexpressionEliminationPass::visit(VectorAccess* node) {
    auto new_vec = visit(node->vector.get());
    auto new_idx = visit(node->index.get());
    return std::make_unique<VectorAccess>(std::move(new_vec), std::move(new_idx));
}

StmtPtr CommonSubexpressionEliminationPass::visit(Statement* node) {
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
    throw std::runtime_error("CSE Pass: Unsupported Statement node.");
}

StmtPtr CommonSubexpressionEliminationPass::visit(Assignment* node) {
    std::vector<ExprPtr> new_lhs;
    for (const auto& expr : node->lhs) {
        new_lhs.push_back(visit(expr.get()));
    }
    std::vector<ExprPtr> new_rhs;
    for (const auto& expr : node->rhs) {
        new_rhs.push_back(visit(expr.get()));
    }

    // Simple CSE for single assignments (e.g., a := b + c)
    if (new_lhs.size() == 1 && new_rhs.size() == 1) {
        std::string exprStr = expressionToString(new_rhs[0].get());
        auto it = availableExpressions.find(exprStr);

        if (it != availableExpressions.end()) {
            // Common subexpression found, replace RHS with temp variable
            std::vector<ExprPtr> rhs_vec;
            rhs_vec.push_back(std::make_unique<VariableAccess>(it->second));
            return std::make_unique<Assignment>(std::move(new_lhs), std::move(rhs_vec));
        } else {
            // No common subexpression, add to available expressions if it's an expression
            // that can be reused (e.g., not a function call with side effects)
            if (dynamic_cast<BinaryOp*>(new_rhs[0].get()) || dynamic_cast<UnaryOp*>(new_rhs[0].get())) {
                std::string temp_name = generateTempVarName();
                availableExpressions[exprStr] = temp_name;
                // Create a new assignment for the temp variable
                std::vector<ExprPtr> temp_lhs;
                temp_lhs.push_back(std::make_unique<VariableAccess>(temp_name));
                std::vector<std::unique_ptr<Node>> compound_stmts;
                compound_stmts.push_back(std::make_unique<Assignment>(std::move(temp_lhs), std::move(new_rhs)));
                std::vector<ExprPtr> temp_rhs_vec;
                temp_rhs_vec.push_back(std::make_unique<VariableAccess>(temp_name));
                compound_stmts.push_back(std::make_unique<Assignment>(std::move(new_lhs), std::move(temp_rhs_vec)));
                return std::make_unique<CompoundStatement>(std::move(compound_stmts));
            }
        }
    }
    return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
}

StmtPtr CommonSubexpressionEliminationPass::visit(CompoundStatement* node) {
    std::vector<std::unique_ptr<Node>> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(static_cast<Statement*>(stmt.get()))) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr CommonSubexpressionEliminationPass::visit(RoutineCall* node) {
    auto new_call_expr = visit(node->call_expression.get());
    return std::make_unique<RoutineCall>(std::move(new_call_expr));
}

StmtPtr CommonSubexpressionEliminationPass::visit(IfStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    return std::make_unique<IfStatement>(std::move(new_cond), std::move(new_then));
}

StmtPtr CommonSubexpressionEliminationPass::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr CommonSubexpressionEliminationPass::visit(WhileStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_body = visit(node->body.get());
    return std::make_unique<WhileStatement>(std::move(new_cond), std::move(new_body));
}

StmtPtr CommonSubexpressionEliminationPass::visit(ForStatement* node) {
    auto new_from = visit(node->from_expr.get());
    auto new_to = visit(node->to_expr.get());
    auto new_by = node->by_expr ? visit(node->by_expr.get()) : nullptr;
    auto new_body = visit(node->body.get());
    return std::make_unique<ForStatement>(node->var_name, std::move(new_from), std::move(new_to), std::move(new_by), std::move(new_body));
}

StmtPtr CommonSubexpressionEliminationPass::visit(GotoStatement* node) {
    auto new_label = visit(node->label.get());
    return std::make_unique<GotoStatement>(std::move(new_label));
}

StmtPtr CommonSubexpressionEliminationPass::visit(LabeledStatement* node) {
    auto new_stmt = visit(node->statement.get());
    return std::make_unique<LabeledStatement>(node->name, std::move(new_stmt));
}

StmtPtr CommonSubexpressionEliminationPass::visit(ReturnStatement* node) {
    return std::make_unique<ReturnStatement>();
}

StmtPtr CommonSubexpressionEliminationPass::visit(FinishStatement* node) {
    return std::make_unique<FinishStatement>();
}

StmtPtr CommonSubexpressionEliminationPass::visit(ResultisStatement* node) {
    auto new_value = visit(node->value.get());
    return std::make_unique<ResultisStatement>(std::move(new_value));
}

StmtPtr CommonSubexpressionEliminationPass::visit(RepeatStatement* node) {
    auto new_body = visit(node->body.get());
    auto new_cond = node->condition ? visit(node->condition.get()) : nullptr;
    return std::make_unique<RepeatStatement>(
        std::move(new_body),
        std::move(new_cond),
        node->loopType  // Pass through the original loop type
    );
}

StmtPtr CommonSubexpressionEliminationPass::visit(SwitchonStatement* node) {
    auto new_expr = visit(node->expression.get());
    std::vector<SwitchonStatement::SwitchCase> new_cases;
    for (auto& scase : node->cases) {
        new_cases.push_back({scase.value, scase.label, visit(scase.statement.get())});
    }
    auto new_default = node->default_case ? visit(node->default_case.get()) : nullptr;
    return std::make_unique<SwitchonStatement>(std::move(new_expr), std::move(new_cases), std::move(new_default));
}

StmtPtr CommonSubexpressionEliminationPass::visit(EndcaseStatement* node) {
    return std::make_unique<EndcaseStatement>();
}

StmtPtr CommonSubexpressionEliminationPass::visit(DeclarationStatement* node) {
    // Optimize the declaration itself, then wrap it back in a DeclarationStatement
    if (DeclPtr optimized_decl = visit(node->declaration.get())) {
        return std::make_unique<DeclarationStatement>(std::move(optimized_decl));
    }
    return nullptr; // If the declaration is optimized away, return nullptr
}
