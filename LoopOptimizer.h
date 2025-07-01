#ifndef LOOPOPTIMIZER_H
#define LOOPOPTIMIZER_H

#include "AST.h"
#include <memory>

// Forward declaration to avoid circular include with Optimizer.h
class Optimizer;

/**
 * @namespace LoopOptimizer
 * @brief A dedicated helper for performing loop-invariant code motion (LICM).
 *
 * This encapsulates all logic for analyzing a loop body, identifying
 * invariant expressions, and hoisting them out of the loop.
 */
namespace LoopOptimizer {
    /**
     * @brief Processes a ForStatement to apply LICM.
     * @param loop The ForStatement node to optimize.
     * @param optimizer A pointer to the main Optimizer instance, used to
     * optimize sub-expressions (like loop bounds) and access
     * shared state (like manifest constants).
     * @return A new StmtPtr. If code was hoisted, this will be a
     * CompoundStatement containing the hoisted declarations followed
     * by the optimized loop. Otherwise, it will be the optimized
     * ForStatement itself.
     */
    StmtPtr process(ForStatement* loop, Optimizer* optimizer);
}

#endif // LOOPOPTIMIZER_H

