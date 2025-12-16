#ifndef BUILDER_CPP_MODULE_API_H
# define BUILDER_CPP_MODULE_API_H

# include "c_module.h"

# include <filesystem>
# include <string>
# include <vector>

class cpp_module_t {
public:
    static cpp_module_t from_c_module(const c_module_t& c_module);

    std::string module_name;
    std::filesystem::path module_dir;
    std::filesystem::path root_dir;
    std::filesystem::path artifact_dir;
    std::vector<cpp_module_t*> builder_dependencies;
    std::vector<cpp_module_t*> module_dependencies;
    c_module_state_t state;
    uint64_t version;
    size_t scc_id;
};

#endif // BUILDER_CPP_MODULE_API_H
