#include "helpers.h"

#include <format>
#include <stdexcept>
#include <string>

#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h>

namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command {

m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t load_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    auto& workspace_graph = module.workspace().graph();
    auto friendly_name = module.name().friendly_name();
    std::string binding_name = "lisp_" + friendly_name;
    if (friendly_name == "workspace_graph") { binding_name = "lisp_graph"; }
    if (friendly_name == "cxx_toolchain") { binding_name = "lisp_compiler"; }
    if (friendly_name == "build_phases") { binding_name = "lisp_phase"; }
    if (friendly_name == "builder_cli") { binding_name = "lisp_kernel"; }
    if (friendly_name == "lisp_runtime") { binding_name = "lisp_runtime_bindings"; }
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* binding = nullptr;
    for (const auto& module_name : workspace_graph.module_names()) {
        if (module_name.friendly_name() == binding_name) {
            if (binding != nullptr) {
                throw std::runtime_error(std::format("Lisp binding '{}' is ambiguous; select its complete identity", binding_name));
            }
            binding = workspace_graph.discover_module(module_name);
        }
    }
    if (binding != nullptr) { return load_module(*binding); }
    const auto dependencies = m03gn8rf3pe86v64vphnaam6rl_source_dependencies::dependency_modules(
        module,
        m03gn8rf3pe86v64vphnaam6rl_source_dependencies::library_source_files(module.source_dir()),
        false,
        m03gn8rf3pe86v64vphnaam6rl_source_dependencies::dependency_mode_t::MODULE,
        [](const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& dependency) { return m03gn8rf3pe86v64vphnaam6rl_source_dependencies::library_source_files(dependency.source_dir()); }
    );
    // Builder libraries leave dependency symbols for their caller to resolve.
    for (auto iterator = dependencies.rbegin(); iterator != dependencies.rend(); ++iterator) {
        const auto phase = m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make(**iterator);
        const auto installed = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t>();
        const auto libraries = m03gagbhsnusi43zogoacgj2ez_filesystem::find(installed.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t([](const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) { return path.extension() == ".so"; }), m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all);
        for (const auto& library : libraries) {
            m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t dependency(library.path(), m03gagbhsx4j5z28bqkac3dhhh_shared_library::lifetime_t::PROCESS, m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_resolution_t::LAZY, m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_visibility_t::GLOBAL);
        }
    }
    const auto phase = m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make(module);
    return m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t(phase->library(), m03gagbhsx4j5z28bqkac3dhhh_shared_library::lifetime_t::PROCESS, m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_resolution_t::LAZY, m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_visibility_t::LOCAL);
}

} // namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command
