#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "Lexer.h" // For TokenType
#include "ASTVisitor.h" // Include for ASTVisitor

// --- Forward Declarations ---
class Expression;
class Statement;
class Declaration;
class Program;
class RepeatStatement;


// --- Smart Pointer Type Definitions ---
using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using DeclPtr = std::unique_ptr<Declaration>;
using ProgramPtr = std::unique_ptr<Program>;

// --- Base Node Class ---
// The base class for all nodes in the Abstract Syntax Tree.
class Node {
public:
    virtual ~Node() = default;
    virtual std::unique_ptr<Node> clone() const = 0;
    virtual void accept(ASTVisitor* visitor) = 0;
};

// --- Expression Nodes ---
class Expression : public Node {
public:
    virtual ExprPtr cloneExpr() const = 0; // Specific clone for expressions
    std::unique_ptr<Node> clone() const override { return cloneExpr(); }
};

// Represents a numeric literal (integer).
class NumberLiteral : public Expression {
public:
    NumberLiteral(int64_t val) : value(val) {}
    int64_t value;
    ExprPtr cloneExpr() const override { return std::make_unique<NumberLiteral>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a floating-point literal.
class FloatLiteral : public Expression {
public:
    FloatLiteral(double val) : value(val) {}
    double value;
    ExprPtr cloneExpr() const override { return std::make_unique<FloatLiteral>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a string literal.
class StringLiteral : public Expression {
public:
    StringLiteral(std::string val) : value(std::move(val)) {}
    std::string value;
    ExprPtr cloneExpr() const override { return std::make_unique<StringLiteral>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a character literal.
class CharLiteral : public Expression {
public:
    CharLiteral(int64_t val) : value(val) {}
    int64_t value;
    ExprPtr cloneExpr() const override { return std::make_unique<CharLiteral>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents accessing a variable by its name.
class VariableAccess : public Expression {
public:
    VariableAccess(std::string name) : name(std::move(name)) {}
    std::string name;
    ExprPtr cloneExpr() const override { return std::make_unique<VariableAccess>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a unary operation (e.g., @E, ~E, !E).
class UnaryOp : public Expression {
public:
    UnaryOp(TokenType op, ExprPtr rhs) : op(op), rhs(std::move(rhs)) {}
    TokenType op;
    ExprPtr rhs;
    ExprPtr cloneExpr() const override { return std::make_unique<UnaryOp>(op, rhs->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a binary operation (e.g., E1 + E2, E1 = E2).
class BinaryOp : public Expression {
public:
    BinaryOp(TokenType op, ExprPtr left, ExprPtr right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    TokenType op;
    ExprPtr left;
    ExprPtr right;
    ExprPtr cloneExpr() const override { return std::make_unique<BinaryOp>(op, left->cloneExpr(), right->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a function call.
class FunctionCall : public Expression {
public:
    FunctionCall(ExprPtr func, std::vector<ExprPtr> args)
        : function(std::move(func)), arguments(std::move(args)) {}
    ExprPtr function;
    std::vector<ExprPtr> arguments;
    ExprPtr cloneExpr() const override {
        std::vector<ExprPtr> new_args;
        for (const auto& arg : arguments) {
            new_args.push_back(arg->cloneExpr());
        }
        return std::make_unique<FunctionCall>(function->cloneExpr(), std::move(new_args));
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a conditional expression (E1 -> E2, E3).
class ConditionalExpression : public Expression {
public:
    ConditionalExpression(ExprPtr cond, ExprPtr true_expr, ExprPtr false_expr)
        : condition(std::move(cond)),
          trueExpr(std::move(true_expr)),
          falseExpr(std::move(false_expr)) {}
    ExprPtr condition;
    ExprPtr trueExpr;
    ExprPtr falseExpr;
    ExprPtr cloneExpr() const override { return std::make_unique<ConditionalExpression>(condition->cloneExpr(), trueExpr->cloneExpr(), falseExpr->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// --- Statement Nodes ---
class Statement : public Node {
public:
    virtual StmtPtr cloneStmt() const = 0; // Specific clone for statements
    std::unique_ptr<Node> clone() const override { return cloneStmt(); }
};

class RepeatStatement : public Statement {
public:
    enum class LoopType { repeat, repeatwhile, repeatuntil };

    std::unique_ptr<Statement> body;
    std::unique_ptr<Expression> condition;
    LoopType loopType;

    RepeatStatement(StmtPtr body)
        : body(std::move(body)) {}


    RepeatStatement(std::unique_ptr<Statement> body, std::unique_ptr<Expression> condition, LoopType loopType)
        : body(std::move(body)), condition(std::move(condition)), loopType(loopType) {}

    void accept(ASTVisitor* visitor) override;

    // This override was incorrect and has been removed.
    // The base class `Statement::clone()` is sufficient.

    // Correctly override the `cloneStmt` function from the `Statement` base class.
    std::unique_ptr<Statement> cloneStmt() const override;
};


// --- New Statement Node Definitions ---
// Represents a SWITCHON statement.
class SwitchonStatement : public Statement {
public:
    struct SwitchCase {
        int value;
        std::string label; // Label for the case's code block
        StmtPtr statement;
    };
    SwitchonStatement(ExprPtr expr, std::vector<SwitchCase> cases, StmtPtr default_case)
        : expression(std::move(expr)), cases(std::move(cases)), default_case(std::move(default_case)) {}
    ExprPtr expression;
    std::vector<SwitchCase> cases;
    StmtPtr default_case; // Can be nullptr
    StmtPtr cloneStmt() const override {
        std::vector<SwitchCase> new_cases;
        for (const auto& scase : cases) {
            new_cases.push_back({scase.value, scase.label, scase.statement ? scase.statement->cloneStmt() : nullptr});
        }
        return std::make_unique<SwitchonStatement>(expression->cloneExpr(), std::move(new_cases), default_case ? default_case->cloneStmt() : nullptr);
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a BREAK statement.
class BreakStatement : public Statement {
public:
    StmtPtr cloneStmt() const override { return std::make_unique<BreakStatement>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a LOOP statement.
class LoopStatement : public Statement {
public:
    StmtPtr cloneStmt() const override { return std::make_unique<LoopStatement>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// Represents an ENDCASE statement.
class EndcaseStatement : public Statement {
public:
    StmtPtr cloneStmt() const override { return std::make_unique<EndcaseStatement>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// --- New Expression Node Definitions ---
// Represents a table constructor.
class TableConstructor : public Expression {
public:
    ExprPtr cloneExpr() const override { return std::make_unique<TableConstructor>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a vector constructor.
class VectorConstructor : public Expression {
public:
    explicit VectorConstructor(ExprPtr size) : size(std::move(size)) {}
    ExprPtr size;
    ExprPtr cloneExpr() const override { return std::make_unique<VectorConstructor>(size->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a VALOF block.
class Valof : public Expression {
public:
    explicit Valof(StmtPtr body) : body(std::move(body)) {}
    StmtPtr body;
    ExprPtr cloneExpr() const override { return std::make_unique<Valof>(body->cloneStmt()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a dereference expression (e.g., !P).
class DereferenceExpr : public Expression {
public:
    explicit DereferenceExpr(ExprPtr ptr) : pointer(std::move(ptr)) {}
    ExprPtr pointer;
    ExprPtr cloneExpr() const override { return std::make_unique<DereferenceExpr>(pointer->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a vector access expression (e.g., V!I).
class VectorAccess : public Expression {
public:
    VectorAccess(ExprPtr vec, ExprPtr idx) : vector(std::move(vec)), index(std::move(idx)) {}
    ExprPtr vector;
    ExprPtr index;
    ExprPtr cloneExpr() const override { return std::make_unique<VectorAccess>(vector->cloneExpr(), index->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a character access expression (e.g., S%I).
class CharacterAccess : public Expression {
public:
    CharacterAccess(ExprPtr str, ExprPtr idx) : string(std::move(str)), index(std::move(idx)) {}
    ExprPtr string;
    ExprPtr index;
    ExprPtr cloneExpr() const override { return std::make_unique<CharacterAccess>(string->cloneExpr(), index->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents an assignment (e.g., V!i := E).
class Assignment : public Statement {
public:
    Assignment(std::vector<ExprPtr> lhs, std::vector<ExprPtr> rhs)
        : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    std::vector<ExprPtr> lhs;
    std::vector<ExprPtr> rhs;
    StmtPtr cloneStmt() const override {
        std::vector<ExprPtr> new_lhs;
        for (const auto& expr : lhs) {
            new_lhs.push_back(expr->cloneExpr());
        }
        std::vector<ExprPtr> new_rhs;
        for (const auto& expr : rhs) {
            new_rhs.push_back(expr->cloneExpr());
        }
        return std::make_unique<Assignment>(std::move(new_lhs), std::move(new_rhs));
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a call to a routine (which doesn't return a value).
class RoutineCall : public Statement {
public:
    explicit RoutineCall(ExprPtr call_expr) : call_expression(std::move(call_expr)) {}
    ExprPtr call_expression;
    StmtPtr cloneStmt() const override { return std::make_unique<RoutineCall>(call_expression->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a block of statements, e.g., $( C1; C2 $)
class CompoundStatement : public Statement {
public:
    explicit CompoundStatement(std::vector<std::unique_ptr<Node>> stmts) : statements(std::move(stmts)) {}
    std::vector<std::unique_ptr<Node>> statements;
    StmtPtr cloneStmt() const override {
        std::vector<std::unique_ptr<Node>> new_stmts;
        for (const auto& stmt : statements) {
            new_stmts.push_back(stmt->clone());
        }
        return std::make_unique<CompoundStatement>(std::move(new_stmts));
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// Represents an IF/UNLESS command.
class IfStatement : public Statement {
public:
    IfStatement(ExprPtr cond, StmtPtr then_stmt)
        : condition(std::move(cond)), then_statement(std::move(then_stmt)) {}
    ExprPtr condition;
    StmtPtr then_statement;
    StmtPtr cloneStmt() const override { return std::make_unique<IfStatement>(condition->cloneExpr(), then_statement->cloneStmt()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a TEST command.
class TestStatement : public Statement {
public:
    TestStatement(ExprPtr cond, StmtPtr then_stmt, StmtPtr else_stmt)
        : condition(std::move(cond)),
          then_statement(std::move(then_stmt)),
          else_statement(std::move(else_stmt)) {}
    ExprPtr condition;
    StmtPtr then_statement;
    StmtPtr else_statement;
    StmtPtr cloneStmt() const override { return std::make_unique<TestStatement>(condition->cloneExpr(), then_statement->cloneStmt(), else_statement ? else_statement->cloneStmt() : nullptr); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a WHILE/UNTIL loop.
class WhileStatement : public Statement {
public:
    WhileStatement(ExprPtr cond, StmtPtr body)
        : condition(std::move(cond)), body(std::move(body)) {}
    ExprPtr condition;
    StmtPtr body;
    StmtPtr cloneStmt() const override { return std::make_unique<WhileStatement>(condition->cloneExpr(), body->cloneStmt()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a FOR loop.
class ForStatement : public Statement {
public:
    ForStatement(std::string var, ExprPtr from, ExprPtr to, ExprPtr by, StmtPtr body)
        : var_name(std::move(var)), from_expr(std::move(from)),
          to_expr(std::move(to)), by_expr(std::move(by)), body(std::move(body)) {}
    std::string var_name;
    ExprPtr from_expr;
    ExprPtr to_expr;
    ExprPtr by_expr; // Can be nullptr for default BY 1
    StmtPtr body;
    StmtPtr cloneStmt() const override { return std::make_unique<ForStatement>(var_name, from_expr->cloneExpr(), to_expr->cloneExpr(), by_expr ? by_expr->cloneExpr() : nullptr, body->cloneStmt()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a GOTO statement.
class GotoStatement : public Statement {
public:
    explicit GotoStatement(ExprPtr label_expr) : label(std::move(label_expr)) {}
    ExprPtr label;
    StmtPtr cloneStmt() const override { return std::make_unique<GotoStatement>(label->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a labeled statement.
class LabeledStatement : public Statement {
public:
    LabeledStatement(std::string name, StmtPtr stmt) : name(std::move(name)), statement(std::move(stmt)) {}
    std::string name;
    StmtPtr statement;
    StmtPtr cloneStmt() const override { return std::make_unique<LabeledStatement>(name, statement->cloneStmt()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a RETURN statement.
class ReturnStatement : public Statement {
public:
    StmtPtr cloneStmt() const override { return std::make_unique<ReturnStatement>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class DeclarationStatement : public Statement {
public:
    explicit DeclarationStatement(DeclPtr decl) : declaration(std::move(decl)) {}
    DeclPtr declaration;
    StmtPtr cloneStmt() const override;
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a FINISH statement.
class FinishStatement : public Statement {
public:
    StmtPtr cloneStmt() const override { return std::make_unique<FinishStatement>(); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a RESULTIS statement.
class ResultisStatement : public Statement {
public:
    explicit ResultisStatement(ExprPtr val) : value(std::move(val)) {}
    ExprPtr value;
    StmtPtr cloneStmt() const override { return std::make_unique<ResultisStatement>(value->cloneExpr()); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// --- Declaration Nodes ---
class Declaration : public Node {
public:
    virtual DeclPtr cloneDecl() const = 0; // Specific clone for declarations
    std::unique_ptr<Node> clone() const override { return cloneDecl(); }
};

class GetDirective : public Declaration {
public:
    explicit GetDirective(std::string file) : filename(std::move(file)) {}
    std::string filename;
    DeclPtr cloneDecl() const override { return std::make_unique<GetDirective>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a LET declaration for variables.
class LetDeclaration : public Declaration {
public:
    struct VarInit {
        std::string name;
        ExprPtr init; // Can be nullptr
    };
    explicit LetDeclaration(std::vector<VarInit> inits) : initializers(std::move(inits)) {}
    std::vector<VarInit> initializers;
    DeclPtr cloneDecl() const override {
        std::vector<VarInit> new_inits;
        for (const auto& init : initializers) {
            new_inits.push_back({init.name, init.init ? init.init->cloneExpr() : nullptr});
        }
        return std::make_unique<LetDeclaration>(std::move(new_inits));
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class GlobalDeclaration : public Declaration {
public:
    struct Global {
        std::string name;
        int size;
    };
    explicit GlobalDeclaration(std::vector<Global> globals) : globals(std::move(globals)) {}
    std::vector<Global> globals;
    DeclPtr cloneDecl() const override { return std::make_unique<GlobalDeclaration>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class ManifestDeclaration : public Declaration {
public:
    struct Manifest {
        std::string name;
        int value;
    };
    explicit ManifestDeclaration(std::vector<Manifest> manifests) : manifests(std::move(manifests)) {}
    std::vector<Manifest> manifests;
    DeclPtr cloneDecl() const override { return std::make_unique<ManifestDeclaration>(*this); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Represents a function or routine declaration.
class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(std::string name, std::vector<std::string> params, ExprPtr body_expr, StmtPtr body_stmt)
        : name(std::move(name)), params(std::move(params)),
          body_expr(std::move(body_expr)), body_stmt(std::move(body_stmt)) {}
    std::string name;
    std::vector<std::string> params;
    ExprPtr body_expr; // For LET F() = E
    StmtPtr body_stmt; // For LET R() BE C
    DeclPtr cloneDecl() const override { return std::make_unique<FunctionDeclaration>(name, params, body_expr ? body_expr->cloneExpr() : nullptr, body_stmt ? body_stmt->cloneStmt() : nullptr); }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// --- Program Node ---
// The root of the entire AST.
class Program : public Node {
public:
    explicit Program(std::vector<DeclPtr> decls) : declarations(std::move(decls)) {}
    std::vector<DeclPtr> declarations;
    std::unique_ptr<Node> clone() const override {
        std::vector<DeclPtr> new_decls;
        for (const auto& decl : declarations) {
            new_decls.push_back(decl->cloneDecl());
        }
        return std::make_unique<Program>(std::move(new_decls));
    }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

#endif // AST_H