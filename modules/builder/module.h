#ifndef MODULES_BUILDER_MODULE_H
# define MODULES_BUILDER_MODULE_H

# include <filesystem>
# include <string>
# include <vector>

struct module_t {
public:
    enum class module_state_t {
        NOT_VISITED,
        VISITING,
        VISITED,
        BUILT
    };

public:
    std::string module_name;
    std::filesystem::path module_dir;
    std::filesystem::path root_dir;
    std::filesystem::path artifact_dir;
    std::vector<module_t*> builder_dependencies;
    std::vector<module_t*> module_dependencies;
    module_state_t state;
    uint64_t version;
    size_t scc_id;
};

#endif // MODULES_BUILDER_MODULE_H
