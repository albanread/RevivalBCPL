#ifndef LIVENESS_ANALYSIS_PASS_H
#define LIVENESS_ANALYSIS_PASS_H

#include "OptimizationPass.h"
#include "AST.h"
#include "BasicBlock.h"
#include "CFGBuilder.h"
#include <set>
#include <map> // Using map for consistent ordering during debugging, can switch to unordered_map later

/**
 * @class LivenessAnalysisPass
 * @brief Computes live-in and live-out sets for each statement in the AST.
 *
 * This pass performs a backward dataflow analysis to determine which variables
 * are "live" (i.e., their values might be used later) at the entry and exit
 * points of each statement. This information is crucial for dead code
 * elimination and register allocation.
 */
class LivenessAnalysisPass : public OptimizationPass {
public:
    ProgramPtr apply(ProgramPtr program) override;
    std::string getName() const override;

    // Public accessors for liveness information (for other passes to use)
    const std::set<std::string>& getLiveIn(const Statement* stmt) const;
    const std::set<std::string>& getLiveOut(const Statement* stmt) const;
    const std::set<std::string>& getLiveIn(const Expression* expr) const;
    const std::set<std::string>& getLiveOut(const Expression* expr) const;


private:
    // Maps AST nodes to their computed live-in and live-out sets
    std::map<const Statement*, std::set<std::string>> liveInStatements;
    std::map<const Statement*, std::set<std::string>> liveOutStatements;
    std::map<const Expression*, std::set<std::string>> liveInExpressions;
    std::map<const Expression*, std::set<std::string>> liveOutExpressions;

    // Maps BasicBlocks to their computed live-in and live-out sets
    std::map<const BasicBlock*, std::set<std::string>> liveInBlocks;
    std::map<const BasicBlock*, std::set<std::string>> liveOutBlocks;

    // Stores the CFG for each function
    std::map<std::string, BasicBlock::Ptr> functionCFGs;

    

    // Helper for set operations
    std::set<std::string> setUnion(const std::set<std::string>& s1, const std::set<std::string>& s2);
    std::set<std::string> setDifference(const std::set<std::string>& s1, const std::set<std::string>& s2);
};

#endif // LIVENESS_ANALYSIS_PASS_H