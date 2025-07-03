#include "LivenessAnalysisPass.h"
#include "VariableVisitor.h"
#include "ExpressionLivenessVisitor.h"
#include "CFGBuilder.h" // Include CFGBuilder
#include "Utils.h" // Include common utility functions
#include <algorithm> // For std::set_union, std::set_difference
#include <iostream> // For debugging
#include <vector>
#include <set>
#include <map>

std::string LivenessAnalysisPass::getName() const {
    return "Liveness Analysis Pass";
}

ProgramPtr LivenessAnalysisPass::apply(ProgramPtr program) {
    std::cout << "\n=== Liveness Analysis Pass: Starting ===\n" << std::flush;
    // Clear previous analysis results
    liveInStatements.clear();
    liveOutStatements.clear();
    liveInExpressions.clear();
    liveOutExpressions.clear();
    liveInBlocks.clear();
    liveOutBlocks.clear();

    CFGBuilder cfgBuilder;
    // Build the CFG from the program. The CFGBuilder will populate its internal functionEntryBlocks map.
    cfgBuilder.build(program); 
    functionCFGs = cfgBuilder.getFunctionEntryBlocks(); // CRITICAL FIX: Populate functionCFGs

    std::cout << "LivenessAnalysisPass: functionCFGs size: " << functionCFGs.size() << "\n" << std::flush;

    // Perform iterative dataflow analysis on the CFG
    bool changed = true;
    int iteration = 0;
    while (changed) {
        changed = false;
        iteration++;
        std::cout << "\n--- Liveness Analysis Iteration " << iteration << " ---\n" << std::flush;

        // Iterate over all functions
        for (const auto& pair : functionCFGs) {
            BasicBlock::Ptr entryBlock = pair.second;

            // Collect all blocks in this function's CFG
            std::vector<BasicBlock::Ptr> allBlocks;
            std::set<BasicBlock::Ptr> visited;
            std::vector<BasicBlock::Ptr> q;
            q.push_back(entryBlock);
            visited.insert(entryBlock);

            while (!q.empty()) {
                BasicBlock::Ptr current = q.back();
                q.pop_back();
                allBlocks.push_back(current);

                for (const auto& succ : current->successors) {
                    if (visited.find(succ) == visited.end()) {
                        visited.insert(succ);
                        q.push_back(succ);
                    }
                }
            }

            // Iterate over basic blocks in reverse topological order (or just reverse for simplicity for now)
            // A proper reverse post-order traversal would be better for convergence.
            std::reverse(allBlocks.begin(), allBlocks.end());

            for (const auto& block : allBlocks) {
                std::cout << "  Processing Block: " << block->toString() << "\n" << std::flush;
                // Initialize live-in and live-out for this block if not already present
                if (liveInBlocks.find(block.get()) == liveInBlocks.end()) {
                    liveInBlocks[block.get()] = {};
                }
                if (liveOutBlocks.find(block.get()) == liveOutBlocks.end()) {
                    liveOutBlocks[block.get()] = {};
                }

                std::set<std::string> oldLiveIn = liveInBlocks[block.get()];
                std::set<std::string> oldLiveOut = liveOutBlocks[block.get()];

                // Compute Live-out[n] = Union (Live-in[s]) for all successors s of n
                std::set<std::string> newLiveOut;
                for (const auto& succ : block->successors) {
                    newLiveOut = setUnion(newLiveOut, liveInBlocks[succ.get()]);
                }
                liveOutBlocks[block.get()] = newLiveOut;

                // Compute Live-in[n] = use[n] U (Live-out[n] - def[n])
                // For basic blocks, use[n] is the union of used variables in the block
                // and def[n] is the union of defined variables in the block.
                // This needs to be computed by iterating through statements in the block.
                std::set<std::string> blockUse;
                std::set<std::string> blockDef;

                // Iterate statements in reverse for use/def of a basic block
                VariableVisitor visitor;
                for (auto it = block->statements.rbegin(); it != block->statements.rend(); ++it) {
                    visitor.clear();
                    (*it)->accept(&visitor);
                    std::set<std::string> stmtUse = visitor.getUsedVariables();
                    std::set<std::string> stmtDef = visitor.getDefinedVariables();

                    blockUse = setUnion(blockUse, setDifference(stmtUse, blockDef));
                    blockDef = setUnion(blockDef, stmtDef);
                }

                liveInBlocks[block.get()] = setUnion(blockUse, setDifference(liveOutBlocks[block.get()], blockDef));

                printSet("  Block Use", blockUse);
                printSet("  Block Def", blockDef);
                printSet("  Block Live-In", liveInBlocks[block.get()]);
                printSet("  Block Live-Out", liveOutBlocks[block.get()]);

                if (liveInBlocks[block.get()] != oldLiveIn || liveOutBlocks[block.get()] != oldLiveOut) {
                    changed = true;
                }
            }
        }
    }

    // After convergence, propagate liveness information to individual statements and expressions
    // This will require another pass over the CFG and AST.
    std::cout << "\n--- Propagating Liveness to Statements and Expressions ---\n" << std::flush;
    for (const auto& pair : functionCFGs) {
        BasicBlock::Ptr entryBlock = pair.second;
        std::vector<BasicBlock::Ptr> allBlocks;
        std::set<BasicBlock::Ptr> visited;
        std::vector<BasicBlock::Ptr> q;
        q.push_back(entryBlock);
        visited.insert(entryBlock);

        while (!q.empty()) {
            BasicBlock::Ptr current = q.back();
            q.pop_back();
            allBlocks.push_back(current);

            // Propagate liveness from block to statements within the block
            // Iterate statements in reverse order for backward propagation
            std::set<std::string> currentLiveOut = liveOutBlocks[current.get()];
            
            for (auto it = current->statements.rbegin(); it != current->statements.rend(); ++it) {
                Statement* stmt = *it; // Now it's a raw pointer
                liveOutStatements[stmt] = currentLiveOut;
                VariableVisitor varVisitor;
                stmt->accept(&varVisitor);
                std::set<std::string> stmtUse = varVisitor.getUsedVariables();
                std::set<std::string> stmtDef = varVisitor.getDefinedVariables();
                liveInStatements[stmt] = setUnion(stmtUse, setDifference(currentLiveOut, stmtDef));
                currentLiveOut = liveInStatements[stmt]; // Live-in of current becomes live-out of previous

                // Propagate to expressions within the statement
                ExpressionLivenessVisitor exprVisitor(liveInExpressions, liveOutExpressions, liveInStatements, liveOutStatements, liveInStatements[stmt]);
                stmt->accept(&exprVisitor);

                // Debugging output for statements
                std::cout << "\n--- Statement Liveness ---\n" << std::flush;
                // std::cout << "Statement: " << stmt->toString() << "\n" << std::flush; // Removed: Statement does not have toString()
                printSet("Live-In", liveInStatements[stmt]);
                printSet("Live-Out", liveOutStatements[stmt]);
                printSet("Use", stmtUse);
                printSet("Def", stmtDef);
            }
        }
    }

    std::cout << "\n=== Liveness Analysis Pass: Finished ===\n" << std::flush;
    // Return the original program, as this pass only analyzes, not transforms.
    return program; // Return the original program, as CFGBuilder now takes a const ref.
}

// --- Public Accessors ---
const std::set<std::string>& LivenessAnalysisPass::getLiveIn(const Statement* stmt) const {
    auto it = liveInStatements.find(stmt);
    if (it != liveInStatements.end()) {
        return it->second;
    }
    static const std::set<std::string> emptySet;
    return emptySet;
}

const std::set<std::string>& LivenessAnalysisPass::getLiveOut(const Statement* stmt) const {
    auto it = liveOutStatements.find(stmt);
    if (it != liveOutStatements.end()) {
        return it->second;
    }
    static const std::set<std::string> emptySet;
    return emptySet;
}

const std::set<std::string>& LivenessAnalysisPass::getLiveIn(const Expression* expr) const {
    auto it = liveInExpressions.find(expr);
    if (it != liveInExpressions.end()) {
        return it->second;
    }
    static const std::set<std::string> emptySet;
    return emptySet;
}

const std::set<std::string>& LivenessAnalysisPass::getLiveOut(const Expression* expr) const {
    auto it = liveOutExpressions.find(expr);
    if (it != liveOutExpressions.end()) {
        return it->second;
    }
    static const std::set<std::string> emptySet;
    return emptySet;
}

// --- Helper for Set Operations ---
std::set<std::string> LivenessAnalysisPass::setUnion(const std::set<std::string>& s1, const std::set<std::string>& s2) {
    std::set<std::string> result;
    std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
                   std::inserter(result, result.begin()));
    return result;
}

std::set<std::string> LivenessAnalysisPass::setDifference(const std::set<std::string>& s1, const std::set<std::string>& s2) {
    std::set<std::string> result;
    std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::inserter(result, result.begin()));
    return result;
}
