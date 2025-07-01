#ifndef PASSMANAGER_H
#define PASSMANAGER_H

#include "OptimizationPass.h"
#include <memory>
#include <vector>

/**
 * @class PassManager
 * @brief Manages and sequences multiple optimization passes.
 * 
 * The PassManager is responsible for:
 * - Storing a collection of optimization passes
 * - Running them in sequence on a program AST
 * - Providing an interface to add new passes
 * 
 * Passes are run in the order they are added to the manager.
 * Each pass receives the output of the previous pass as input.
 */
class PassManager {
public:
    /**
     * @brief Add an optimization pass to be run by this manager.
     * @param pass The optimization pass to add (takes ownership)
     */
    void addPass(std::unique_ptr<OptimizationPass> pass);

    /**
     * @brief Run all registered passes on the given program.
     * @param program The program AST to optimize
     * @return The optimized program AST after all passes have been applied
     */
    ProgramPtr runPasses(ProgramPtr program);

    /**
     * @brief Get the number of passes registered with this manager.
     * @return The number of passes
     */
    size_t getPassCount() const;

private:
    std::vector<std::unique_ptr<OptimizationPass>> passes;
};

#endif // PASSMANAGER_H