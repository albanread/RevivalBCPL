#ifndef LIVENESS_ANALYSIS_PASS_H
#define LIVENESS_ANALYSIS_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include "BasicBlock.h"
#include "CFGBuilder.h"

#include <set>
#include <map>
#include <string>
#include <memory>

class LivenessAnalysisPass : public OptimizationPass {
public:
    std::string getName() const override;
    ProgramPtr apply(ProgramPtr program) override;

    // Public accessors for liveness information
    const std::set<std::string>& getLiveIn(const Statement* stmt) const;
    const std::set<std::string>& getLiveOut(const Statement* stmt) const;
    const std::set<std::string>& getLiveIn(const Expression* expr) const;
    const std::set<std::string>& getLiveOut(const Expression* expr) const;

private:
    // Liveness information for basic blocks
    std::map<BasicBlock*, std::set<std::string>> liveInBlocks;
    std::map<BasicBlock*, std::set<std::string>> liveOutBlocks;

    // Liveness information for individual statements and expressions
    std::map<const Statement*, std::set<std::string>> liveInStatements;
    std::map<const Statement*, std::set<std::string>> liveOutStatements;
    std::map<const Expression*, std::set<std::string>> liveInExpressions;
    std::map<const Expression*, std::set<std::string>> liveOutExpressions;

    // Control Flow Graphs for each function
    std::map<std::string, BasicBlock::Ptr> functionCFGs;

    // Helper functions for set operations
    std::set<std::string> setUnion(const std::set<std::string>& s1, const std::set<std::string>& s2);
    std::set<std::string> setDifference(const std::set<std::string>& s1, const std::set<std::string>& s2);
};

#endif // LIVENESS_ANALYSIS_PASS_H
