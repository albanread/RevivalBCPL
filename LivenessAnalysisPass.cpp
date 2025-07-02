#include "LivenessAnalysisPass.h"
#include "VariableVisitor.h"
#include "ExpressionLivenessVisitor.h"
#include <algorithm> // For std::set_union, std::set_difference
#include <iostream> // For debugging

std::string LivenessAnalysisPass::getName() const {
    return "Liveness Analysis Pass";
}

ProgramPtr LivenessAnalysisPass::apply(ProgramPtr program) {
    std::cout << "
=== Liveness Analysis Pass: Starting ===" << std::endl;
    // Clear previous analysis results
    liveInStatements.clear();
    liveOutStatements.clear();
    liveInExpressions.clear();
    liveOutExpressions.clear();

    CFGBuilder cfgBuilder;
    functionCFGs = cfgBuilder.build(std::move(program));

    // Initialize live-in and live-out for all basic blocks
    // This will be done within the iterative analysis loop

    // Perform iterative dataflow analysis on the CFG
    bool changed = true;
    while (changed) {
        changed = false;
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

                if (liveInBlocks[block.get()] != oldLiveIn || liveOutBlocks[block.get()] != oldLiveOut) {
                    changed = true;
                }
            }
        }
    }

    // After convergence, propagate liveness information to individual statements and expressions
    // This will require another pass over the CFG and AST.
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
                Statement* stmt = it->get();                liveOutStatements[stmt] = currentLiveOut;                VariableVisitor varVisitor;                stmt->accept(&varVisitor);                std::set<std::string> stmtUse = varVisitor.getUsedVariables();                std::set<std::string> stmtDef = varVisitor.getDefinedVariables();                liveInStatements[stmt] = setUnion(stmtUse, setDifference(currentLiveOut, stmtDef));                currentLiveOut = liveInStatements[stmt]; // Live-in of current becomes live-out of previous                // Propagate to expressions within the statement                ExpressionLivenessVisitor exprVisitor(liveInExpressions, liveOutExpressions, liveInStatements, liveOutStatements, liveInStatements[stmt]);                stmt->accept(&exprVisitor);                // Debugging output                std::cout << "
--- Statement Liveness ---" << std::endl;
                std::cout << "Statement: " << stmt->toString() << std::endl;
                std::cout << "  Live-In: {";
                bool first = true;
                for (const auto& var : liveInStatements[stmt]) {
                    if (!first) std::cout << ", ";
                    std::cout << var;
                    first = false;
                }
                std::cout << "}" << std::endl;
                std::cout << "  Live-Out: {";
                first = true;
                for (const auto& var : liveOutStatements[stmt]) {
                    if (!first) std::cout << ", ";
                    std::cout << var;
                    first = false;
                }
                std::cout << "}" << std::endl;
                std::cout << "  Use: {";
                first = true;
                for (const auto& var : stmtUse) {
                    if (!first) std::cout << ", ";
                    std::cout << var;
                    first = false;
                }
                std::cout << "}" << std::endl;
                std::cout << "  Def: {";
                first = true;
                for (const auto& var : stmtDef) {
                    if (!first) std::cout << ", ";
                    std::cout << var;
                    first = false;
                }
                std::cout << "}" << std::endl;
            }
        }
    }

    std::cout << "
=== Liveness Analysis Pass: Finished ===" << std::endl;
    // Return the original program, as this pass only analyzes, not transforms.
    return program;
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

