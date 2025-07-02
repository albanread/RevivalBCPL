#ifndef AST_VISITOR_H
#define AST_VISITOR_H

// Forward declarations for all AST node types
class NumberLiteral;
class FloatLiteral;
class StringLiteral;
class CharLiteral;
class VariableAccess;
class UnaryOp;
class BinaryOp;
class FunctionCall;
class ConditionalExpression;
class SwitchonStatement;
class BreakStatement;
class LoopStatement;
class RepeatStatement;
class EndcaseStatement;
class TableConstructor;
class VectorConstructor;
class Valof;
class DereferenceExpr;
class VectorAccess;
class CharacterAccess;
class Assignment;
class RoutineCall;
class CompoundStatement;
class IfStatement;
class TestStatement;
class WhileStatement;
class ForStatement;
class GotoStatement;
class LabeledStatement;
class ReturnStatement;
class DeclarationStatement;
class FinishStatement;
class ResultisStatement;
class GetDirective;
class LetDeclaration;
class GlobalDeclaration;
class ManifestDeclaration;
class FunctionDeclaration;
class Program;

/**
 * @class ASTVisitor
 * @brief Abstract base class for AST visitors.
 *
 * This class defines the interface for visiting different types of AST nodes.
 * Concrete visitors (like VariableVisitor) will implement these methods
 * to perform specific operations during AST traversal.
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Visit methods for expressions
    virtual void visit(NumberLiteral* node) {}
    virtual void visit(FloatLiteral* node) {}
    virtual void visit(StringLiteral* node) {}
    virtual void visit(CharLiteral* node) {}
    virtual void visit(VariableAccess* node) {}
    virtual void visit(UnaryOp* node) {}
    virtual void visit(BinaryOp* node) {}
    virtual void visit(FunctionCall* node) {}
    virtual void visit(ConditionalExpression* node) {}
    virtual void visit(TableConstructor* node) {}
    virtual void visit(VectorConstructor* node) {}
    virtual void visit(Valof* node) {}
    virtual void visit(DereferenceExpr* node) {}
    virtual void visit(VectorAccess* node) {}
    virtual void visit(CharacterAccess* node) {}

    // Visit methods for statements
    virtual void visit(SwitchonStatement* node) {}
    virtual void visit(BreakStatement* node) {}
    virtual void visit(LoopStatement* node) {}
    virtual void visit(RepeatStatement* node) {}
    virtual void visit(EndcaseStatement* node) {}
    virtual void visit(Assignment* node) {}
    virtual void visit(RoutineCall* node) {}
    virtual void visit(CompoundStatement* node) {}
    virtual void visit(IfStatement* node) {}
    virtual void visit(TestStatement* node) {}
    virtual void visit(WhileStatement* node) {}
    virtual void visit(ForStatement* node) {}
    virtual void visit(GotoStatement* node) {}
    virtual void visit(LabeledStatement* node) {}
    virtual void visit(ReturnStatement* node) {}
    virtual void visit(DeclarationStatement* node) {}
    virtual void visit(FinishStatement* node) {}
    virtual void visit(ResultisStatement* node) {}

    // Visit methods for declarations
    virtual void visit(GetDirective* node) {}
    virtual void visit(LetDeclaration* node) {}
    virtual void visit(GlobalDeclaration* node) {}
    virtual void visit(ManifestDeclaration* node) {}
    virtual void visit(FunctionDeclaration* node) {}

    // Visit method for program root
    virtual void visit(Program* node) {}
};

#endif // AST_VISITOR_H
