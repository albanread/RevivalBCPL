#include "FunctionInliningPass.h"

std::string FunctionInliningPass::getName() const {
    return "Function Inlining Pass";
}

ProgramPtr FunctionInliningPass::apply(ProgramPtr program) {
    // Stage 1: Find all functions that can be inlined.
    findInlinableFunctions(program.get());

    // Stage 2: Visit the AST and perform the inlining.
    return visit(program.get());
}

ProgramPtr FunctionInliningPass::visit(Program* node) {
    std::vector<DeclPtr> new_decls;
    for (const auto& decl : node->declarations) {
        if (DeclPtr optimized_decl = visit(decl.get())) {
            new_decls.push_back(std::move(optimized_decl));
        }
    }
    return std::make_unique<Program>(std::move(new_decls));
}

DeclPtr FunctionInliningPass::visit(Declaration* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<FunctionDeclaration*>(node)) return visit(n);
    return node->cloneDecl();
}

DeclPtr FunctionInliningPass::visit(FunctionDeclaration* node) {
    auto new_body_stmt = node->body_stmt ? visit(node->body_stmt.get()) : nullptr;
    auto new_body_expr = node->body_expr ? visit(node->body_expr.get()) : nullptr;
    return std::make_unique<FunctionDeclaration>(node->name, node->params, std::move(new_body_expr), std::move(new_body_stmt));
}

ExprPtr FunctionInliningPass::visit(Expression* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<FunctionCall*>(node)) return visit(n);
    return node->cloneExpr();
}

StmtPtr FunctionInliningPass::visit(Statement* node) {
    if (!node) return nullptr;
    if (auto* n = dynamic_cast<CompoundStatement*>(node)) return visit(n);
    return node->cloneStmt();
}

void FunctionInliningPass::findInlinableFunctions(Program* program) {
    // Logic to iterate through all declarations.
    // If a FunctionDeclaration is found, apply heuristics:
    // - Is it small enough?
    // - Is it non-recursive? (Check its body for calls to itself).
    // If so, add it to the 'inlinableFunctions' map.
}

ExprPtr FunctionInliningPass::visit(FunctionCall* node) {
    // First, recursively optimize the arguments of the call.
    // ...

    auto* func_var = dynamic_cast<VariableAccess*>(node->function.get());
    if (!func_var) {
        return node->cloneExpr(); // Cannot inline indirect function calls.
    }

    auto it = inlinableFunctions.find(func_var->name);
    if (it == inlinableFunctions.end()) {
        return node->cloneExpr(); // Not an inlinable function.
    }

    const FunctionDeclaration* func_decl = it->second.declaration;

    // --- Core Inlining Logic ---
    // 1. Create LET declarations to map arguments to parameters.
    //    LET param1 = arg1, param2 = arg2, ...
    std::vector<LetDeclaration::VarInit> param_bindings;
    for (size_t i = 0; i < func_decl->params.size(); ++i) {
        param_bindings.push_back({func_decl->params[i], node->arguments[i]->cloneExpr()});
    }
    auto let_decl = std::make_unique<LetDeclaration>(std::move(param_bindings));

    // 2. Clone the function's body. Must be a deep copy.
    StmtPtr inlined_body = func_decl->body_stmt->cloneStmt();

    // 3. Create a CompoundStatement with the bindings and the body.
    std::vector<std::unique_ptr<Node>> new_block_stmts;
    new_block_stmts.push_back(std::move(let_decl));
    new_block_stmts.push_back(std::move(inlined_body));
    auto new_block = std::make_unique<CompoundStatement>(std::move(new_block_stmts));

    // 4. Wrap the entire thing in a VALOF expression to replace the original FunctionCall.
    return std::make_unique<Valof>(std::move(new_block));
}

StmtPtr FunctionInliningPass::visit(CompoundStatement* node) {
    std::vector<std::unique_ptr<Node>> new_stmts;
    for (const auto& stmt : node->statements) {
        if(auto new_stmt = visit(static_cast<Statement*>(stmt.get()))) {
             new_stmts.push_back(std::move(new_stmt));
        }
    }
    return std::make_unique<CompoundStatement>(std::move(new_stmts));
}