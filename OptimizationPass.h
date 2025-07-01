#ifndef OPTIMIZATIONPASS_H
#define OPTIMIZATIONPASS_H

#include "AST.h"
#include <memory>
#include <string>

/**
 * @class OptimizationPass
 * @brief Base interface for all optimization passes in the pass manager system.
 * 
 * This abstract class defines the uniform interface that all optimization passes
 * must implement. Each pass takes a Program AST and returns an optimized version.
 * 
 * To add a new optimization pass:
 * 1. Create a class that inherits from OptimizationPass
 * 2. Implement the apply() method with your optimization logic
 * 3. Implement getName() to return a descriptive name for your pass
 * 4. Register the pass with the PassManager in Optimizer::optimize()
 * 
 * Example:
 * ```cpp
 * class MyOptimizationPass : public OptimizationPass {
 * public:
 *     ProgramPtr apply(ProgramPtr program) override {
 *         // Your optimization logic here
 *         return optimized_program;
 *     }
 *     
 *     std::string getName() const override {
 *         return "MyOptimization";
 *     }
 * };
 * ```
 */
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;

    /**
     * @brief Apply this optimization pass to the given program.
     * @param program The program AST to optimize
     * @return A new optimized program AST
     */
    virtual ProgramPtr apply(ProgramPtr program) = 0;

    /**
     * @brief Get the name of this optimization pass.
     * @return A descriptive name for this pass
     */
    virtual std::string getName() const = 0;
};

#endif // OPTIMIZATIONPASS_H