#include "Lexer.h"
#include <unordered_map>
#include <cctype>
#include <stdexcept>

// A map to associate BCPL keywords with their corresponding token types.
const std::unordered_map<std::string, TokenType> keywords = {
    {"LET", TokenType::KwLet}, {"AND", TokenType::KwAnd}, {"BE", TokenType::KwBe},
    {"VEC", TokenType::KwVec}, {"IF", TokenType::KwIf}, {"THEN", TokenType::KwThen},
    {"UNLESS", TokenType::KwUnless}, {"TEST", TokenType::KwTest}, {"OR", TokenType::KwOr},
    {"WHILE", TokenType::KwWhile}, {"DO", TokenType::KwDo}, {"UNTIL", TokenType::KwUntil},
    {"REPEAT", TokenType::KwRepeat}, {"REPEATWHILE", TokenType::KwRepeatWhile},
    {"REPEATUNTIL", TokenType::KwRepeatUntil}, {"FOR", TokenType::KwFor},
    {"TO", TokenType::KwTo}, {"BY", TokenType::KwBy}, {"SWITCHON", TokenType::KwSwitchon},
    {"INTO", TokenType::KwInto}, {"CASE", TokenType::KwCase}, {"DEFAULT", TokenType::KwDefault},
    {"ENDCASE", TokenType::KwEndcase}, {"GOTO", TokenType::KwGoto},
    {"RETURN", TokenType::KwReturn},
    {"RESULTIS", TokenType::KwResultis}, {"BREAK", TokenType::KwBreak},
    {"LOOP", TokenType::KwLoop}, {"VALOF", TokenType::KwValof},
    {"MANIFEST", TokenType::KwManifest}, {"STATIC", TokenType::KwStatic},
    {"GLOBAL", TokenType::KwGlobal}, {"TRUE", TokenType::KwTrue}, {"FALSE", TokenType::KwFalse}, {"FINISH", TokenType::KwFinish},
    // Operators that are parsed as identifiers
    {"REM", TokenType::OpRemainder}, {"EQV", TokenType::OpLogEqv}, {"NEQV", TokenType::OpLogNeqv}
};


void Lexer::init(const std::string& source) {
    source_code = source;
    pos = 0;
    current_line = 1;
    current_col = 1;
}

char Lexer::peek() const {
    if (pos >= source_code.length()) return '\0';
    return source_code[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source_code.length()) return '\0';
    return source_code[pos + 1];
}

char Lexer::advance() {
    if (pos >= source_code.length()) return '\0';
    char current_char = source_code[pos];
    pos++;
    if (current_char == '\n') {
        current_line++;
        current_col = 1;
    } else {
        current_col++;
    }
    return current_char;
}

void Lexer::skipWhitespace() {
    while (isspace(peek())) {
        advance();
    }
}

void Lexer::skipComments() {
    while (true) {
        if (peek() == '/' && peekNext() == '/') {
            // Single line comment
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        } else if (peek() == '/' && peekNext() == '*') {
            // Multi-line comment
            advance(); // Consume '/'
            advance(); // Consume '*'
            while (peek() != '\0' && (peek() != '*' || peekNext() != '/')) {
                advance();
            }
            if (peek() != '\0') {
                advance(); // Consume '*'
                advance(); // Consume '/'
            }
        } else {
            break; // Not a comment
        }
        skipWhitespace(); // Skip any whitespace after the comment
    }
}

Token Lexer::identifierOrKeyword() {
    std::string text;
    uint32_t start_col = current_col;
    while (isalnum(peek()) || peek() == '_') {
        text += advance();
    }

    // Check if the identifier is a keyword
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return {it->second, text, 0, 0.0, current_line, start_col};
    }

    return {TokenType::Identifier, text, 0, 0.0, current_line, start_col};
}

Token Lexer::number() {
    std::string text;
    uint32_t start_col = current_col;
    bool is_float = false;
    int base = 10;

    if (peek() == '#') {
        text += advance();
        if (toupper(peek()) == 'X') {
            text += advance();
            base = 16;
        } else {
            base = 8;
        }
    }

    while (isalnum(peek()) || peek() == '.') {
        char c = peek();
        if (c == '.') {
            if (is_float) break; // Can't have two decimal points
            is_float = true;
            text += advance();
        } else if (toupper(c) == 'E' && is_float) {
            // Scientific notation part
            text += advance();
            if (peek() == '+' || peek() == '-') {
                text += advance();
            }
        } else if ((base == 10 && isdigit(c)) ||
                   (base == 8 && c >= '0' && c <= '7') ||
                   (base == 16 && isxdigit(c))) {
            text += advance();
        } else {
            break;
        }
    }
    
    // Determine if it was a float literal based on presence of '.' or 'e'/'E'
    is_float = (text.find('.') != std::string::npos || 
                text.find('e') != std::string::npos ||
                text.find('E') != std::string::npos);

    if (is_float) {
        return {TokenType::FloatLiteral, text, 0, std::stod(text), current_line, start_col};
    } else {
        std::string num_part = text;
        if (base != 10) num_part = text.substr(1);
        if (base == 16) num_part = text.substr(2);
        return {TokenType::IntegerLiteral, text, std::stoll(num_part, nullptr, base), 0.0, current_line, start_col};
    }
}

Token Lexer::stringLiteral() {
    std::string text;
    uint32_t start_col = current_col;
    advance(); // Consume opening "
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '*') { // Handle escape sequences
            advance(); // consume '*'
            char escaped = advance();
            switch (tolower(escaped)) {
                case 'n': text += '\n'; break;
                case 't': text += '\t'; break;
                case 's': text += ' '; break;
                case 'b': text += '\b'; break;
                case 'p': text += '\f'; break;
                case 'c': text += '\r'; break;
                case '"': text += '"'; break;
                case '*': text += '*'; break;
                default: text += escaped; // As per spec, unknown escapes are the char itself
            }
        } else {
            text += advance();
        }
    }
    advance(); // Consume closing "
    return {TokenType::StringLiteral, text, 0, 0.0, current_line, start_col};
}

Token Lexer::charLiteral() {
    uint32_t start_col = current_col;
    advance(); // Consume opening '
    int64_t val = (int64_t)advance(); // Read the character
    if (peek() != '\'') {
      // Handle error, for now we assume valid syntax
    }
    advance(); // Consume closing '
    return {TokenType::CharLiteral, std::string(1, (char)val), val, 0.0, current_line, start_col};
}

Token Lexer::operatorOrDelimiter() {
    uint32_t start_col = current_col;
    char c = advance();
    switch (c) {
        case '(': return {TokenType::LParen, "(", 0, 0.0, current_line, start_col};
        case ')': return {TokenType::RParen, ")", 0, 0.0, current_line, start_col};
        case '{': return {TokenType::LBrace, "{", 0, 0.0, current_line, start_col};
        case '}': return {TokenType::RBrace, "}", 0, 0.0, current_line, start_col};
        case ',': return {TokenType::Comma, ",", 0, 0.0, current_line, start_col};
        case ';': return {TokenType::Semicolon, ";", 0, 0.0, current_line, start_col};
        case '!': return {TokenType::OpBang, "!", 0, 0.0, current_line, start_col};
        case '@': return {TokenType::OpAt, "@", 0, 0.0, current_line, start_col};
        case '&': return {TokenType::OpLogAnd, "&", 0, 0.0, current_line, start_col};
        case '|': return {TokenType::OpLogOr, "|", 0, 0.0, current_line, start_col};
        case '%': return {TokenType::OpCharSub, "%", 0, 0.0, current_line, start_col};

        case '+':
            if (peek() == '.') { advance(); return {TokenType::OpFloatPlus, "+.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpPlus, "+", 0, 0.0, current_line, start_col};
        case '*':
            if (peek() == '.') { advance(); return {TokenType::OpFloatMultiply, "*.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpMultiply, "*", 0, 0.0, current_line, start_col};
        case '/':
            if (peek() == '.') { advance(); return {TokenType::OpFloatDivide, "/.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpDivide, "/", 0, 0.0, current_line, start_col};

        case '-':
            if (peek() == '>') { advance(); return {TokenType::OpConditional, "->", 0, 0.0, current_line, start_col}; }
            if (peek() == '.') { advance(); return {TokenType::OpFloatMinus, "-.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpMinus, "-", 0, 0.0, current_line, start_col};

        case ':':
            if (peek() == '=') { advance(); return {TokenType::OpAssign, ":=", 0, 0.0, current_line, start_col}; }
            return {TokenType::Colon, ":", 0, 0.0, current_line, start_col};
            
        case '~':
            if (peek() == '=') {
                advance();
                if (peek() == '.') { advance(); return {TokenType::OpFloatNe, "~=.", 0, 0.0, current_line, start_col}; }
                return {TokenType::OpNe, "~=", 0, 0.0, current_line, start_col};
            }
            return {TokenType::OpLogNot, "~", 0, 0.0, current_line, start_col};
            
        case '=':
            if (peek() == '.') { advance(); return {TokenType::OpFloatEq, "=.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpEq, "=", 0, 0.0, current_line, start_col};
        
        case '<':
            if (peek() == '=') {
                advance();
                if (peek() == '.') { advance(); return {TokenType::OpFloatLe, "<=.", 0, 0.0, current_line, start_col}; }
                return {TokenType::OpLe, "<=", 0, 0.0, current_line, start_col};
            }
            if (peek() == '<') { advance(); return {TokenType::OpLshift, "<<", 0, 0.0, current_line, start_col}; }
            if (peek() == '.') { advance(); return {TokenType::OpFloatLt, "<.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpLt, "<", 0, 0.0, current_line, start_col};
            
        case '>':
            if (peek() == '=') {
                advance();
                if (peek() == '.') { advance(); return {TokenType::OpFloatGe, ">=.", 0, 0.0, current_line, start_col}; }
                return {TokenType::OpGe, ">=", 0, 0.0, current_line, start_col};
            }
            if (peek() == '>') { advance(); return {TokenType::OpRshift, ">>", 0, 0.0, current_line, start_col}; }
            if (peek() == '.') { advance(); return {TokenType::OpFloatGt, ">.", 0, 0.0, current_line, start_col}; }
            return {TokenType::OpGt, ">", 0, 0.0, current_line, start_col};

        case '.':
            if (peek() == '%') { advance(); return {TokenType::OpFloatVecSub, ".%", 0, 0.0, current_line, start_col}; }
            // Fallthrough for '.' that might start a number like .5
            // This is handled by the number() function logic, so we shouldn't get here
            // unless it's an invalid use of '.'
            return {TokenType::Illegal, ".", 0, 0.0, current_line, start_col};

        case '$':
            if (peek() == '(') { advance(); return {TokenType::LSection, "$(", 0, 0.0, current_line, start_col}; }
            if (peek() == ')') { advance(); return {TokenType::RSection, "$)", 0, 0.0, current_line, start_col}; }
            return {TokenType::Illegal, "$", 0, 0.0, current_line, start_col};

        default:
            return {TokenType::Illegal, std::string(1, c), 0, 0.0, current_line, start_col};
    }
}


Token Lexer::getNextToken() {
    skipWhitespace();
    skipComments();

    if (pos >= source_code.length()) {
        return {TokenType::Eof, "", 0, 0.0, current_line, current_col};
    }

    char current_char = peek();

    if (isalpha(current_char) || current_char == '_') {
        return identifierOrKeyword();
    }
    if (isdigit(current_char) || (current_char == '.' && isdigit(peekNext())) || current_char == '#') {
        return number();
    }
    if (current_char == '"') {
        return stringLiteral();
    }
    if (current_char == '\'') {
        return charLiteral();
    }
    
    return operatorOrDelimiter();
}

std::string Token::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Eof: return "Eof";
        case TokenType::Identifier: return "Identifier";
        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::CharLiteral: return "CharLiteral";
        case TokenType::KwLet: return "LET";
        case TokenType::KwAnd: return "AND";
        case TokenType::KwBe: return "BE";
        case TokenType::KwVec: return "VEC";
        case TokenType::KwIf: return "IF";
        case TokenType::KwThen: return "THEN";
        case TokenType::KwUnless: return "UNLESS";
        case TokenType::KwTest: return "TEST";
        case TokenType::KwOr: return "OR";
        case TokenType::KwWhile: return "WHILE";
        case TokenType::KwDo: return "DO";
        case TokenType::KwUntil: return "UNTIL";
        case TokenType::KwRepeat: return "REPEAT";
        case TokenType::KwRepeatWhile: return "REPEATWHILE";
        case TokenType::KwRepeatUntil: return "REPEATUNTIL";
        case TokenType::KwFor: return "FOR";
        case TokenType::KwTo: return "TO";
        case TokenType::KwBy: return "BY";
        case TokenType::KwSwitchon: return "SWITCHON";
        case TokenType::KwInto: return "INTO";
        case TokenType::KwCase: return "CASE";
        case TokenType::KwDefault: return "DEFAULT";
        case TokenType::KwEndcase: return "ENDCASE";
        case TokenType::KwGoto: return "GOTO";
        case TokenType::KwReturn: return "RETURN";
        case TokenType::KwResultis: return "RESULTIS";
        case TokenType::KwBreak: return "BREAK";
        case TokenType::KwLoop: return "LOOP";
        case TokenType::KwValof: return "VALOF";
        case TokenType::KwManifest: return "MANIFEST";
        case TokenType::KwStatic: return "STATIC";
        case TokenType::KwGlobal: return "GLOBAL";
        case TokenType::KwTrue: return "TRUE";
        case TokenType::KwFalse: return "FALSE";
        case TokenType::KwFinish: return "FINISH";
        case TokenType::OpAssign: return ":=";
        case TokenType::OpPlus: return "+";
        case TokenType::OpMinus: return "-";
        case TokenType::OpMultiply: return "*";
        case TokenType::OpDivide: return "/";
        case TokenType::OpRemainder: return "REM";
        case TokenType::OpEq: return "=";
        case TokenType::OpNe: return "~=";
        case TokenType::OpLt: return "<";
        case TokenType::OpGt: return ">";
        case TokenType::OpLe: return "<=";
        case TokenType::OpGe: return ">=";
        case TokenType::OpLogAnd: return "&";
        case TokenType::OpLogOr: return "|";
        case TokenType::OpLogNot: return "~";
        case TokenType::OpLogEqv: return "EQV";
        case TokenType::OpLogNeqv: return "NEQV";
        case TokenType::OpLshift: return "<<";
        case TokenType::OpRshift: return ">>";
        case TokenType::OpAt: return "@";
        case TokenType::OpBang: return "!";
        case TokenType::OpConditional: return "->";
        case TokenType::OpFloatPlus: return "+.";
        case TokenType::OpFloatMinus: return "-.";
        case TokenType::OpFloatMultiply: return "*.";
        case TokenType::OpFloatDivide: return "/.";
        case TokenType::OpFloatEq: return "=.";
        case TokenType::OpFloatNe: return "~=.";
        case TokenType::OpFloatLt: return "<.";
        case TokenType::OpFloatGt: return ">.";
        case TokenType::OpFloatLe: return "<=.";
        case TokenType::OpFloatGe: return ">=.";
        case TokenType::OpFloatVecSub: return ".%";
        case TokenType::OpCharSub: return "%";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::LBrace: return "{";
        case TokenType::RBrace: return "}";
        case TokenType::LSection: return "$(";
        case TokenType::RSection: return "$)";
        case TokenType::Comma: return ",";
        case TokenType::Colon: return ":";
        case TokenType::Semicolon: return ";";
        case TokenType::Illegal: return "Illegal";
        default: return "Unknown";
    }
}

