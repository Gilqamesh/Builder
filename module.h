#ifndef BUILDER_PROJECT_BUILDER_MODULE_H
# define BUILDER_PROJECT_BUILDER_MODULE_H

# include <filesystem>
# include <string>
# include <vector>

struct module_t {
public:
    std::string name;
    std::filesystem::path src_dir;
    std::filesystem::path modules_dir;
    std::filesystem::path artifact_dir;
    std::vector<module_t*> builder_dependencies;
    std::vector<module_t*> module_dependencies;
    uint64_t version;
    size_t scc_id;
};

#endif // BUILDER_PROJECT_BUILDER_MODULE_H
