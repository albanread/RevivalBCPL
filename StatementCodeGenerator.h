// StatementCodeGenerator.h
#ifndef STATEMENTCODEGENERATOR_H
#define STATEMENTCODEGENERATOR_H

#include "AST.h"
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

class StatementCodeGenerator {
public:
    StatementCodeGenerator(CodeGenerator& codeGenerator);

    // Statement visitors
    void visitFunctionDeclaration(const FunctionDeclaration* node);
    void visitGlobalDeclaration(const GlobalDeclaration* node);
    void visitManifestDeclaration(const ManifestDeclaration* node);
    void visitLetDeclaration(const LetDeclaration* node);
    void visitCompoundStatement(const CompoundStatement* node);
    void visitIfStatement(const IfStatement* node);
    void visitTestStatement(const TestStatement* node);
    void visitWhileStatement(const WhileStatement* node);
    void visitForStatement(const ForStatement* node);
    void visitSwitchonStatement(const SwitchonStatement* node);
    void visitGotoStatement(const GotoStatement* node);
    void visitLabeledStatement(const LabeledStatement* node);
    void visitAssignment(const Assignment* node);
    void visitRoutineCall(const RoutineCall* node);
    void visitReturnStatement(const ReturnStatement* node);
    void visitResultisStatement(const ResultisStatement* node);
    void visitBreakStatement(const BreakStatement* node);
    void visitLoopStatement(const LoopStatement* node);
    void visitRepeatStatement(const RepeatStatement* node);
    void visitEndcaseStatement(const EndcaseStatement* node);
    void visitFinishStatement(const FinishStatement* node);

private:
    CodeGenerator& codeGen;
    
    // Helper methods for switch statement generation
    void generateJumpTable(const std::vector<SwitchonStatement::SwitchCase>& cases, const std::string& defaultLabel);
    void generateBinarySearchTree(const std::vector<SwitchonStatement::SwitchCase>& cases, const std::string& defaultLabel);
    void generateBinarySearchNode(const std::vector<SwitchonStatement::SwitchCase>& cases, size_t start, size_t end, const std::string& defaultLabel);
    bool isSmallDenseRange(const std::vector<SwitchonStatement::SwitchCase>& cases);
};

#endif // STATEMENTCODEGENERATOR_H