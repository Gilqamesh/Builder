#include "bootstrap_seed.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed {

namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;

static constexpr const char* BOOTSTRAP_SEED_MODULE = "m03gagbhst621faiop1rztfkqp_builder_cli";
static constexpr const char* BOOTSTRAP_SEED_WORKSPACE = "ws0";
static constexpr const char* BOOTSTRAP_SEED_MODULES[] = {
    "m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain",
    "m03gagbhsnusi43zogoacgj2ez_filesystem",
    "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph",
    "m03gagbhst621faiop1rztfkqp_builder_cli",
    "m03gagbhsujjf63n0w3r2w4q6h_build_phases",
    "m03gagbhsvr0m5w15urj0o291m_process",
    "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler",
    "m03gagbhsx4j5z28bqkac3dhhh_shared_library",
    "m03gagbht2l61mj6qitacwbmea_byte_stream",
    "m03gagbhtft23yhjwpp881tfmc_uuid",
    "m03gn7qllwpi68ovctow4jrccj_lexer",
    "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store",
    "m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed",
    "m03gn8rf3pe86v64vphnaam6rl_source_dependencies"
};

workspace_graph::module_t& module(workspace_graph::workspace_graph_t& graph) {
    return *graph.discover_module(workspace_graph::module_name_t(BOOTSTRAP_SEED_MODULE));
}

bool is_module(const workspace_graph::module_t& module) {
    if (!(module.workspace().name() == workspace_graph::workspace_name_t(BOOTSTRAP_SEED_WORKSPACE))) {
        return false;
    }

    for (const auto* bootstrap_module : BOOTSTRAP_SEED_MODULES) {
        if (module.name().unique_name() == bootstrap_module) {
            return true;
        }
    }

    return false;
}

std::vector<workspace_graph::module_t*> modules(workspace_graph::workspace_graph_t& graph) {
    std::vector<workspace_graph::module_t*> result;
    result.reserve(sizeof(BOOTSTRAP_SEED_MODULES) / sizeof(*BOOTSTRAP_SEED_MODULES));

    for (const auto* bootstrap_module : BOOTSTRAP_SEED_MODULES) {
        auto* module = graph.discover_module(workspace_graph::module_name_t(bootstrap_module));
        if (!is_module(*module)) {
            throw std::runtime_error(std::format(
                "m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed::modules: bootstrap seed module '{}' is in workspace '{}', expected '{}'",
                module->name(),
                module->workspace().name(),
                BOOTSTRAP_SEED_WORKSPACE
            ));
        }
        result.push_back(module);
    }

    return result;
}

workspace_graph::version_t version(workspace_graph::workspace_graph_t& graph) {
    uint64_t result = 0;

    for (const auto* module : modules(graph)) {
        result = std::max(result, module->version().value);
    }

    return workspace_graph::version_t(result);
}

filesystem::path_t builder_plugin_path(workspace_graph::workspace_graph_t& graph) {
    const auto path = module(graph).artifact_latest_dir()
        / filesystem::relative_path_t("builder")
        / filesystem::relative_path_t("install")
        / filesystem::relative_path_t("builder.so");
    if (filesystem::exists(path)) {
        return path;
    }

    throw std::runtime_error(std::format(
        "m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed::builder_plugin_path: compiled bootstrap builder plugin '{}' does not exist; run 'make bootstrap' to recreate bootstrap artifacts",
        path
    ));
}

} // namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed
