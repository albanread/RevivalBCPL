#include "VariableVisitor.h"

void VariableVisitor::visit(VariableAccess* node) {
    usedVariables.insert(node->name);
}

void VariableVisitor::visit(Assignment* node) {
    // LHS defines, RHS uses
    for (const auto& lhs_expr : node->lhs) {
        if (auto varAccess = dynamic_cast<VariableAccess*>(lhs_expr.get())) {
            definedVariables.insert(varAccess->name);
        } else {
            // For complex LHS (e.g., vector access), its components are used
            lhs_expr->accept(this);
        }
    }
    for (const auto& rhs_expr : node->rhs) {
        rhs_expr->accept(this);
    }
}

void VariableVisitor::visit(LetDeclaration* node) {
    for (const auto& init : node->initializers) {
        definedVariables.insert(init.name);
        if (init.init) {
            init.init->accept(this);
        }
    }
}

void VariableVisitor::visit(ForStatement* node) {
    definedVariables.insert(node->var_name);
    node->from_expr->accept(this);
    node->to_expr->accept(this);
    if (node->by_expr) {
        node->by_expr->accept(this);
    }
    node->body->accept(this);
}

void VariableVisitor::visit(FunctionDeclaration* node) {
    // Parameters are defined variables within the function's scope
    for (const auto& param : node->params) { // Corrected from node->parameters
        definedVariables.insert(param);
    }
    if (node->body_stmt) {
        node->body_stmt->accept(this);
    }
    if (node->body_expr) {
        node->body_expr->accept(this);
    }
}

void VariableVisitor::visit(FunctionCall* node) {
    node->function->accept(this);
    for (const auto& arg : node->arguments) {
        arg->accept(this);
    }
}

void VariableVisitor::visit(RoutineCall* node) {
    node->call_expression->accept(this);
}

void VariableVisitor::visit(UnaryOp* node) {
    node->rhs->accept(this);
}

void VariableVisitor::visit(BinaryOp* node) {
    node->left->accept(this);
    node->right->accept(this);
}

void VariableVisitor::visit(ConditionalExpression* node) {
    node->condition->accept(this);
    node->trueExpr->accept(this);
    node->falseExpr->accept(this);
}

void VariableVisitor::visit(Valof* node) {
    node->body->accept(this);
}

void VariableVisitor::visit(VectorConstructor* node) {
    node->size->accept(this);
}

void VariableVisitor::visit(VectorAccess* node) {
    node->vector->accept(this);
    node->index->accept(this);
}

void VariableVisitor::visit(CharacterAccess* node) {
    node->string->accept(this);
    node->index->accept(this);
}

void VariableVisitor::visit(IfStatement* node) {
    node->condition->accept(this);
    node->then_statement->accept(this);
    // IfStatement does not have an else_statement. This is handled by TestStatement.
}

void VariableVisitor::visit(TestStatement* node) {
    node->condition->accept(this);
    node->then_statement->accept(this);
    if (node->else_statement) {
        node->else_statement->accept(this);
    }
}

void VariableVisitor::visit(WhileStatement* node) {
    node->condition->accept(this);
    node->body->accept(this);
}

void VariableVisitor::visit(ResultisStatement* node) {
    node->value->accept(this);
}

void VariableVisitor::visit(RepeatStatement* node) {
    node->body->accept(this);
    node->condition->accept(this);
}

void VariableVisitor::visit(SwitchonStatement* node) {
    node->expression->accept(this);
    for (const auto& scase : node->cases) {
        scase.statement->accept(this);
    }
    if (node->default_case) {
        node->default_case->accept(this);
    }
}

void VariableVisitor::visit(GotoStatement* node) {
    node->label->accept(this);
}