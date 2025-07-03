### **Overview of the "Parse-and-Check" Strategy**

The core idea is to change the responsibility of the parseStatement function. Instead of trying to predict what kind of statement is coming, it will:

1. **Parse a "simple" statement:** This is any command *without* a postfix modifier (e.g., an IF statement, an assignment, a FOR loop, etc.).  
2. **Check for a postfix modifier:** After parsing the simple statement, it will look at the next token to see if it's REPEAT, REPEATWHILE, or REPEATUNTIL.  
3. **Wrap the AST node:** If a modifier is found, the parser will "wrap" the simple statement's AST node inside a new RepeatStatement AST node. Otherwise, it will return the simple statement's node as is.

### **Step 1: Update AST.h for a More Expressive RepeatStatement**

First, your RepeatStatement AST node should be able to represent all three forms of the loop (C REPEAT, C REPEATWHILE E, C REPEATUNTIL E). The best way to do this is with an enum.  
**File: AST.h**  
// Represents a REPEAT loop.  
class RepeatStatement : public Statement {  
public:  
    // Enum to distinguish between the different kinds of REPEAT loops  
    enum class LoopType {  
        INFINITE,      // C REPEAT  
        WHILE,         // C REPEATWHILE E  
        UNTIL          // C REPEATUNTIL E  
    };

    RepeatStatement(StmtPtr body, ExprPtr cond, LoopType type)  
        : body(std::move(body)), condition(std::move(cond)), type(type) {}

    StmtPtr body;  
    ExprPtr condition; // Can be nullptr for INFINITE type  
    LoopType type;

    StmtPtr cloneStmt() const override {  
        return std::make\_unique\<RepeatStatement\>(  
            body-\>cloneStmt(),  
            condition ? condition-\>cloneExpr() : nullptr,  
            type  
        );  
    }  
    void accept(ASTVisitor\* visitor) override { visitor-\>visit(this); }  
};

**Reasoning:** This change makes your AST explicit. Anyone consuming the tree (like a code generator) can simply check the type field to know exactly how to handle the loop's logic, rather than inferring it from whether the condition is null or not.

### **Step 2: Update the Parser Declaration (Parser.h)**

We will restructure the parsing functions. The incorrect parseRepeatStatement will be removed, and a new helper function, parseSimpleStatement, will be introduced.  
**File: Parser.h**  
\--- a/Parser.h  
\+++ b/Parser.h  
@@ \-37,12 \+37,12 @@  
     // Statements  
     StmtPtr parseStatement();  
     StmtPtr parseCompoundStatement();  
\+    StmtPtr parseSimpleStatement(); // New helper function  
     StmtPtr parseTestStatement();  
     StmtPtr parseIfStatement();  
     StmtPtr parseWhileStatement();  
     StmtPtr parseForStatement();  
     StmtPtr parseSwitchonStatement();  
     StmtPtr parseGotoStatement();  
     StmtPtr parseReturnStatement();  
\-    StmtPtr parseRepeatStatement();  
     StmtPtr parseResultisStatement();  
     StmtPtr parseEndcaseStatement();  
     StmtPtr parseLabeledStatement(const std::string& label\_name);

### **Step 3: Implement the Core Logic in the Parser (Parser.cpp)**

This is the most significant change. We will implement the new "Parse-and-Check" logic.  
**File: Parser.cpp**  
// \--- In Parser.cpp \---

// 1\. REMOVE the old, incorrect parseRepeatStatement() function entirely.  
// StmtPtr Parser::parseRepeatStatement() { ... } // DELETE THIS

// 2\. RENAME your old parseStatement() function to parseSimpleStatement().  
//    Then, REMOVE the 'KwRepeat' case from its switch.  
StmtPtr Parser::parseSimpleStatement() {  
    std::cout \<\< "parseSimpleStatement: " \<\< currentToken.text \<\< std::endl;  
    switch (currentToken.type) {  
        case TokenType::KwLet:  
            return std::make\_unique\<DeclarationStatement\>(parseLetDeclaration());  
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
            return std::make\_unique\<ReturnStatement\>();  
        // REMOVED: case TokenType::KwRepeat:  
        case TokenType::KwLoop:  
            advanceTokens();  
            return std::make\_unique\<LoopStatement\>();  
        case TokenType::KwBreak:  
            advanceTokens();  
            return std::make\_unique\<BreakStatement\>();  
        case TokenType::KwFinish:  
            advanceTokens();  
            return std::make\_unique\<FinishStatement\>();  
        case TokenType::KwEndcase:  
            advanceTokens();  
            return std::make\_unique\<EndcaseStatement\>();  
        case TokenType::KwResultis:  
            return parseResultisStatement();  
        case TokenType::LSection:  
        case TokenType::LBrace:  
            return parseCompoundStatement();  
        case TokenType::Identifier: {  
            if (peekToken.type \== TokenType::Colon) {  
                std::string label\_name \= currentToken.text;  
                advanceTokens(); // consume identifier  
                advanceTokens(); // consume ':'  
                return std::make\_unique\<LabeledStatement\>(label\_name, parseStatement());  
            }  
            return parseExpressionStatement();  
        }  
        default:  
            return parseExpressionStatement();  
    }  
}

// 3\. CREATE the new parseStatement() function which implements the "Parse-and-Check" logic.  
StmtPtr Parser::parseStatement() {  
    // Step 1: Parse a simple statement first. This is the potential loop body 'C'.  
    StmtPtr body \= parseSimpleStatement();

    // Step 2: Check if the statement is followed by a REPEAT modifier.  
    if (currentToken.type \!= TokenType::KwRepeat) {  
        // Not a repeat loop, so just return the simple statement we parsed.  
        return body;  
    }

    // It IS a repeat loop.  
    advanceTokens(); // Consume 'REPEAT'

    // Step 3: Determine the type of REPEAT loop and build the correct AST node.  
    if (currentToken.type \== TokenType::KwWhile) {  
        // Case: C REPEATWHILE E  
        advanceTokens(); // Consume 'WHILE'  
        ExprPtr condition \= parseExpression();  
        return std::make\_unique\<RepeatStatement\>(std::move(body), std::move(condition), RepeatStatement::LoopType::WHILE);

    } else if (currentToken.type \== TokenType::KwUntil) {  
        // Case: C REPEATUNTIL E  
        advanceTokens(); // Consume 'UNTIL'  
        ExprPtr condition \= parseExpression();  
        return std::make\_unique\<RepeatStatement\>(std::move(body), std::move(condition), RepeatStatement::LoopType::UNTIL);

    } else {  
        // Case: C REPEAT (infinite loop)  
        // The next token is not WHILE or UNTIL, so it's a simple infinite loop.  
        return std::make\_unique\<RepeatStatement\>(std::move(body), nullptr, RepeatStatement::LoopType::INFINITE);  
    }  
}

### **Summary of the Refactoring**

By making these changes, you achieve the following:

1. **Correctness:** Your parser now correctly interprets the postfix grammar of BCPL's REPEAT loops as specified in the language reference.  
2. **Robust AST:** The Abstract Syntax Tree accurately represents the structure of the source code, with a clear distinction between the different loop types. This simplifies all subsequent compiler phases.  
3. **Clean Architecture:** The logic is cleanly separated. parseSimpleStatement handles the parsing of a command C, and parseStatement handles the higher-level logic of checking for and applying postfix modifiers. This makes the parser easier to read, maintain, and debug.  
4. **Improved Error Handling:** This structure naturally allows for better error messages. For instance, if REPEAT is encountered in an invalid position, parseSimpleStatement can throw an error, because it's not a valid start to a simple statement.