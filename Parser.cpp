#include "Parser.h"
#include <stdexcept>
#include <map>
#include <iostream>
#include <string>

// Map to determine the precedence of binary operators.
static const std::map<TokenType, int> PrecedenceMap = {
    {TokenType::OpLogOr, 1}, {TokenType::OpLogNeqv, 1}, {TokenType::OpLogEqv, 1},
    {TokenType::OpLogAnd, 2},
    {TokenType::OpEq, 3}, {TokenType::OpNe, 3}, {TokenType::OpLt, 3},
    {TokenType::OpGt, 3}, {TokenType::OpLe, 3}, {TokenType::OpGe, 3},
    {TokenType::OpFloatEq, 3}, {TokenType::OpFloatNe, 3}, {TokenType::OpFloatLt, 3},
    {TokenType::OpFloatGt, 3}, {TokenType::OpFloatLe, 3}, {TokenType::OpFloatGe, 3},
    {TokenType::OpLshift, 4}, {TokenType::OpRshift, 4},
    {TokenType::OpPlus, 5}, {TokenType::OpMinus, 5},
    {TokenType::OpFloatPlus, 5}, {TokenType::OpFloatMinus, 5},
    {TokenType::OpMultiply, 6}, {TokenType::OpDivide, 6}, {TokenType::OpRemainder, 6},
    {TokenType::OpFloatMultiply, 6}, {TokenType::OpFloatDivide, 6},
    {TokenType::OpBang, 7}, {TokenType::OpCharSub, 7}, {TokenType::OpFloatVecSub, 7}
};

namespace {
    int label_counter = 0;
    std::string generate_label() {
        return "case_" + std::to_string(label_counter++);
    }
}

int Parser::getTokenPrecedence() {
    if (PrecedenceMap.count(currentToken.type)) {
        return PrecedenceMap.at(currentToken.type);
    }
    return -1;
}

ProgramPtr Parser::parse(const std::string& source) {
    lexer.init(source);
    advanceTokens();
    advanceTokens();

    std::vector<DeclPtr> declarations;
    while (currentToken.type != TokenType::Eof) {
        // Top-level must be declarations in BCPL
        declarations.push_back(parseDeclaration());
    }

    return std::make_unique<Program>(std::move(declarations));
}

void Parser::advanceTokens() {
    currentToken = peekToken;
    peekToken = lexer.getNextToken();
}

void Parser::expect(TokenType type, const std::string& message) {
    if (currentToken.type != type) {
        throw std::runtime_error("Parser Error (line " + std::to_string(currentToken.line) + "): " + message);
    }
    advanceTokens();
}

// --- Declaration Parsing ---

DeclPtr Parser::parseDeclaration() {
    if (currentToken.type == TokenType::KwLet) {
        return parseLetDeclaration();
    } else if (currentToken.type == TokenType::KwGlobal) {
        return parseGlobalDeclaration();
    } else if (currentToken.type == TokenType::KwManifest) {
        return parseManifestDeclaration();
    }
    throw std::runtime_error("Parser Error: Expected top-level declaration (LET, GLOBAL, etc).");
}

DeclPtr Parser::parseLetDeclaration() {
    expect(TokenType::KwLet, "Expected 'LET'");

    std::string name = currentToken.text;
    expect(TokenType::Identifier, "Expected identifier after 'LET'");

    if (currentToken.type == TokenType::LParen) {
        return parseFunctionOrRoutineDeclaration(name);
    }

    std::vector<LetDeclaration::VarInit> initializers;
    initializers.push_back({name, nullptr});

    while (currentToken.type == TokenType::Comma) {
        advanceTokens();
        initializers.push_back({currentToken.text, nullptr});
        expect(TokenType::Identifier, "Expected identifier in declaration list.");
    }

    expect(TokenType::OpEq, "Expected '=' in LET declaration.");

    for (auto& init : initializers) {
        init.init = parseExpression();
        if (currentToken.type != TokenType::Comma) break;
        advanceTokens();
    }

    return std::make_unique<LetDeclaration>(std::move(initializers));
}

DeclPtr Parser::parseGlobalDeclaration() {
    expect(TokenType::KwGlobal, "Expected 'GLOBAL'");
    expect(TokenType::LSection, "Expected '$(' after 'GLOBAL'");
    std::vector<GlobalDeclaration::Global> globals;
    while (currentToken.type != TokenType::RSection) {
        std::string name = currentToken.text;
        expect(TokenType::Identifier, "Expected identifier in global declaration");
        expect(TokenType::Colon, "Expected ':' after identifier in global declaration");
        int size = currentToken.int_val;
        expect(TokenType::IntegerLiteral, "Expected integer literal for size in global declaration");
        globals.push_back({name, size});
        if (currentToken.type == TokenType::Semicolon) advanceTokens();
    }
    expect(TokenType::RSection, "Expected '$)' after global declarations");
    return std::make_unique<GlobalDeclaration>(std::move(globals));
}

DeclPtr Parser::parseManifestDeclaration() {
    expect(TokenType::KwManifest, "Expected 'MANIFEST'");
    expect(TokenType::LSection, "Expected '$(' after 'MANIFEST'");
    std::vector<ManifestDeclaration::Manifest> manifests;
    while (currentToken.type != TokenType::RSection) {
        std::string name = currentToken.text;
        expect(TokenType::Identifier, "Expected identifier in manifest declaration");
        expect(TokenType::OpEq, "Expected '=' after identifier in manifest declaration");
        int value = currentToken.int_val;
        expect(TokenType::IntegerLiteral, "Expected integer literal for value in manifest declaration");
        manifests.push_back({name, value});

        if (currentToken.type == TokenType::Semicolon) {
            advanceTokens();
        }
    }
    expect(TokenType::RSection, "Expected '$)' after manifest declarations");
    return std::make_unique<ManifestDeclaration>(std::move(manifests));
}

DeclPtr Parser::parseFunctionOrRoutineDeclaration(const std::string& name) {
    expect(TokenType::LParen, "Expected '(' for function declaration.");

    std::vector<std::string> params;
    if (currentToken.type != TokenType::RParen) {
        params.push_back(currentToken.text);
        expect(TokenType::Identifier, "Expected parameter name.");
        while(currentToken.type == TokenType::Comma) {
            advanceTokens();
            params.push_back(currentToken.text);
            expect(TokenType::Identifier, "Expected parameter name.");
        }
    }
    expect(TokenType::RParen, "Expected ')' after parameters.");

    ExprPtr body_expr = nullptr;
    StmtPtr body_stmt = nullptr;

    if (currentToken.type == TokenType::OpEq) {
        advanceTokens();
        body_expr = parseExpression();
    } else if (currentToken.type == TokenType::KwBe) {
        advanceTokens();
        body_stmt = parseStatement();
    } else if (currentToken.type == TokenType::KwValof) {
        advanceTokens();
        body_expr = parseValofExpression();
    } else {
        throw std::runtime_error("Parser Error: Expected '=', 'BE', or 'VALOF' in function/routine declaration.");
    }

    return std::make_unique<FunctionDeclaration>(name, std::move(params), std::move(body_expr), std::move(body_stmt));
}


// --- Statement Parsing ---

// The new main entry point for parsing any statement.
// It implements the "Parse-and-Check" strategy for postfix loops.
StmtPtr Parser::parseStatement() {
    // Step 1: Parse a simple statement first. This is the potential loop body 'C'.
    StmtPtr body = parseSimpleStatement();

    // Step 2: Check if the statement is followed by a REPEAT modifier.
    // This version checks for combined tokens like REPEATWHILE.
    switch (currentToken.type) {
        case TokenType::KwRepeat:
            // Case: C REPEAT (infinite loop)
            advanceTokens(); // Consume 'REPEAT'
            return std::make_unique<RepeatStatement>(std::move(body), nullptr, RepeatStatement::LoopType::repeat);

        case TokenType::KwRepeatWhile:
            // Case: C REPEATWHILE E
            advanceTokens(); // Consume 'REPEATWHILE'
            {
                ExprPtr condition = parseExpression();
                return std::make_unique<RepeatStatement>(std::move(body), std::move(condition), RepeatStatement::LoopType::repeatwhile);
            }

        case TokenType::KwRepeatUntil:
            // Case: C REPEATUNTIL E
            advanceTokens(); // Consume 'REPEATUNTIL'
            {
                ExprPtr condition = parseExpression();
                return std::make_unique<RepeatStatement>(std::move(body), std::move(condition), RepeatStatement::LoopType::repeatuntil);
            }

        default:
            // Not a repeat loop, so just return the simple statement we parsed.
            return body;
    }
}


// This function parses any statement that is NOT a postfix-repeat loop.
StmtPtr Parser::parseSimpleStatement() {
    switch (currentToken.type) {
        case TokenType::KwLet:
            return std::make_unique<DeclarationStatement>(parseLetDeclaration());
        case TokenType::KwIf:
        case TokenType::KwUnless:
            return parseIfStatement();
        case TokenType::KwTest:
            return parseTestStatement();
        case TokenType::KwWhile:
        case TokenType::KwUntil:
            return parseWhileStatement();
        case TokenType::KwFor:
            return parseForStatement();
        case TokenType::KwSwitchon:
            return parseSwitchonStatement();
        case TokenType::KwGoto:
            return parseGotoStatement();
        case TokenType::KwReturn:
            advanceTokens();
            return std::make_unique<ReturnStatement>();
        case TokenType::KwLoop:
            advanceTokens();
            return std::make_unique<LoopStatement>();
        case TokenType::KwBreak:
            advanceTokens();
            return std::make_unique<BreakStatement>();
        case TokenType::KwFinish:
            advanceTokens();
            return std::make_unique<FinishStatement>();
        case TokenType::KwEndcase:
            advanceTokens();
            return std::make_unique<EndcaseStatement>();
        case TokenType::KwResultis:
            return parseResultisStatement();
        case TokenType::LSection:
        case TokenType::LBrace:
            return parseCompoundStatement();
        case TokenType::Identifier: {
            if (peekToken.type == TokenType::Colon) {
                std::string label_name = currentToken.text;
                advanceTokens(); // consume identifier
                advanceTokens(); // consume ':'
                return std::make_unique<LabeledStatement>(label_name, parseStatement());
            }
            return parseExpressionStatement();
        }
        default:
            return parseExpressionStatement();
    }
}


StmtPtr Parser::parseCompoundStatement() {
    expect(currentToken.type == TokenType::LSection || currentToken.type == TokenType::LBrace ? currentToken.type : TokenType::LSection, "Expected '$(' or '{' to start a block.");

    std::vector<std::unique_ptr<Node>> statements;
    while(currentToken.type != TokenType::RSection && currentToken.type != TokenType::RBrace && currentToken.type != TokenType::Eof) {
        statements.push_back(std::move(parseStatement()));
        if(currentToken.type == TokenType::Semicolon) {
            advanceTokens();
        }
    }

    expect(currentToken.type == TokenType::RSection || currentToken.type == TokenType::RBrace ? currentToken.type : TokenType::RSection, "Expected '$)' or '}' to end a block.");
    return std::make_unique<CompoundStatement>(std::move(statements));
}

StmtPtr Parser::parseIfStatement() {
    TokenType type = currentToken.type; // IF or UNLESS
    advanceTokens();
    ExprPtr condition = parseExpression();
    expect(TokenType::KwThen, "Expected 'THEN' after condition.");
    StmtPtr then_stmt = parseStatement();

    if (type == TokenType::KwUnless) {
        condition = std::make_unique<UnaryOp>(TokenType::OpLogNot, std::move(condition));
    }

    return std::make_unique<IfStatement>(std::move(condition), std::move(then_stmt));
}

StmtPtr Parser::parseWhileStatement() {
    TokenType type = currentToken.type; // WHILE or UNTIL
    advanceTokens();
    ExprPtr condition = parseExpression();
    expect(TokenType::KwDo, "Expected 'DO' in loop.");
    StmtPtr body = parseStatement();

    if (type == TokenType::KwUntil) {
        condition = std::make_unique<UnaryOp>(TokenType::OpLogNot, std::move(condition));
    }

    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

StmtPtr Parser::parseForStatement() {
    expect(TokenType::KwFor, "Expected 'FOR'");
    std::string var_name = currentToken.text;
    expect(TokenType::Identifier, "Expected identifier for loop variable.");
    expect(TokenType::OpEq, "Expected '=' in FOR loop.");
    ExprPtr from_expr = parseExpression();
    expect(TokenType::KwTo, "Expected 'TO' in FOR loop.");
    ExprPtr to_expr = parseExpression();
    ExprPtr by_expr = nullptr;
    if (currentToken.type == TokenType::KwBy) {
        advanceTokens();
        by_expr = parseExpression();
    }
    expect(TokenType::KwDo, "Expected 'DO' in FOR loop.");
    StmtPtr body = parseStatement();

    return std::make_unique<ForStatement>(var_name, std::move(from_expr), std::move(to_expr), std::move(by_expr), std::move(body));
}

StmtPtr Parser::parseTestStatement() {
    expect(TokenType::KwTest, "Expected 'TEST'");
    ExprPtr condition = parseExpression();
    expect(TokenType::KwThen, "Expected 'THEN' after condition.");
    StmtPtr then_stmt = parseStatement();
    StmtPtr else_stmt = nullptr;
    if (currentToken.type == TokenType::KwOr) {
        advanceTokens();
        else_stmt = parseStatement();
    }
    return std::make_unique<TestStatement>(std::move(condition), std::move(then_stmt), std::move(else_stmt));
}

StmtPtr Parser::parseGotoStatement() {
    expect(TokenType::KwGoto, "Expected 'GOTO'");
    return std::make_unique<GotoStatement>(parseExpression());
}

StmtPtr Parser::parseSwitchonStatement() {
    expect(TokenType::KwSwitchon, "Expected 'SWITCHON'");
    ExprPtr expr = parseExpression();
    expect(TokenType::KwInto, "Expected 'INTO'");

    expect(TokenType::LSection, "Expected '$(' after 'INTO'");

    std::vector<SwitchonStatement::SwitchCase> cases;
    StmtPtr default_case = nullptr;

    while (currentToken.type != TokenType::RSection && currentToken.type != TokenType::Eof) {
        if (currentToken.type == TokenType::KwCase) {
            advanceTokens();
            if (currentToken.type != TokenType::IntegerLiteral) {
                throw std::runtime_error("Parser Error: Expected integer literal for case value.");
            }
            int case_val = currentToken.int_val;
            advanceTokens();
            expect(TokenType::Colon, "Expected ':' after case value.");
            StmtPtr case_stmt = parseStatement();
            cases.push_back({case_val, generate_label(), std::move(case_stmt)});
        } else if (currentToken.type == TokenType::KwDefault) {
            advanceTokens();
            expect(TokenType::Colon, "Expected ':' after 'DEFAULT'.");
            default_case = parseStatement();
        } else {
            throw std::runtime_error("Parser Error: Unexpected token in SWITCHON statement.");
        }
    }

    expect(TokenType::RSection, "Expected '$)' to end SWITCHON statement.");

    return std::make_unique<SwitchonStatement>(std::move(expr), std::move(cases), std::move(default_case));
}

StmtPtr Parser::parseResultisStatement() {
    expect(TokenType::KwResultis, "Expected 'RESULTIS'");
    return std::make_unique<ResultisStatement>(parseExpression());
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();

    if (auto* call = dynamic_cast<FunctionCall*>(expr.get())) {
        // If it's a function call and not part of an assignment, it's a routine call.
        if (currentToken.type != TokenType::OpAssign) {
             return std::make_unique<RoutineCall>(std::move(expr));
        }
    }

    // Must be an assignment
    if(currentToken.type == TokenType::OpAssign || currentToken.type == TokenType::Comma) {
        std::vector<ExprPtr> lhs_list;
        lhs_list.push_back(std::move(expr));

        while(currentToken.type == TokenType::Comma) {
            advanceTokens();
            // In a multi-assignment, the LHS can only be simple names/accesses
            lhs_list.push_back(parsePrimaryExpression());
        }

        expect(TokenType::OpAssign, "Expected ':=' for assignment.");

        std::vector<ExprPtr> rhs_list;
        rhs_list.push_back(parseExpression());
        while(currentToken.type == TokenType::Comma) {
            advanceTokens();
            rhs_list.push_back(parseExpression());
        }

        return std::make_unique<Assignment>(std::move(lhs_list), std::move(rhs_list));
    }

    throw std::runtime_error("Parser Error: This expression does not result in a valid statement.");
}

// --- Expression Parsing (Precedence Climbing) ---

ExprPtr Parser::parseExpression(int precedence) {
    ExprPtr lhs = parsePrimaryExpression();

    while (true) {
        // Handle conditional expression separately as it's right-associative and has special syntax
        if (currentToken.type == TokenType::OpConditional) {
            advanceTokens();
            ExprPtr true_expr = parseExpression();
            expect(TokenType::Comma, "Expected ',' in conditional expression");
            ExprPtr false_expr = parseExpression();
            lhs = std::make_unique<ConditionalExpression>(std::move(lhs), std::move(true_expr), std::move(false_expr));
            continue;
        }

        int prec = getTokenPrecedence();
        if (prec < precedence) {
            break;
        }

        TokenType op = currentToken.type;
        advanceTokens();

        if (op == TokenType::OpBang) { // Vector access V!E
            ExprPtr rhs = parseExpression(prec + 1);
            lhs = std::make_unique<VectorAccess>(std::move(lhs), std::move(rhs));
        } else if (op == TokenType::OpCharSub) { // Character access S%E
            ExprPtr rhs = parseExpression(prec + 1);
            lhs = std::make_unique<CharacterAccess>(std::move(lhs), std::move(rhs));
        } else { // Other binary operators
            ExprPtr rhs = parseExpression(prec + 1);
            lhs = std::make_unique<BinaryOp>(op, std::move(lhs), std::move(rhs));
        }
    }
    return lhs;
}

ExprPtr Parser::parsePrimaryExpression() {
    ExprPtr expr;
    if (currentToken.type == TokenType::OpAt || currentToken.type == TokenType::OpLogNot || currentToken.type == TokenType::OpMinus) {
        expr = parseUnary();
    } else {
        switch (currentToken.type) {
            case TokenType::Identifier:
                expr = parseIdentifierExpression();
                break;
            case TokenType::IntegerLiteral:
                expr = std::make_unique<NumberLiteral>(currentToken.int_val);
                advanceTokens();
                break;
            case TokenType::FloatLiteral:
                expr = std::make_unique<FloatLiteral>(currentToken.float_val);
                advanceTokens();
                break;
            case TokenType::StringLiteral:
                expr = std::make_unique<StringLiteral>(currentToken.text);
                advanceTokens();
                break;
            case TokenType::CharLiteral:
                 expr = std::make_unique<CharLiteral>(currentToken.int_val);
                 advanceTokens();
                 break;
            case TokenType::LParen:
                expr = parseParenExpression();
                break;
            case TokenType::KwValof:
                expr = parseValofExpression();
                break;
            case TokenType::KwVec:
                expr = parseVectorConstructor();
                break;
            case TokenType::KwTrue:
                 advanceTokens();
                 expr = std::make_unique<NumberLiteral>(-1);
                 break;
            case TokenType::KwFalse:
                 advanceTokens();
                 expr = std::make_unique<NumberLiteral>(0);
                 break;
            default:
                throw std::runtime_error("Parser Error (line " + std::to_string(currentToken.line) + "): Unexpected token in expression: " + currentToken.text);
        }
    }

    // After parsing a primary expression, check if it's a function call.
    // This handles cases like (f(x))(y)
    while (currentToken.type == TokenType::LParen) {
        expr = parseFunctionCall(std::move(expr));
    }

    return expr;
}

ExprPtr Parser::parseUnary() {
    TokenType op = currentToken.type;
    advanceTokens();
    // Use a high precedence (e.g., 7) for the operand of a unary operator.
    ExprPtr rhs = parseExpression(7);
    return std::make_unique<UnaryOp>(op, std::move(rhs));
}

ExprPtr Parser::parseIdentifierExpression() {
    std::string name = currentToken.text;
    advanceTokens();
    return std::make_unique<VariableAccess>(name);
}

ExprPtr Parser::parseParenExpression() {
    expect(TokenType::LParen, "Expected '('.");
    ExprPtr expr = parseExpression();
    expect(TokenType::RParen, "Expected ')'.");
    return expr;
}

ExprPtr Parser::parseFunctionCall(ExprPtr function_expr) {
    expect(TokenType::LParen, "Expected '(' for function call.");
    std::vector<ExprPtr> args;
    if (currentToken.type != TokenType::RParen) {
        while (true) {
            args.push_back(parseExpression());
            if (currentToken.type == TokenType::RParen) break;
            expect(TokenType::Comma, "Expected ',' or ')' in argument list.");
        }
    }
    expect(TokenType::RParen, "Expected ')' after arguments.");
    return std::make_unique<FunctionCall>(std::move(function_expr), std::move(args));
}

ExprPtr Parser::parseValofExpression() {
    expect(TokenType::KwValof, "Expected 'VALOF'");
    StmtPtr body = parseStatement();
    return std::make_unique<Valof>(std::move(body));
}

ExprPtr Parser::parseVectorConstructor() {
    expect(TokenType::KwVec, "Expected 'VEC'");
    ExprPtr size = parseExpression();
    return std::make_unique<VectorConstructor>(std::move(size));
}
