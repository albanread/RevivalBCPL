// ExpressionCodeGenerator.h
#ifndef EXPRESSIONCODEGENERATOR_H
#define EXPRESSIONCODEGENERATOR_H

#include "AST.h"
#include "StringAccess.h"
#include "AArch64Instructions.h"
#include "LabelManager.h"
#include "ScratchAllocator.h"
#include "RegisterManager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Forward declarations
class CodeGenerator;

class ExpressionCodeGenerator {
public:
    ExpressionCodeGenerator(CodeGenerator& codeGenerator);

    // Expression visitors
    void visitNumberLiteral(const NumberLiteral* node);
    void visitStringLiteral(const StringLiteral* node);
    void visitCharLiteral(const CharLiteral* node);
    void visitVariableAccess(const VariableAccess* node);
    void visitUnaryOp(const UnaryOp* node);
    void visitBinaryOp(const BinaryOp* node);
    void visitFunctionCall(const FunctionCall* node);
    void visitConditionalExpression(const ConditionalExpression* node);
    void visitValof(const Valof* node);
    void visitTableConstructor(const TableConstructor* node);
    void visitVectorConstructor(const VectorConstructor* node);
    void visitStringAccess(const StringAccess* node);
    void visitCharacterAccess(const CharacterAccess* node);

private:
    CodeGenerator& codeGen;
};

#endif // EXPRESSIONCODEGENERATOR_H