# 1. Set the minimum required version of CMake and define the project.
cmake_minimum_required(VERSION 3.19)
project(compiler CXX)

# 2. Set the C++ standard for the project. C++17 is a good modern default.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# 5. Define the executable for your compiler.
#
#    IMPORTANT: You must list all your .cpp source files here.
#    I have only included CodeGenerator.cpp as it's the only one I know about.
#    Please add your other files (like main.cpp, parser.cpp, etc.).
#
add_executable(compiler
        main.cpp
        Lexer.cpp
        Parser.cpp
        CodeGenerator.cpp
        StatementCodeGenerator.cpp
        ExpressionCodeGenerator.cpp
        AArch64Instructions.cpp
        JitRuntime.cpp
        JITMemoryManager.cpp
        DebugPrinter.cpp
        Optimizer.cpp
        LoopOptimizer.cpp
        PassManager.cpp
        ConstantFoldingPass.cpp
        LoopInvariantCodeMotionPass.cpp
        FunctionInliningPass.cpp
        RepeatUntilOptimizationPass.cpp
        CommonSubexpressionEliminationPass.cpp
        DeadCodeEliminationPass.cpp
        LabelManager.cpp
        ScratchAllocator.cpp
        RegisterManager.cpp
        Preprocessor.cpp
        AST.cpp
)

# Add test executable for JITMemoryManager
add_executable(test_jit_memory_manager
        test_jit_memory_manager.cpp
        JITMemoryManager.cpp
)

# Add advanced test executable for JITMemoryManager
add_executable(test_jit_memory_advanced
        test_jit_memory_advanced.cpp
        JITMemoryManager.cpp
)

# Add test executable for instruction encoding
add_executable(test_instruction_encoding
        test_instruction_encoding.cpp
        AArch64Instructions.cpp
)

if(APPLE)
    set_target_properties(compiler PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
    )
    add_custom_command(TARGET compiler POST_BUILD
        COMMAND codesign -s - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist --force $<TARGET_FILE:compiler>
    )
endif()



# 7. (Optional) Set properties for your executable if needed.
# For example, you can define the output directory for the final binary.
set_target_properties(compiler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)