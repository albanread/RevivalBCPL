#ifndef EXPRESSION_LIVENESS_VISITOR_H
#define EXPRESSION_LIVENESS_VISITOR_H

#include "AST.h"
#include "ASTVisitor.h"
#include "VariableVisitor.h"
#include <set>
#include <map>
#include <string>

/**
 * @class ExpressionLivenessVisitor
 * @brief A visitor that computes live-in and live-out sets for expressions.
 *
 * This visitor is used to propagate liveness information from statements down
 * to their contained expressions, and to compute liveness for sub-expressions
 * based on their use/def and the liveness of their successors.
 */
class ExpressionLivenessVisitor : public ASTVisitor {
public:
    ExpressionLivenessVisitor(
        std::map<const Expression*, std::set<std::string>>& liveInExprs,
        std::map<const Expression*, std::set<std::string>>& liveOutExprs,
        std::map<const Statement*, std::set<std::string>>& liveInStmts,
        std::map<const Statement*, std::set<std::string>>& liveOutStmts,
        const std::set<std::string>& initialLiveOut
    );

    void clear();

    // Override visit methods for relevant AST nodes (expressions)
    void visit(NumberLiteral* node) override;
    void visit(FloatLiteral* node) override;
    void visit(StringLiteral* node) override;
    void visit(CharLiteral* node) override;
    void visit(VariableAccess* node) override;
    void visit(UnaryOp* node) override;
    void visit(BinaryOp* node) override;
    void visit(FunctionCall* node) override;
    void visit(ConditionalExpression* node) override;
    void visit(TableConstructor* node) override;
    void visit(VectorConstructor* node) override;
    void visit(Valof* node) override;
    void visit(DereferenceExpr* node) override;
    void visit(VectorAccess* node) override;
    void visit(CharacterAccess* node) override;

private:
    std::map<const Expression*, std::set<std::string>>& liveInExpressions;
    std::map<const Expression*, std::set<std::string>>& liveOutExpressions;
    std::map<const Statement*, std::set<std::string>>& liveInStatements;
    std::map<const Statement*, std::set<std::string>>& liveOutStatements;
    std::set<std::string> currentLiveOut; // Represents the live-out of the current node being processed

    // Helper for set operations
    std::set<std::string> setUnion(const std::set<std::string>& s1, const std::set<std::string>& s2);
    std::set<std::string> setDifference(const std::set<std::string>& s1, const std::set<std::string>& s2);
};

#endif // EXPRESSION_LIVENESS_VISITOR_H