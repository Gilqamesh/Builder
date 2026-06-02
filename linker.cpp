#include "linker.h"

#include "phase.h"

#include <format>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>

namespace kernel {

namespace linker {

link_inputs_t binary_link_inputs(const graph::module_t& module, module_config::module_config_t module_config) {
    bool static_libraries = false;
    switch (module_config.library_type) {
        case module_config::library_type_t::STATIC: static_libraries = true; break ;
        case module_config::library_type_t::SHARED: break ;
        default: throw std::runtime_error(std::format("kernel::linker::binary_link_inputs: unknown library_type {}", static_cast<std::underlying_type_t<module_config::library_type_t>>(module_config.library_type)));
    }

    const auto* scc = module.module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format(
            "kernel::linker::binary_link_inputs: module '{}' has no SCC",
            std::format(
                "{}/{}",
                module.workspace->relative_path,
                module.module_relative_path_to_workspace
            )
        ));
    }

    std::unordered_set<const graph::module_scc_t*> visited_sccs;
    std::vector<const graph::module_scc_t*> ordered_sccs;
    const auto collect_scc = [&]<class self_t>(self_t& self, const graph::module_scc_t& current_scc) -> void {
        if (!visited_sccs.insert(&current_scc).second) {
            return ;
        }

        for (const auto* dependency : current_scc.dependencies) {
            self(self, *dependency);
        }

        ordered_sccs.push_back(&current_scc);
    };

    collect_scc(collect_scc, *scc);

    link_inputs_t result;
    for (auto scc_it = ordered_sccs.rbegin(); scc_it != ordered_sccs.rend(); ++scc_it) {
        const auto& current_scc = **scc_it;
        link_input_group_t group {
            .libraries = {},
            .static_library_group = false
        };

        for (auto module_it = current_scc.modules.rbegin(); module_it != current_scc.modules.rend(); ++module_it) {
            phase::output_artifacts_t output {
                .root = filesystem::path_t("/"),
                .artifacts = {}
            };
            const phase::library_phase_t library_phase(**module_it, module_config, output);
            const auto library_output = library_phase.build<phase::library_phase_t>();
            for (const auto& library : library_output.artifacts) {
                group.libraries.push_back(library.path);
            }
        }

        if (!group.libraries.empty()) {
            group.static_library_group = static_libraries && 1 < group.libraries.size();
            result.groups.push_back(group);
        }
    }

    return result;
}

} // namespace linker

} // namespace kernel
