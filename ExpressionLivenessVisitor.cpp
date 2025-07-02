#include "ExpressionLivenessVisitor.h"
#include <algorithm>

ExpressionLivenessVisitor::ExpressionLivenessVisitor(
    std::map<const Expression*, std::set<std::string>>& liveInExprs,
    std::map<const Expression*, std::set<std::string>>& liveOutExprs,
    std::map<const Statement*, std::set<std::string>>& liveInStmts,
    std::map<const Statement*, std::set<std::string>>& liveOutStmts,
    const std::set<std::string>& initialLiveOut
) : liveInExpressions(liveInExprs),
    liveOutExpressions(liveOutExprs),
    liveInStatements(liveInStmts),
    liveOutStatements(liveOutStmts),
    currentLiveOut(initialLiveOut) {}

void ExpressionLivenessVisitor::clear() {
    currentLiveOut.clear();
}

std::set<std::string> ExpressionLivenessVisitor::setUnion(const std::set<std::string>& s1, const std::set<std::string>& s2) {
    std::set<std::string> result;
    std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
                   std::inserter(result, result.begin()));
    return result;
}

std::set<std::string> ExpressionLivenessVisitor::setDifference(const std::set<std::string>& s1, const std::set<std::string>& s2) {
    std::set<std::string> result;
    std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::inserter(result, result.begin()));
    return result;
}

// Generic visit for expressions that don't have sub-expressions or special liveness rules
// For these, Live-in = use U (Live-out - def)
// Expressions typically don't define variables, so def is usually empty.
#define GENERIC_EXPR_VISIT(NodeType) \
void ExpressionLivenessVisitor::visit(NodeType* node) { \
    liveOutExpressions[node] = currentLiveOut; \
    VariableVisitor varVisitor; \
    node->accept(&varVisitor); \
    std::set<std::string> use_e = varVisitor.getUsedVariables(); \
    std::set<std::string> def_e = varVisitor.getDefinedVariables(); \
    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e)); \
}

GENERIC_EXPR_VISIT(NumberLiteral)
GENERIC_EXPR_VISIT(FloatLiteral)
GENERIC_EXPR_VISIT(StringLiteral)
GENERIC_EXPR_VISIT(CharLiteral)
GENERIC_EXPR_VISIT(VariableAccess)
GENERIC_EXPR_VISIT(TableConstructor)
GENERIC_EXPR_VISIT(DereferenceExpr)

void ExpressionLivenessVisitor::visit(UnaryOp* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    // Live-in = use(e) U (Live-out(e) - def(e))
    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to RHS
    currentLiveOut = liveInExpressions[node];
    node->rhs->accept(this);
}

void ExpressionLivenessVisitor::visit(BinaryOp* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to right operand first (backward analysis)
    currentLiveOut = liveInExpressions[node];
    node->right->accept(this);

    // Propagate liveness to left operand
    currentLiveOut = liveInExpressions[node]; // Live-out of left is also live-in of binary op
    node->left->accept(this);
}

void ExpressionLivenessVisitor::visit(FunctionCall* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to arguments in reverse order
    currentLiveOut = liveInExpressions[node];
    for (auto it = node->arguments.rbegin(); it != node->arguments.rend(); ++it) {
        (*it)->accept(this);
    }
    // Propagate liveness to function expression itself
    node->function->accept(this);
}

void ExpressionLivenessVisitor::visit(ConditionalExpression* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to true and false branches, then condition
    std::set<std::string> liveOutBranches = liveInExpressions[node];
    currentLiveOut = liveOutBranches;
    node->trueExpr->accept(this);
    currentLiveOut = liveOutBranches;
    node->falseExpr->accept(this);

    // Live-out of condition is union of live-in of true and false branches
    currentLiveOut = setUnion(liveInExpressions[node->trueExpr.get()], liveInExpressions[node->falseExpr.get()]);
    node->condition->accept(this);
}

void ExpressionLivenessVisitor::visit(VectorConstructor* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to size expression
    currentLiveOut = liveInExpressions[node];
    node->size->accept(this);
}

void ExpressionLivenessVisitor::visit(Valof* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to body statement
    // The live-out of the Valof's body is the live-in of the Valof expression itself.
    liveOutStatements[node->body.get()] = liveInExpressions[node];
    // Now, we need to recursively analyze the statement. This visitor is for expressions.
    // The main LivenessAnalysisPass::apply loop handles statement analysis.
    // So, we just set the live-out for the statement and assume it will be processed.
}

void ExpressionLivenessVisitor::visit(VectorAccess* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to index then vector expression
    currentLiveOut = liveInExpressions[node];
    node->index->accept(this);
    node->vector->accept(this);
}

void ExpressionLivenessVisitor::visit(CharacterAccess* node) {
    liveOutExpressions[node] = currentLiveOut;
    VariableVisitor varVisitor;
    node->accept(&varVisitor);
    std::set<std::string> use_e = varVisitor.getUsedVariables();
    std::set<std::string> def_e = varVisitor.getDefinedVariables();

    liveInExpressions[node] = setUnion(use_e, setDifference(currentLiveOut, def_e));

    // Propagate liveness to index then string expression
    currentLiveOut = liveInExpressions[node];
    node->index->accept(this);
    node->string->accept(this);
}