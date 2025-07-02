#include "CommonSubexpressionEliminationPass.h"

std::string CommonSubexpressionEliminationPass::getName() const {
    return "Common Subexpression Elimination Pass";
}

// A helper to create a canonical string for an expression
std::string CommonSubexpressionEliminationPass::expressionToString(Expression* expr) {
    // This would need a robust implementation to serialize an expression.
    // For example, a BinaryOp would become "(op left right)".
    // e.g., for "a + b", it might return "(+ a b)"
    if (auto* b = dynamic_cast<BinaryOp*>(expr)) {
        // ...
    }
    return ""; // Placeholder
}

// Conceptual visitor for a statement that contains expressions
StmtPtr CommonSubexpressionEliminationPass::visit(Assignment* node) {
    // This is where the core logic would live. For each subexpression in the
    // assignment's RHS, we would check if it's in `availableExpressions`.

    // If an expression `a + b` is found:
    std::string exprStr = expressionToString(node->rhs[0].get()); // e.g. "(+ a b)"
    auto it = availableExpressions.find(exprStr);

    if (it != availableExpressions.end()) {
        // FOUND: Replace `a + b` with a variable access to `it->second` (the temp var)
        node->rhs[0] = std::make_unique<VariableAccess>(it->second);
    } else {
        // NOT FOUND: Create a new temporary variable to store the result.
        std::string tempName = generateTempVarName();
        // 1. Create a LET declaration: LET temp = a + b
        // 2. Replace `a + b` in the original statement with `temp`
        // 3. Add the expression to our map: availableExpressions[exprStr] = tempName;
    }
    return node;
}
