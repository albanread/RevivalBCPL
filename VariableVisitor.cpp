#include "VariableVisitor.h"
#include <iostream>

void VariableVisitor::visit(VariableAccess* node) {
    // Only mark as used if it's a variable access, not a routine name.
    // This distinction might need more context (e.g., symbol table lookup).
    // For now, assume all VariableAccess are actual variable uses.
    usedVariables.insert(node->name);
    std::cout << "VariableVisitor: Used variable: " << node->name << "\n";
}

void VariableVisitor::visit(Assignment* node) {
    // LHS defines, RHS uses
    for (const auto& lhs_expr : node->lhs) {
        if (auto varAccess = dynamic_cast<VariableAccess*>(lhs_expr.get())) {
            definedVariables.insert(varAccess->name);
            std::cout << "VariableVisitor: Defined variable (Assignment): " << varAccess->name << "\n";
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
        std::cout << "VariableVisitor: Defined variable (LetDeclaration): " << init.name << "\n";
        if (init.init) {
            init.init->accept(this);
        }
    }
}

void VariableVisitor::visit(ForStatement* node) {
    definedVariables.insert(node->var_name);
    std::cout << "VariableVisitor: Defined variable (ForStatement): " << node->var_name << "\n";
    node->from_expr->accept(this);
    node->to_expr->accept(this);
    if (node->by_expr) {
        node->by_expr->accept(this);
    }
    node->body->accept(this);
}

void VariableVisitor::visit(FunctionDeclaration* node) {
    // Parameters are defined variables within the function's scope
    for (const auto& param : node->params) {
        definedVariables.insert(param);
        std::cout << "VariableVisitor: Defined variable (FunctionDeclaration param): " << param << "\n";
    }
    if (node->body_stmt) {
        node->body_stmt->accept(this);
    }
    if (node->body_expr) {
        node->body_expr->accept(this);
    }
}

void VariableVisitor::visit(FunctionCall* node) {
    // The function itself is used (its name)
    node->function->accept(this);
    // Arguments are used
    for (const auto& arg : node->arguments) {
        arg->accept(this);
    }
}

void VariableVisitor::visit(RoutineCall* node) {
    // The routine name itself is not a variable, so don't add to usedVariables.
    // However, its arguments are used.
    if (auto funcCall = dynamic_cast<FunctionCall*>(node->call_expression.get())) {
        // Visit arguments of the function call within the routine call
        for (const auto& arg : funcCall->arguments) {
            arg->accept(this);
        }
    }
    // Removed: else if (auto varAccess = dynamic_cast<VariableAccess*>(node->call_expression.get())) {
    //    usedVariables.insert(varAccess->name);
    //    std::cout << "VariableVisitor: Used routine name: " << varAccess->name << "\n";
    // }
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

void VariableVisitor::visit(DeclarationStatement* node) {
    // Visit the declaration within the statement to find defined variables
    node->declaration->accept(this);
}
