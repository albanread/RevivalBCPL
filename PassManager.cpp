#include "PassManager.h"

void PassManager::addPass(std::unique_ptr<OptimizationPass> pass) {
    if (LivenessAnalysisPass* liveness = dynamic_cast<LivenessAnalysisPass*>(pass.get())) {
        livenessAnalysisPass = liveness;
    }
    passes.push_back(std::move(pass));
}

ProgramPtr PassManager::optimize(ProgramPtr program) {
    ProgramPtr current = std::move(program);
    
    for (const auto& pass : passes) {
        current = pass->apply(std::move(current));
    }
    
    return current;
}

size_t PassManager::getPassCount() const {
    return passes.size();
}