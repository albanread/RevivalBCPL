#include "PassManager.h"

void PassManager::addPass(std::unique_ptr<OptimizationPass> pass) {
    passes.push_back(std::move(pass));
}

ProgramPtr PassManager::runPasses(ProgramPtr program) {
    ProgramPtr current = std::move(program);
    
    for (const auto& pass : passes) {
        current = pass->apply(std::move(current));
    }
    
    return current;
}

size_t PassManager::getPassCount() const {
    return passes.size();
}