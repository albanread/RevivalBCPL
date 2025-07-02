#ifndef VARIABLE_VISITOR_H
#define VARIABLE_VISITOR_H

#include "AST.h"
#include "ASTVisitor.h"
#include <set>
#include <string>

/**
 * @class VariableVisitor
 * @brief A visitor that traverses the AST to collect used and defined variables.
 */
class VariableVisitor : public ASTVisitor {
public:
    VariableVisitor() : usedVariables(), definedVariables() {}

    const std::set<std::string>& getUsedVariables() const { return usedVariables; }
    const std::set<std::string>& getDefinedVariables() const { return definedVariables; }

    void clear() {
        usedVariables.clear();
        definedVariables.clear();
    }

    // Override visit methods for relevant AST nodes
    void visit(VariableAccess* node) override;
    void visit(Assignment* node) override;
    void visit(LetDeclaration* node) override;
    void visit(ForStatement* node) override;
    void visit(FunctionDeclaration* node) override;
    void visit(FunctionCall* node) override;
    void visit(RoutineCall* node) override;
    void visit(UnaryOp* node) override;
    void visit(BinaryOp* node) override;
    void visit(ConditionalExpression* node) override;
    void visit(Valof* node) override;
    void visit(VectorConstructor* node) override;
    void visit(VectorAccess* node) override;
    void visit(CharacterAccess* node) override;
    void visit(IfStatement* node) override;
    void visit(TestStatement* node) override;
    void visit(WhileStatement* node) override;
    void visit(ResultisStatement* node) override;
    void visit(RepeatStatement* node) override;
    void visit(SwitchonStatement* node) override;
    void visit(GotoStatement* node) override;

private:
    std::set<std::string> usedVariables;
    std::set<std::string> definedVariables;
};

#endif // VARIABLE_VISITOR_H