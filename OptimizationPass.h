#ifndef OPTIMIZATION_PASS_H
#define OPTIMIZATION_PASS_H

#include "AST.h"
#include <memory>

/**
 * @class OptimizationPass
 * @brief Base interface for all optimization passes.
 * 
 * This abstract class defines the interface that all optimization passes must implement.
 * Each pass takes a Program AST and returns an optimized version of it.
 */
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    
    /**
     * @brief Apply this optimization pass to the given program.
     * @param program The AST to optimize
     * @return A new optimized AST
     */
    virtual ProgramPtr apply(ProgramPtr program) = 0;
    
    /**
     * @brief Get the name of this optimization pass.
     * @return A string describing this pass
     */
    virtual std::string getName() const = 0;
};

#endif // OPTIMIZATION_PASS_H