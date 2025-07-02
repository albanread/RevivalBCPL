#include "ConstantFoldingPass.h"
#include <stdexcept>
#include <vector>

ConstantFoldingPass::ConstantFoldingPass(std::unordered_map<std::string, int64_t>& manifests)
    : manifests(manifests) {}

ProgramPtr ConstantFoldingPass::apply(ProgramPtr program) {
    return visit(program.get());
}

std::string ConstantFoldingPass::getName() const {
    return "Constant Folding Pass";
}

// --- Visitor Dispatchers ---

ExprPtr ConstantFoldingPass::visit(Expression* node) {
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
    throw std::runtime_error("ConstantFoldingPass: Unsupported Expression node.");
}

StmtPtr ConstantFoldingPass::visit(Statement* node) {
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

    throw std::runtime_error("ConstantFoldingPass: Unsupported Statement node.");
}

DeclPtr ConstantFoldingPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<LetDeclaration*>(node)) return visit(n);
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    if (dynamic_cast<GlobalDeclaration*>(node) || dynamic_cast<ManifestDeclaration*>(node) || dynamic_cast<GetDirective*>(node)) {
        return nullptr;
    }
    throw std::runtime_error("ConstantFoldingPass: Unsupported Declaration node.");
}

// --- Top-Level and Declarations ---

ProgramPtr ConstantFoldingPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr ConstantFoldingPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> new_inits;
    for (const auto& init : node->initializers) {
        ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;
        new_inits.push_back({init.name, std::move(optimized_init)});
    }
    return std::make_unique<LetDeclaration>(std::move(new_inits));
}

DeclPtr ConstantFoldingPass::visit(FunctionDeclaration* node) {
    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

// --- Expression Visitors ---

ExprPtr ConstantFoldingPass::visit(NumberLiteral* node) { 
    return std::make_unique<NumberLiteral>(*node); 
}

ExprPtr ConstantFoldingPass::visit(FloatLiteral* node) { 
    return std::make_unique<FloatLiteral>(*node); 
}

ExprPtr ConstantFoldingPass::visit(StringLiteral* node) { 
    return std::make_unique<StringLiteral>(*node); 
}

ExprPtr ConstantFoldingPass::visit(CharLiteral* node) { 
    return std::make_unique<CharLiteral>(*node); 
}

ExprPtr ConstantFoldingPass::visit(VariableAccess* node) {
    // Check if this is a manifest constant
    auto it = manifests.find(node->name);
    if (it != manifests.end()) {
        return std::make_unique<NumberLiteral>(it->second);
    }
    return std::make_unique<VariableAccess>(*node);
}

ExprPtr ConstantFoldingPass::visit(UnaryOp* node) {
    auto new_rhs = visit(node->rhs.get());
    return std::make_unique<UnaryOp>(node->op, std::move(new_rhs));
}

ExprPtr ConstantFoldingPass::visit(BinaryOp* node) {
    auto left = visit(node->left.get());
    auto right = visit(node->right.get());
    
    // Constant folding for two number literals
    if (auto* left_num = dynamic_cast<NumberLiteral*>(left.get())) {
        if (auto* right_num = dynamic_cast<NumberLiteral*>(right.get())) {
            int64_t l = left_num->value;
            int64_t r = right_num->value;
            switch (node->op) {
                case TokenType::OpPlus:     return std::make_unique<NumberLiteral>(l + r);
                case TokenType::OpMinus:    return std::make_unique<NumberLiteral>(l - r);
                case TokenType::OpMultiply: return std::make_unique<NumberLiteral>(l * r);
                case TokenType::OpDivide:   if (r != 0) return std::make_unique<NumberLiteral>(l / r); break;
                case TokenType::OpEq:       return std::make_unique<NumberLiteral>(l == r ? -1 : 0);
                case TokenType::OpNe:       return std::make_unique<NumberLiteral>(l != r ? -1 : 0);
                case TokenType::OpLt:       return std::make_unique<NumberLiteral>(l < r ? -1 : 0);
                case TokenType::OpLe:       return std::make_unique<NumberLiteral>(l <= r ? -1 : 0);
                case TokenType::OpGt:       return std::make_unique<NumberLiteral>(l > r ? -1 : 0);
                case TokenType::OpGe:       return std::make_unique<NumberLiteral>(l >= r ? -1 : 0);
                default: break;
            }
        }
    }
    
    // Constant folding for two float literals
    if (auto* left_float = dynamic_cast<FloatLiteral*>(left.get())) {
        if (auto* right_float = dynamic_cast<FloatLiteral*>(right.get())) {
             double l = left_float->value;
             double r = right_float->value;
             switch (node->op) {
                case TokenType::OpFloatPlus:     return std::make_unique<FloatLiteral>(l + r);
                case TokenType::OpFloatMinus:    return std::make_unique<FloatLiteral>(l - r);
                case TokenType::OpFloatMultiply: return std::make_unique<FloatLiteral>(l * r);
                case TokenType::OpFloatDivide:   if (r != 0.0) return std::make_unique<FloatLiteral>(l / r); break;
                default: break;
             }
        }
    }
    
    // Strength reduction: multiply/divide by powers of 2
    if (auto* right_num = dynamic_cast<NumberLiteral*>(right.get())) {
        if (node->op == TokenType::OpMultiply && right_num->value == 2) 
            return std::make_unique<BinaryOp>(TokenType::OpLshift, std::move(left), std::make_unique<NumberLiteral>(1));
        if (node->op == TokenType::OpDivide && right_num->value == 2) 
            return std::make_unique<BinaryOp>(TokenType::OpRshift, std::move(left), std::make_unique<NumberLiteral>(1));
    }
    
    // Algebraic simplifications with right operand
    if (auto* right_num = dynamic_cast<NumberLiteral*>(right.get())) {
        if (node->op == TokenType::OpPlus && right_num->value == 0) return left;
        if (node->op == TokenType::OpMinus && right_num->value == 0) return left;
        if (node->op == TokenType::OpMultiply && right_num->value == 1) return left;
        if (node->op == TokenType::OpDivide && right_num->value == 1) return left;
        if (node->op == TokenType::OpMultiply && right_num->value == 0) return std::make_unique<NumberLiteral>(0);
    }
    
    // Algebraic simplifications with left operand
    if (auto* left_num = dynamic_cast<NumberLiteral*>(left.get())) {
        if (node->op == TokenType::OpPlus && left_num->value == 0) return right;
        if (node->op == TokenType::OpMultiply && left_num->value == 1) return right;
    }
    
    return std::make_unique<BinaryOp>(node->op, std::move(left), std::move(right));
}

ExprPtr ConstantFoldingPass::visit(ConditionalExpression* node) {
    auto new_cond = visit(node->condition.get());
    // Constant condition optimization
    if (auto* cond_lit = dynamic_cast<NumberLiteral*>(new_cond.get())) {
        return (cond_lit->value != 0) ? visit(node->trueExpr.get()) : visit(node->falseExpr.get());
    }
    return std::make_unique<ConditionalExpression>(std::move(new_cond), visit(node->trueExpr.get()), visit(node->falseExpr.get()));
}

ExprPtr ConstantFoldingPass::visit(FunctionCall* node) {
    auto new_func = visit(node->function.get());
    std::vector<ExprPtr> new_args;
    for (const auto& arg : node->arguments) {
        new_args.push_back(visit(arg.get()));
    }
    return std::make_unique<FunctionCall>(std::move(new_func), std::move(new_args));
}

ExprPtr ConstantFoldingPass::visit(Valof* node) { 
    return std::make_unique<Valof>(visit(node->body.get())); 
}

ExprPtr ConstantFoldingPass::visit(VectorConstructor* node) { 
    return std::make_unique<VectorConstructor>(visit(node->size.get())); 
}

ExprPtr ConstantFoldingPass::visit(VectorAccess* node) { 
    return std::make_unique<VectorAccess>(visit(node->vector.get()), visit(node->index.get())); 
}

// --- Statement Visitors ---

StmtPtr ConstantFoldingPass::visit(CompoundStatement* node) {
    std::vector<std::unique_ptr<Node>> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(static_cast<Statement*>(stmt.get()))) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}

StmtPtr ConstantFoldingPass::visit(Assignment* node) {
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

StmtPtr ConstantFoldingPass::visit(IfStatement* node) {
    auto new_cond = visit(node->condition.get());
    // Constant condition optimization
    if (auto* cond_lit = dynamic_cast<NumberLiteral*>(new_cond.get())) {
        if (cond_lit->value != 0) {
            return visit(node->then_statement.get());
        } else {
            return std::make_unique<CompoundStatement>(std::vector<std::unique_ptr<Node>>());
        }
    }
    return std::make_unique<IfStatement>(std::move(new_cond), visit(node->then_statement.get()));
}

StmtPtr ConstantFoldingPass::visit(TestStatement* node) {
    auto new_cond = visit(node->condition.get());
    // Constant condition optimization
    if (auto* cond_lit = dynamic_cast<NumberLiteral*>(new_cond.get())) {
        if (cond_lit->value != 0) {
            return visit(node->then_statement.get());
        } else {
            return node->else_statement ? visit(node->else_statement.get()) : std::make_unique<CompoundStatement>(std::vector<std::unique_ptr<Node>>());
        }
    }
    auto new_then = visit(node->then_statement.get());
    auto new_else = node->else_statement ? visit(node->else_statement.get()) : nullptr;
    return std::make_unique<TestStatement>(std::move(new_cond), std::move(new_then), std::move(new_else));
}

StmtPtr ConstantFoldingPass::visit(WhileStatement* node) {
    auto new_cond = visit(node->condition.get());
    auto new_body = visit(node->body.get());
    return std::make_unique<WhileStatement>(std::move(new_cond), std::move(new_body));
}

StmtPtr ConstantFoldingPass::visit(RepeatStatement* node) {
    return std::make_unique<RepeatStatement>(visit(node->body.get()), visit(node->condition.get()));
}

StmtPtr ConstantFoldingPass::visit(ForStatement* node) {
    auto new_from = visit(node->from_expr.get());
    auto new_to = visit(node->to_expr.get());
    auto new_by = node->by_expr ? visit(node->by_expr.get()) : nullptr;
    auto new_body = visit(node->body.get());
    return std::make_unique<ForStatement>(node->var_name, std::move(new_from), std::move(new_to), std::move(new_by), std::move(new_body));
}

StmtPtr ConstantFoldingPass::visit(RoutineCall* node) {
    return std::make_unique<RoutineCall>(visit(node->call_expression.get()));
}

StmtPtr ConstantFoldingPass::visit(LabeledStatement* node) {
    return std::make_unique<LabeledStatement>(node->name, visit(node->statement.get()));
}

StmtPtr ConstantFoldingPass::visit(GotoStatement* node) {
    return std::make_unique<GotoStatement>(visit(node->label.get()));
}

StmtPtr ConstantFoldingPass::visit(ResultisStatement* node) {
    return std::make_unique<ResultisStatement>(visit(node->value.get()));
}

StmtPtr ConstantFoldingPass::visit(ReturnStatement* node) {
    return std::make_unique<ReturnStatement>();
}

StmtPtr ConstantFoldingPass::visit(FinishStatement* node) {
    return std::make_unique<FinishStatement>();
}

StmtPtr ConstantFoldingPass::visit(SwitchonStatement* node) {
    auto new_expr = visit(node->expression.get());
    std::vector<SwitchonStatement::SwitchCase> new_cases;
    for (auto& scase : node->cases) {
        new_cases.push_back({scase.value, scase.label, visit(scase.statement.get())});
    }
    auto new_default = node->default_case ? visit(node->default_case.get()) : nullptr;
    return std::make_unique<SwitchonStatement>(std::move(new_expr), std::move(new_cases), std::move(new_default));
}

StmtPtr ConstantFoldingPass::visit(EndcaseStatement* node) {
    return std::make_unique<EndcaseStatement>(); // No children to optimize
}

StmtPtr ConstantFoldingPass::visit(DeclarationStatement* node) {
    // Optimize the declaration itself, then wrap it back in a DeclarationStatement
    if (DeclPtr optimized_decl = visit(node->declaration.get())) {
        return std::make_unique<DeclarationStatement>(std::move(optimized_decl));
    }
    return nullptr; // If the declaration is optimized away, return nullptr
}