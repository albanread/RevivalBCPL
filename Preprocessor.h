
#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <set>
#include <filesystem>

class Preprocessor {
public:
    static Preprocessor& getInstance() {
        static Preprocessor instance;
        return instance;
    }

    Preprocessor(const Preprocessor&) = delete;
    Preprocessor& operator=(const Preprocessor&) = delete;

    std::string process(const std::filesystem::path& main_file);

private:
    Preprocessor() = default;
    std::string process_internal(const std::filesystem::path& file_path, std::set<std::filesystem::path>& included_files);
};

#endif // PREPROCESSOR_H
