#include "AST.h"
#include "ASTVisitor.h"

// --- RepeatStatement Method Definitions ---

// Provide the definition for the accept method.
void RepeatStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

// Provide the definition for the cloning method.
std::unique_ptr<Statement> RepeatStatement::cloneStmt() const {
    // Create a deep copy by cloning the body and condition.
    auto cloned_body = body ? body->cloneStmt() : nullptr;
    auto cloned_condition = condition ? condition->cloneExpr() : nullptr;
    return std::make_unique<RepeatStatement>(std::move(cloned_body), std::move(cloned_condition), loopType);
}

// Provide the definition for DeclarationStatement's cloneStmt as well, as it is also missing.
std::unique_ptr<Statement> DeclarationStatement::cloneStmt() const {
    return std::make_unique<DeclarationStatement>(declaration ? declaration->cloneDecl() : nullptr);
}