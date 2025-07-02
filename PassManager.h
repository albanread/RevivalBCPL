#ifndef PASS_MANAGER_H
#define PASS_MANAGER_H

#include "OptimizationPass.h"
#include "AST.h"
#include "LivenessAnalysisPass.h"
#include <vector>
#include <memory>

/**
 * @class PassManager
 * @brief Manages and sequences optimization passes.
 * 
 * The PassManager maintains a collection of optimization passes and applies them
 * in sequence to a program AST. It provides a simple interface for registering
 * new passes and running the complete optimization pipeline.
 */
class PassManager {
public:
    /**
     * @brief Register an optimization pass to be run.
     * @param pass The optimization pass to add to the pipeline
     */
    void addPass(std::unique_ptr<OptimizationPass> pass);
    
    /**
     * @brief Apply all registered passes to the program in sequence.
     * @param program The program AST to optimize
     * @return The optimized program AST
     */
    ProgramPtr optimize(ProgramPtr program);
    
    /**
     * @brief Get the number of registered passes.
     * @return The number of passes in the pipeline
     */
    size_t getPassCount() const;
    
private:
    std::vector<std::unique_ptr<OptimizationPass>> passes;
    LivenessAnalysisPass* livenessAnalysisPass = nullptr; // Pointer to the LivenessAnalysisPass instance

public:
    LivenessAnalysisPass* getLivenessAnalysisPass() const { return livenessAnalysisPass; }
};

#endif // PASS_MANAGER_H