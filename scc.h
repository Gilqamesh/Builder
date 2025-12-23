#ifndef BUILDER_PROJECT_BUILDER_SCC_H
# define BUILDER_PROJECT_BUILDER_SCC_H

# include "module.h"

# include <map>
# include <unordered_set>
# include <filesystem>
# include <cstdint>

struct scc_t {
    int id;
    std::map<std::string, module_t*> module_by_name;
    std::unordered_set<scc_t*> deps;
    std::filesystem::path scc_static_lib;
    std::filesystem::path scc_static_pic_lib;
    uint64_t version;

    bool has_static_bundle;
    std::filesystem::path static_bundle;
    bool is_static_bundle_link_command_line_initialized;
    std::string static_bundle_link_command_line;
};

#endif // BUILDER_PROJECT_BUILDER_SCC_H
