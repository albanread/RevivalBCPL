#include "DeadCodeEliminationPass.h"
#include <vector>

// --- Public Methods ---
std::string DeadCodeEliminationPass::getName() const {
    return "Dead Code Elimination Pass";
}

ProgramPtr DeadCodeEliminationPass::apply(ProgramPtr program) {
    // Stage 1: Analyze the entire program to find live variables.
    liveVariables.clear();
    analyzeNode(program.get());

    // Stage 2: Transform the AST to remove dead code.
    return visit(program.get());
}

// --- Analysis Stage Implementation ---
void DeadCodeEliminationPass::analyzeNode(Node* node) {
    if (!node) return;

    // A full implementation would have a visitor here.
    // This is a simplified example:
    if (auto* var = dynamic_cast<VariableAccess*>(node)) {
        liveVariables.insert(var->name);
    }

    // Recursively analyze all children of the node
    for (const auto& child : node->getChildren()) {
        analyzeNode(child.get());
    }
}

// --- Transformation Stage Implementation ---
DeclPtr DeadCodeEliminationPass::visit(LetDeclaration* node) {
    std::vector<LetDeclaration::VarInit> liveInits;
    for (const auto& init : node->initializers) {
        // Keep the declaration if the variable is in the live set.
        if (liveVariables.count(init.name)) {
            ExprPtr optimized_init = init.init ? visit(init.init.get()) : nullptr;
            liveInits.push_back({init.name, std::move(optimized_init)});
        }
    }

    if (liveInits.empty()) {
        return nullptr; // Remove the entire LET statement if all its variables are dead.
    }
    return std::make_unique<LetDeclaration>(std::move(liveInits));
}

StmtPtr DeadCodeEliminationPass::visit(Assignment* node) {
    if (node->lhs.size() == 1) {
        if (auto* var = dynamic_cast<VariableAccess*>(node->lhs[0].get())) {
            // If the variable being assigned to is not live, remove the assignment.
            if (liveVariables.find(var->name) == liveVariables.end()) {
                // Return an empty statement to effectively remove the assignment.
                return std::make_unique<CompoundStatement>(std::vector<StmtPtr>());
            }
        }
    }
    // If the variable is live, rebuild the assignment statement.
    // (A full implementation would be here)
    return main_optimizer->visit(node); // Simplified for this example
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
