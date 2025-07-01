#include "LoopOptimizer.h"
#include "Optimizer.h"
#include <set>
#include <vector>

namespace { // Anonymous namespace to keep helper classes internal to this file

// (ModifiedVariableCollector is unchanged)
class ModifiedVariableCollector {
public:
    void collect(Statement* stmt, const std::string& loop_var) {
        modifiedVariables.insert(loop_var);
        visit(stmt);
    }
    std::set<std::string> modifiedVariables;
private:
    void visit(Statement* node);
};

void ModifiedVariableCollector::visit(Statement* node) {
    if (!node) return;
    if (auto* n = dynamic_cast<Assignment*>(node)) {
        for (const auto& lhs_expr : n->lhs) {
            if (auto* var = dynamic_cast<VariableAccess*>(lhs_expr.get())) {
                modifiedVariables.insert(var->name);
            }
        }
    } else if (auto* n = dynamic_cast<CompoundStatement*>(node)) {
        for (const auto& s : n->statements) visit(s.get());
    } else if (auto* n = dynamic_cast<IfStatement*>(node)) {
        visit(n->then_statement.get());
    } else if (auto* n = dynamic_cast<TestStatement*>(node)) {
        visit(n->then_statement.get());
        visit(n->else_statement.get());
    } else if (auto* n = dynamic_cast<WhileStatement*>(node)) {
        visit(n->body.get());
    } else if (auto* n = dynamic_cast<ForStatement*>(node)) {
        modifiedVariables.insert(n->var_name);
        visit(n->body.get());
    } else if (auto* n = dynamic_cast<LabeledStatement*>(node)) {
        visit(n->statement.get());
    }
}


// --- HoistingOptimizer with corrections ---
class HoistingOptimizer {
public:
    HoistingOptimizer(Optimizer* optimizer, const std::set<std::string>& modified, const std::string& loop_var)
        : main_optimizer(optimizer), modifiedVariables(modified), loopVarName(loop_var) {}

    StmtPtr transform(Statement* stmt) { return visit(stmt); }
    std::vector<DeclPtr> getHoistedDecls() { return std::move(hoistedDeclarations); }

private:
    Optimizer* main_optimizer;
    const std::set<std::string>& modifiedVariables;
    const std::string& loopVarName;
    std::vector<DeclPtr> hoistedDeclarations;
    int tempVarCounter = 0;

    bool isInvariant(Expression* expr);
    std::string generateTempVarName();
    ExprPtr hoistIfInvariant(ExprPtr expr);

    // Expression Visitors
    ExprPtr visit(Expression* node);
    ExprPtr visit(UnaryOp* node);
    ExprPtr visit(BinaryOp* node);
    ExprPtr visit(FunctionCall* node);

    // Statement Visitors
    StmtPtr visit(Statement* node);
    StmtPtr visit(Assignment* node);
    StmtPtr visit(CompoundStatement* node);
    StmtPtr visit(RoutineCall* node);
    StmtPtr visit(IfStatement* node);
    StmtPtr visit(TestStatement* node);
    StmtPtr visit(ForStatement* node);
    StmtPtr visit(LabeledStatement* node);
    // FIX: Added missing declaration for WhileStatement visitor
    StmtPtr visit(WhileStatement* node);
};

bool HoistingOptimizer::isInvariant(Expression* expr) {
    if (!expr) return true;
    if (dynamic_cast<NumberLiteral*>(expr) || dynamic_cast<FloatLiteral*>(expr) ||
        dynamic_cast<StringLiteral*>(expr) || dynamic_cast<CharLiteral*>(expr)) {
        return true;
    }
    if (auto* var = dynamic_cast<VariableAccess*>(expr)) {
        return modifiedVariables.find(var->name) == modifiedVariables.end();
    }
    if (auto* op = dynamic_cast<UnaryOp*>(expr)) {
        return isInvariant(op->rhs.get());
    }
    if (auto* op = dynamic_cast<BinaryOp*>(expr)) {
        return isInvariant(op->left.get()) && isInvariant(op->right.get());
    }
    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        if (auto* func_var = dynamic_cast<VariableAccess*>(call->function.get())) {
            const std::string& name = func_var->name;
            if (name == "WRITES" || name == "WRITEN" || name == "NEWLINE" || name == "FINISH" || name == "READN") {
                return false;
            }
        }
        for (const auto& arg : call->arguments) {
            if (!isInvariant(arg.get())) return false;
        }
        return isInvariant(call->function.get());
    }
    return false;
}

std::string HoistingOptimizer::generateTempVarName() {
    return "_licm_temp_" + std::to_string(tempVarCounter++);
}

ExprPtr HoistingOptimizer::hoistIfInvariant(ExprPtr expr) {
    if (isInvariant(expr.get())) {
        if (dynamic_cast<NumberLiteral*>(expr.get()) ||
            dynamic_cast<VariableAccess*>(expr.get())) {
            return expr;
        }
        std::string temp_name = generateTempVarName();
        // FIX: Construct the vector and its contents correctly to avoid copying a unique_ptr.
        std::vector<LetDeclaration::VarInit> inits;
        inits.emplace_back(LetDeclaration::VarInit{temp_name, std::move(expr)});
        hoistedDeclarations.push_back(std::make_unique<LetDeclaration>(std::move(inits)));
        
        return std::make_unique<VariableAccess>(temp_name);
    }
    return expr;
}

ExprPtr HoistingOptimizer::visit(Expression* node) {
    if (!node) return nullptr;
    
    // Dispatch to a specific visitor if one exists.
    if (auto* n = dynamic_cast<BinaryOp*>(node)) return visit(n);
    if (auto* n = dynamic_cast<UnaryOp*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FunctionCall*>(node)) return visit(n);
    
    // For leaf nodes (literals, variables), just run the main optimizer's visit.
    return main_optimizer->visit(node);
}

ExprPtr HoistingOptimizer::visit(UnaryOp* node) {
    auto new_rhs = visit(node->rhs.get());
    auto current_expr = std::make_unique<UnaryOp>(node->op, std::move(new_rhs));
    return hoistIfInvariant(std::move(current_expr));
}

ExprPtr HoistingOptimizer::visit(BinaryOp* node) {
    auto new_left = visit(node->left.get());
    auto new_right = visit(node->right.get());
    auto current_expr = std::make_unique<BinaryOp>(node->op, std::move(new_left), std::move(new_right));
    return hoistIfInvariant(std::move(current_expr));
}

ExprPtr HoistingOptimizer::visit(FunctionCall* node) {
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    auto new_func = visit(node->function.get());
    auto current_expr = std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
    return hoistIfInvariant(std::move(current_expr));
}

StmtPtr HoistingOptimizer::visit(Statement* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<Assignment*>(node)) return visit(n);
    if (auto* n = dynamic_cast<CompoundStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<IfStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<TestStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<WhileStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<ForStatement*>(node)) return visit(n);
    if (auto* n = dynamic_cast<RoutineCall*>(node)) return visit(n);
    if (auto* n = dynamic_cast<LabeledStatement*>(node)) return visit(n);
    return main_optimizer->visit(node);
}

StmtPtr HoistingOptimizer::visit(Assignment* node) {
    // FIX: Correctly loop through the 'rhs' vector.
    std::vector<ExprPtr> new_rhs;
    for (const auto& expr : node->rhs) {
        new_rhs.push_back(visit(expr.get()));
    }
    
    // LHS is not optimized for hoisting, just rebuilt.
    std::vector<ExprPtr> new_lhs;
    for (const auto& expr : node->lhs) {
        new_lhs.push_back(main_optimizer->visit(expr.get()));
    }
    
    return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
}

StmtPtr HoistingOptimizer::visit(CompoundStatement* node) {
    std::vector<StmtPtr> new_stmts;
    for (const auto& stmt : node->statements) {
        new_stmts.push_back(visit(stmt.get()));
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr HoistingOptimizer::visit(RoutineCall* node) {
    auto new_call_expr = visit(node->call_expression.get());
    return std::make_unique<RoutineCall>(std::move(new_call_expr));
}

StmtPtr HoistingOptimizer::visit(IfStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    return std::make_unique<IfStatement>(std::move(new_cond), std::move(new_then));
}

StmtPtr HoistingOptimizer::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr HoistingOptimizer::visit(WhileStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_body = visit(node->body.get());
    return std::make_unique<WhileStatement>(std::move(new_cond), std::move(new_body));
}

StmtPtr HoistingOptimizer::visit(ForStatement* node) {
    return LoopOptimizer::process(node, main_optimizer);
}
    
StmtPtr HoistingOptimizer::visit(LabeledStatement* node) {
    auto new_stmt = visit(node->statement.get());
    return std::make_unique<LabeledStatement>(node->name, std::move(new_stmt));
}

} // end anonymous namespace

namespace LoopOptimizer {

StmtPtr process(ForStatement* loop, Optimizer* optimizer) {
    ExprPtr new_from = optimizer->visit(loop->from_expr.get());
    ExprPtr new_to = optimizer->visit(loop->to_expr.get());
    ExprPtr new_by = loop->by_expr ? optimizer->visit(loop->by_expr.get()) : nullptr;

    ModifiedVariableCollector collector;
    collector.collect(loop->body.get(), loop->var_name);

    HoistingOptimizer hoister(optimizer, collector.modifiedVariables, loop->var_name);
    StmtPtr new_body = hoister.transform(loop->body.get());

    auto new_loop = std::make_unique<ForStatement>(
        loop->var_name, std::move(new_from), std::move(new_to), std::move(new_by), std::move(new_body)
    );

    auto hoisted_decls = hoister.getHoistedDecls();
    if (hoisted_decls.empty()) {
        return new_loop;
    } else {
        std::vector<StmtPtr> final_statements;
        for (auto& decl : hoisted_decls) {
            final_statements.push_back(std::move(decl));
        }
        final_statements.push_back(std::move(new_loop));
        return std::make_unique<CompoundStatement>(std::move(final_statements));
    }
}

} // namespace LoopOptimizer
