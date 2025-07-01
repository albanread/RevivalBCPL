// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <filesystem>
#include "Parser.h"
#include "CodeGenerator.h"
#include "JitRuntime.h"
#include "AST.h"
#include "DebugPrinter.h"
#include "Preprocessor.h"
#include "Optimizer.h"

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [options] <source_file.b>\n"
              << "Options:\n"
              << "  --debug     Print debug information (tokens and AST)\n"
              << "  --asm       Output generated assembly\n"
              << "  --opt       Enable optimization\n"
              << "  --help      Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Parse command line arguments
    std::set<std::string> flags;
    std::string source_filename_str;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg.rfind("--", 0) == 0) {
            flags.insert(arg);
        } else {
            source_filename_str = arg;
        }
    }

    if (source_filename_str.empty()) {
        std::cerr << "Error: No source file specified.\n";
        return 1;
    }
    
    std::filesystem::path source_filename(source_filename_str);

    try {
        std::cout << "=== BCPL Compiler ===\n";
        std::cout << "Source file: " << source_filename << "\n\n";

        // Preprocess the source file
        std::cout << "Preprocessing...\n";
        std::string source_code = Preprocessor::getInstance().process(source_filename);
        std::cout << "Preprocessing complete.\n\n";

        std::cout << "=== Preprocessed Source Code ===\n";
        std::cout << source_code << "\n";
        std::cout << "==============================\n\n";

        // Parse source code
        std::cout << "Parsing...\n";
        ProgramPtr ast = Parser::getInstance().parse(source_code);
        std::cout << "Parsing complete.\n\n";

        ProgramPtr optimized_ast = nullptr;
        if (flags.count("--opt")) {
            // Optimize the AST
            std::cout << "Optimizing...\n";
            optimized_ast = Optimizer::getInstance().optimize(std::move(ast));
            std::cout << "Optimization complete.\n\n";
        } else {
            optimized_ast = std::move(ast);
        }

        // Print debug information if requested
        if (flags.count("--debug")) {
            std::cout << "=== Debug Information ===\n";
            DebugPrinter::getInstance().printTokens(source_code);
            DebugPrinter::getInstance().printAST(optimized_ast);

            std::cout << "\n";
        }

        // Generate code
        std::cout << "Generating code...\n";
        JitRuntime::getInstance().registerSymbol("bcpl_vec", (uintptr_t)bcpl_vec);
        JitRuntime::getInstance().registerSymbol("bcpl_unpack_string", (uintptr_t)bcpl_unpack_string);
        JitRuntime::getInstance().registerSymbol("writes", (uintptr_t)bcpl_writes);
        JitRuntime::getInstance().registerSymbol("writen", (uintptr_t)bcpl_writen);
        JitRuntime::getInstance().registerSymbol("newline", (uintptr_t)bcpl_newline);
        JitRuntime::getInstance().registerSymbol("finish", (uintptr_t)bcpl_finish);
        
        CodeGenerator codegen;
        codegen.compile(std::move(optimized_ast));
        std::cout << "Code generation complete.\n\n";

        // Print assembly if requested
        if (flags.count("--asm")) {
            std::cout << "=== Generated Assembly ===\n";
            codegen.printAsm();
            std::cout << "\n";
        }

        std::cout << "Compilation successful.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n=== Compilation Failed ===\n";
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}