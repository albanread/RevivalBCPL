#include "AST.h"
#include "Lexer.h"

StmtPtr DeclarationStatement::cloneStmt() const {
    return std::make_unique<DeclarationStatement>(declaration->cloneDecl());
}