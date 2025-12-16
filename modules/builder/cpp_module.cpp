#include "cpp_module.h"

cpp_module_t cpp_module_t::from_c_module(const c_module_t& c_module) {
    return {
        .module_name = c_module.module_name,
        .module_dir = std::filesystem::path(c_module.module_dir),
        .root_dir = std::filesystem::path(c_module.root_dir),
        .artifact_dir = std::filesystem::path(c_module.artifact_dir),
        .builder_dependencies = [&]() {
            std::vector<cpp_module_t*> deps;
            for (unsigned int i = 0; i < c_module.builder_dependencies_count; ++i) {
                deps.push_back(new cpp_module_t(from_c_module(*c_module.builder_dependencies[i])));
            }
            return deps;
        }(),
        .module_dependencies = [&]() {
            std::vector<cpp_module_t*> deps;
            for (unsigned int i = 0; i < c_module.module_dependencies_count; ++i) {
                deps.push_back(new cpp_module_t(from_c_module(*c_module.module_dependencies[i])));
            }
            return deps;
        }(),
        .state = c_module.state,
        .version = c_module.version,
        .scc_id = c_module.scc_id
    };
}
