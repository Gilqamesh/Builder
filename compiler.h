#ifndef BUILDER_PROJECT_BUILDER_COMPILER_H
# define BUILDER_PROJECT_BUILDER_COMPILER_H

# include <filesystem>
# include <vector>
# include <variant>

class compiler_t {
public:
    // Compiles a source file into an object file if the source or any of the headers are newer than the output object file.
    // Returns the path to the output object file.
    // Throws std::runtime_error on failure.
    static std::filesystem::path update_object_file(
        const std::filesystem::path& source_file,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::filesystem::path& output_object_file_file,
        bool position_independent
    );

    // Creates or updates an archive from the input object files if any of them are newer than the output archive.
    // Returns the path to the output archive.
    // Throws std::runtime_error on failure.
    static std::filesystem::path update_static_library(
        const std::vector<std::filesystem::path>& objects,
        const std::filesystem::path& output_static_library
    );

    // Creates an archive from the input archives.
    // Returns the path to the output archive.
    // Throws std::runtime_error on failure.
    static std::filesystem::path bundle_static_libraries(
        const std::vector<std::filesystem::path>& archives,
        const std::filesystem::path& output_static_library
    );

    // Links input object files into a shared library if any of them are newer than the output shared library.
    // Returns the path to the output shared library.
    // Throws std::runtime_error on failure.
    static std::filesystem::path update_shared_libary(
        const std::vector<std::filesystem::path>& input_files,
        const std::filesystem::path& output_shared_libary
    );

    using binary_file_input_t = std::variant<
        std::vector<std::filesystem::path>,
        std::filesystem::path,
        std::string,
        const char*
    >;

    // Links input object files into a binary if any of them are newer than the output binary.
    // Returns the path to the output binary.
    // Throws std::runtime_error on failure.
    static std::filesystem::path update_binary(
        const std::vector<binary_file_input_t>& input_libraries,
        const std::vector<std::string>& additional_linker_flags,
        const std::filesystem::path& output_binary
    );
};

#endif // BUILDER_PROJECT_BUILDER_COMPILER_H
