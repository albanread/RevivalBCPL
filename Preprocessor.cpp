#include "Preprocessor.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string Preprocessor::process(const std::filesystem::path& main_file) {
    std::set<std::filesystem::path> included_files;
    return process_internal(main_file, included_files);
}

std::string Preprocessor::process_internal(const std::filesystem::path& file_path, std::set<std::filesystem::path>& included_files) {
    if (included_files.count(file_path)) {
        // Circular dependency detected, return empty string or handle as an error
        return ""; 
    }
    included_files.insert(file_path);

    std::ifstream source_file(file_path);
    if (!source_file) {
        throw std::runtime_error("Preprocessor: Could not open source file '" + file_path.string() + "'");
    }

    std::stringstream output;
    std::string line;
    size_t line_num = 0;

    while (std::getline(source_file, line)) {
        line_num++;
        if (line.rfind("GET", 0) == 0) {
            size_t start_quote = line.find('"');
            size_t end_quote = line.rfind('"');
            if (start_quote != std::string::npos && end_quote != std::string::npos && start_quote != end_quote) {
                std::string include_filename = line.substr(start_quote + 1, end_quote - start_quote - 1);
                std::filesystem::path include_path = file_path.parent_path() / include_filename;
                output << process_internal(include_path, included_files);
            } else {
                output << line << '\n';
            }
        } else {
            output << line << '\n';
        }
    }

    return output.str();
}

