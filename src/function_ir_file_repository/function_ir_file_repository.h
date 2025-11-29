#ifndef FUNCTION_IR_FILE_REPOSITORY_H
# define FUNCTION_IR_FILE_REPOSITORY_H

#include <filesystem>
#include <string>
#include <function_ir.h>
#include <typesystem.h>

class function_ir_file_repository_t {
public:
    function_ir_file_repository_t(const std::filesystem::path& directory_path);

    void save(const function_ir_t& function_ir);

    function_ir_t load(const function_id_t& function_id) const;
    function_ir_t load_latest(const std::string& ns, const std::string& name) const;

private:
    function_id_t path_to_function_id(const std::filesystem::path& path) const;
    std::filesystem::path function_id_to_path(const function_id_t& id) const;

private:
    std::filesystem::path m_directory_path;
};

#endif // FUNCTION_IR_FILE_REPOSITORY_H
