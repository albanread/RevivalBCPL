#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include <string>

// Helper to print a set of strings
inline void printSet(const std::string& name, const std::set<std::string>& s) {
    std::cout << "  " << name << ": {";
    bool first = true;
    for (const auto& var : s) {
        if (!first) std::cout << ", ";
        std::cout << var;
        first = false;
    }
    std::cout << "}\n" << std::flush;
}

#endif // UTILS_H
