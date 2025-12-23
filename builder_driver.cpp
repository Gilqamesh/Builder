#include "builder.h"
#include "builder_plugin.h"
#include "builder_plugin_internal.h"
#include "compiler.h"
#include "module.h"
#include "scc.h"
#include "builder_ctx.h"
#include "builder_api.h"
#include "external/nlohmann/json.hpp"

#include <vector>
#include <regex>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <format>
#include <filesystem>
#include <string>
#include <stack>
#include <limits>
#include <algorithm>

#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>

static std::filesystem::path get_artifact_dir(const std::string& module_name, const std::filesystem::path& artifacts_dir, uint64_t version) {
    return artifacts_dir / module_name / (module_name + "@" + std::to_string(version));
}

static void log(const std::string& msg) {
    std::cout << std::format("[{}] {}", BUILDER_DRIVER, msg) << std::endl;
}

static void visit_sccs_topo(scc_t* scc, const std::vector<scc_t*>& sccs, const std::function<void(scc_t*)>& f, std::unordered_set<scc_t*>& visited) {
    if (visited.insert(scc).second == false) {
        return ;
    }

    for (const auto& dep : scc->deps) {
        visit_sccs_topo(dep, sccs, f, visited);
    }

    f(scc);
}

static void visit_sccs_topo(scc_t* scc, const std::vector<scc_t*>& sccs, const std::function<void(scc_t*)>& f) {
    std::unordered_set<scc_t*> visited;
    visit_sccs_topo(scc, sccs, f, visited);
}

static void visit_sccs_rev_topo(scc_t* scc, const std::vector<scc_t*>& sccs, const std::function<void(scc_t*)>& f, std::unordered_set<scc_t*>& visited) {
    if (visited.insert(scc).second == false) {
        return ;
    }

    f(scc);

    for (const auto& dep : scc->deps) {
        visit_sccs_rev_topo(dep, sccs, f, visited);
    }
}

static void visit_sccs_rev_topo(scc_t* scc, const std::vector<scc_t*>& sccs, const std::function<void(scc_t*)>& f) {
    std::unordered_set<scc_t*> visited;
    visit_sccs_rev_topo(scc, sccs, f, visited);
}

static void connect_components(std::vector<scc_t*>& components) {
    for (int id = 0; id < components.size(); ++id) {
        auto& scc = components[id];
        for (const auto& module_by_name : scc->module_by_name) {
            const auto& module = module_by_name.second;
            for (const auto& dep : module->builder_dependencies) {
                if (dep->scc_id != id) {
                    scc->deps.insert(components[dep->scc_id]);
                }
            }
            for (const auto& dep : module->module_dependencies) {
                if (dep->scc_id != id) {
                    scc->deps.insert(components[dep->scc_id]);
                }
            }
        }
    }
}

static void print_strongly_connected_components(const std::vector<scc_t>& sccs) {
    std::cout << "Strongly connected components:" << std::endl;
    for (const auto& scc : sccs) {
        std::cout << std::format("Component {}:", scc.id) << std::endl;
        for (const auto& module_by_name : scc.module_by_name) {
            const auto& module = module_by_name.second;
            std::cout << " - " << module->name << std::endl;
        }
        std::cout << " deps:" << std::endl;
        for (const auto& dep : scc.deps) {
            std::cout << std::format("   - Component {}", dep->id) << std::endl;
        }
    }
}

static std::vector<scc_t*> tarjan(const std::unordered_map<std::string, module_t*>& discovered_modules_by_name, module_t* target_module) {
    std::vector<scc_t*> result;

    struct scc_metadata_t {
        int index;
        int lowlink;
        bool on_stack;
        module_t* module;
    };

    std::unordered_map<module_t*, scc_metadata_t> metadata_by_module;
    int index = 0;
    std::stack<scc_metadata_t*> S;

    std::function<void(scc_metadata_t&)> strong_connect;
    strong_connect = [&](scc_metadata_t& metadata) {
        metadata.index = index;
        metadata.lowlink = index;
        ++index;
        S.push(&metadata);
        metadata.on_stack = true;

        for (const auto& module_dependency : metadata.module->module_dependencies) {
            auto dep_it = metadata_by_module.find(module_dependency);
            if (dep_it == metadata_by_module.end()) {
                throw std::runtime_error(std::format("module '{}' not found in metadata_by_module", module_dependency->name));
            }
            auto& dep_metadata = dep_it->second;
            if (dep_metadata.index == -1) {
                strong_connect(dep_metadata);
                metadata.lowlink = std::min(metadata.lowlink, dep_metadata.lowlink);
            } else if (dep_metadata.on_stack) {
                metadata.lowlink = std::min(metadata.lowlink, dep_metadata.index);
            }
        }

        if (metadata.lowlink == metadata.index) {
            scc_t* scc = new scc_t {
                .id = -1,
                .version = 0,
                .has_static_bundle = false,
                .is_static_bundle_link_command_line_initialized = false
            };
            scc->id = result.size();
            while (true) {
                scc_metadata_t* dep = S.top();
                S.pop();
                dep->on_stack = false;
                dep->module->scc_id = scc->id;
                scc->module_by_name[dep->module->name] = dep->module;
                if (dep->module == metadata.module) {
                    break ;
                }
            }
            result.push_back(scc);
        }
    };

    for (const auto& [module_name, module] : discovered_modules_by_name) {
        if (module != target_module && !module->is_module_dependency) {
            continue ;
        }

        metadata_by_module[module] = scc_metadata_t{ .index = -1, .lowlink = -1, .on_stack = false, .module = module };
    }

    for (auto& [module, metadata] : metadata_by_module) {
        if (metadata.index == -1) {
            strong_connect(metadata);
        }
    }

    connect_components(result);

    // print_strongly_connected_components(result);

    return result;
}

static uint64_t get_module_version(const std::filesystem::path& modules_dir, const std::string& module_name) {
    // TODO: compare builder bin file hash instead of timestamp for more robust check

    const auto module_dir = modules_dir / module_name;
    auto latest_module_file = std::filesystem::file_time_type::min();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(module_dir)) {
        latest_module_file = std::max(latest_module_file, std::filesystem::last_write_time(entry.path()));
    }
    const auto module_version = static_cast<uint64_t>(latest_module_file.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min());

    return module_version;
}

static void relaunch_newer_version(const std::filesystem::path& modules_dir, const std::filesystem::path& artifacts_dir, const std::string& target_module, uint64_t new_version) {
    // compile shared library of the module so that other builder plugins (including builder module) can depend on it to link their plugins
    // then build binary and exec

    const auto module_name = std::string(BUILDER_MODULE_NAME);
    const auto source_dir = modules_dir / module_name;
    const auto builder_driver_cpp = source_dir / BUILDER_DRIVER_CPP;
    const auto builder_plugin_cpp = source_dir / BUILDER_PLUGIN_CPP;
    const auto artifact_dir = get_artifact_dir(module_name, artifacts_dir, new_version);
    const auto builder_plugin_shared_library = artifact_dir / BUILDER_PLUGIN_SO;
    const auto builder_driver_binary = artifact_dir / LINK_MODULE_DIR / BUILDER_DRIVER;

    module_t* module = new module_t(module_name, source_dir, modules_dir, artifact_dir, new_version);
    module->is_versioned = true;

    if (!std::filesystem::exists(artifact_dir)) {

        std::vector<std::filesystem::path> lib_cpp_files;
        for (const auto& entry : std::filesystem::directory_iterator(source_dir)) {
            const auto& path = entry.path();
            if (path.extension() != ".cpp" || path == builder_driver_cpp) {
                continue ;
            }
            lib_cpp_files.push_back(path);
        }
        compiler_t::update_binary(
            {
                builder_driver_cpp,
                compiler_t::update_shared_libary(lib_cpp_files, builder_plugin_shared_library)
            },
            {
                std::format("-DVERSION={}", new_version),
                std::format("-I$(dirname {})", modules_dir.string())
            },
            builder_driver_binary
        );
    }

    std::vector<std::string> args = {
        builder_driver_binary.string(),
        modules_dir.string(),
        target_module,
        artifacts_dir.string()
    };
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    std::string run_command;
    for (const auto& arg : args) {
        if (!run_command.empty()) {
            run_command += " ";
        }
        run_command += arg;
    }
    std::cout << run_command << std::endl;
    extern char **environ;
    execve(builder_driver_binary.string().c_str(), argv.data(), environ);
    throw module_exception_t(module, std::format("execve failed to run '{}', errno: {}, errno_msg: {}", builder_driver_binary.string(), errno, std::strerror(errno)));
}

static module_t* discover_modules(const std::filesystem::path& modules_dir, const std::string& module_name, const std::filesystem::path& artifacts_dir, std::unordered_map<std::string, module_t*>& discovered_modules_by_name) {
    const auto module_dir = modules_dir / module_name;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    auto it = discovered_modules_by_name.find(module_name);
    if (it == discovered_modules_by_name.end()) {
        const auto version = get_module_version(modules_dir, module_name);
        auto artifact_dir = get_artifact_dir(module_name, artifacts_dir, version);
        module_t* module = new module_t(module_name, module_dir, modules_dir, artifact_dir, version);
        it = discovered_modules_by_name.emplace(module_name, module).first;
    } else {
        return it->second;
    }
    module_t* module = it->second;

    const auto deps_json_path = module_dir / DEPS_JSON;
    if (!std::filesystem::exists(deps_json_path)) {
        throw module_exception_t(module, std::format("file does not exist '{}'", deps_json_path.string()));
    }

    nlohmann::json deps_json;
    {
        std::ifstream ifs(deps_json_path);
        if (!ifs) {
            throw module_exception_t(module, std::format("failed to open file '{}'", deps_json_path.string()));
        }

        try {
            deps_json = nlohmann::json::parse(ifs);
        } catch (const nlohmann::json::parse_error& e) {
            throw module_exception_t(module, std::format("failed to parse json file '{}': {}", deps_json_path.string(), e.what()));
        }
    }

    constexpr std::string_view builder_deps_key = "builder_deps";
    auto builder_deps_it = deps_json.find(builder_deps_key);
    if (builder_deps_it == deps_json.end()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), builder_deps_key));
    }
    if (!builder_deps_it->is_array()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), builder_deps_key));
    }
    const auto& builder_deps = builder_deps_it->get_ref<const nlohmann::json::array_t&>();
    if (builder_deps.size() != std::unordered_set<std::string>(builder_deps.begin(), builder_deps.end()).size()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array has duplicates", deps_json_path.string(), builder_deps_key));
    }
    for (const auto& builder_dep : builder_deps) {
        if (!builder_dep.is_string()) {
            throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), builder_deps_key));
        }
        const std::string builder_dep_str = builder_dep.get<std::string>();
        if (builder_dep_str.empty()) {
            throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), builder_deps_key));
        }

        module_t* builder_plugin_dependency = discover_modules(modules_dir, builder_dep_str, artifacts_dir, discovered_modules_by_name);
        builder_plugin_dependency->is_builder_plugin_dependency = true;
        module->builder_dependencies.push_back(builder_plugin_dependency);
    }

    constexpr std::string_view module_deps_key = "module_deps";
    auto module_deps_it = deps_json.find(module_deps_key);
    if (module_deps_it == deps_json.end()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), module_deps_key));
    }
    if (!module_deps_it->is_array()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), module_deps_key));
    }
    const auto& module_deps = module_deps_it->get_ref<const nlohmann::json::array_t&>();
    if (module_deps.size() != std::unordered_set<std::string>(module_deps.begin(), module_deps.end()).size()) {
        throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array has duplicates", deps_json_path.string(), module_deps_key));
    }
    for (const auto& module_dep : module_deps) {
        if (!module_dep.is_string()) {
            throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_deps_key));
        }
        const std::string module_dep_str = module_dep.get<std::string>();
        if (module_dep_str.empty()) {
            throw module_exception_t(module, std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_deps_key));
        }

        module_t* module_dependency = discover_modules(modules_dir, module_dep_str, artifacts_dir, discovered_modules_by_name);
        module_dependency->is_module_dependency = true;
        module->module_dependencies.push_back(module_dependency);
    }

    return module;
}

static void validate_builder_dependency_dag(module_t* module, std::unordered_set<module_t*>& visited, std::unordered_set<module_t*>& on_stack, std::stack<module_t*>& stack) {
    if (on_stack.contains(module)) {
        std::cerr << std::format("detected cycle in builder dependencies:") << std::endl;
        while (!stack.empty()) {
            const auto top = stack.top();
            stack.pop();
            std::cerr << std::format("  module '{}'", top->name) << std::endl;
            if (top == module) {
                break ;
            }
        }
        throw module_exception_t(module, "builder dependency cycle detected");
    }

    if (visited.contains(module)) {
        return ;
    }

    visited.insert(module);
    stack.push(module);
    on_stack.insert(module);

    for (const auto& builder_dependency : module->builder_dependencies) {
        if (module->name == builder_dependency->name && module->name == BUILDER_MODULE_NAME) {
            continue ;
        }
        validate_builder_dependency_dag(builder_dependency, visited, on_stack, stack);
    }

    stack.pop();
    on_stack.erase(module);
}

static void validate_builder_dependency_dag(module_t* module) {
    std::unordered_set<module_t*> visited;
    std::unordered_set<module_t*> on_stack;
    std::stack<module_t*> stack;
    validate_builder_dependency_dag(module, visited, on_stack, stack);
}

static uint64_t version_modules(int index, const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir) {
    scc_t* scc = sccs[index];
    if (scc->version != 0) {
        return scc->version;
    }

    uint64_t max_dep_version = 0;


    for (const auto& dep : scc->deps) {
        max_dep_version = std::max(max_dep_version, version_modules(dep->id, sccs, artifacts_dir));
    }

    for (const auto& module_by_name : scc->module_by_name) {
        const auto& module = module_by_name.second;
        max_dep_version = std::max(max_dep_version, module->version);
    }

    for (auto& module_by_name : scc->module_by_name) {
        auto& module = module_by_name.second;
        module->version = max_dep_version;
        module->artifact_dir = get_artifact_dir(module->name, artifacts_dir, module->version);
        module->is_versioned = true;

        if (!std::filesystem::exists(module->artifact_dir)) {
            if (!std::filesystem::create_directories(module->artifact_dir)) {
                throw module_exception_t(module, std::format("failed to create artifact directory '{}'", module->artifact_dir.string()));
            }
        }
    }

    return scc->version = max_dep_version;
}

static void version_modules(const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir) {
    for (int i = 0; i < sccs.size(); ++i) {
        version_modules(i, sccs, artifacts_dir);
    }
}

static void print_planned_build_order(int& index, module_t* module, std::unordered_set<module_t*>& visited) {
    if (visited.insert(module).second == false) {
        return ;
    }

    for (size_t i = 0; i < module->builder_dependencies.size(); ++i) {
        print_planned_build_order(index, module->builder_dependencies[i], visited);
    }

    ++index;

    std::cout << std::format("  {}. module '{}', version {}", index, module->name, module->version) << std::endl;
    std::cout << std::format("    builder_deps:") << std::endl;
    for (const auto& dep : module->builder_dependencies) {
        std::cout << std::format("      {}", dep->name) << std::endl;
    }
    std::cout << std::format("    module_deps:") << std::endl;
    for (const auto& dep : module->module_dependencies) {
        std::cout << std::format("      {}", dep->name) << std::endl;
    }

    for (size_t i = 0; i < module->module_dependencies.size(); ++i) {
        print_planned_build_order(index, module->module_dependencies[i], visited);
    }
}

static void print_planned_build_order(module_t* module) {
    log(std::format("Planned build order:"));

    int index = 0;
    std::unordered_set<module_t*> visited;
    print_planned_build_order(index, module, visited);
}

static void link_builder_plugin_and_run_export_bundle_shared(builder_ctx_t* ctx, builder_api_t* api, std::unordered_set<module_t*>& visited) {
    if (visited.insert(ctx->module).second == false) {
        return ;
    }

    // for the `builder` module, the builder plugin has to exist at this point for this to work correctly
    const auto builder_plugin = ctx->module->artifact_dir / BUILDER_PLUGIN_SO;
    if (std::filesystem::exists(builder_plugin)) {
        ctx->module->builder_plugin = builder_plugin;
        api->assert_has_shared_libraries(ctx);
        return ;
    } else if (ctx->module->name == BUILDER_MODULE_NAME) {
        // TODO: have per module bootstrap flag and logic
        throw module_exception_t(ctx->module, std::format("temporary error: builder plugin for module '{}' should exist at this point", ctx->module->name));
    }

    // to link the builder plugin of the module, we need either shared bundles of the module's dependencies, or 
    // static pic bundles or a combination of static pic and shared
    // for simplicity, we could require shared bundles only for a module to be dependable for other builders

    // first ensure dependency builder plugins are linked and they produced a shared library of the module
    std::vector<std::filesystem::path> shared_libraries_of_dependencies;
    const auto builder_plugin_o = compiler_t::update_object_file(
        ctx->module->source_dir / BUILDER_PLUGIN_CPP,
        { ctx->module->modules_dir.parent_path() },
        {},
        ctx->module->artifact_dir / BUILDER_PLUGIN_O,
        true
    );
    shared_libraries_of_dependencies.push_back(builder_plugin_o);
    for (const auto& dep : ctx->module->builder_dependencies) {
        builder_ctx_t dep_ctx(dep, ctx->sccs, ctx->artifacts_dir);
        link_builder_plugin_and_run_export_bundle_shared(&dep_ctx, api, visited);
        if (!dep->has_shared_bundle) {
            throw module_exception_t(ctx->module, std::format("dependency module '{}' does not have a shared bundle", dep->name));
        }
        shared_libraries_of_dependencies.insert(shared_libraries_of_dependencies.end(), dep->shared_bundle.begin(), dep->shared_bundle.end());
    }

    // now we are ready to link
    ctx->module->builder_plugin = compiler_t::update_shared_libary(shared_libraries_of_dependencies, builder_plugin);

    // produce shared library bundle for dependent builder_plugins
    if (ctx->module->is_builder_plugin_dependency) {
        api->assert_has_shared_libraries(ctx);
    }

    // now that the module's builder_plugin exists, and it's called to ensure dependent builder_plugins can be linked,
    // also ensure this for the module dependency graph, because those modules's resulting artifacts will be used by builder__link_module
    for (const auto& dep : ctx->module->module_dependencies) {
        builder_ctx_t dep_ctx(dep, ctx->sccs, ctx->artifacts_dir);
        link_builder_plugin_and_run_export_bundle_shared(&dep_ctx, api, visited);
    }
}

static void link_builder_plugin_and_run_export_bundle_shared(builder_ctx_t* ctx, builder_api_t* api) {
    std::unordered_set<module_t*> visited;
    link_builder_plugin_and_run_export_bundle_shared(ctx, api, visited);
}

void remove_stale_module_versions(const std::unordered_map<std::string, module_t*>& discovered_modules_by_name, const std::filesystem::path& artifacts_dir) {
    // NOTE: could also delete all except the last one if versions are ordered (if it's guaranteed nothing else can be in the artifact directories
    for (const auto& module_by_name : discovered_modules_by_name) {
        const auto& module = module_by_name.second;
        for (const auto& entry : std::filesystem::directory_iterator(artifacts_dir / module->name)) {
            if (!entry.is_directory()) {
                continue ;
            }

            const auto& path = entry.path();
            const auto stem = path.stem().string();
            const auto at_pos = stem.find('@');
            if (at_pos == std::string::npos) {
                continue ;
            }

            const auto version_str = stem.substr(at_pos + 1);
            const uint64_t version = std::stoull(version_str);
            if (version < module->version) {
                std::filesystem::remove_all(path);
            }
        }
    }
}

int main(int argc, char **argv) {
    try {
        if (argc < 4) {
            throw std::runtime_error(std::format("usage: {} <modules_dir> <module_name> <artifacts_dir>", argv[0]));
        }

        const auto modules_dir = std::filesystem::absolute(std::filesystem::path(argv[1]));
        const auto module_name = std::string(argv[2]);
        const auto artifacts_dir = std::filesystem::absolute(std::filesystem::path(argv[3]));

        if (!std::filesystem::exists(modules_dir)) {
            throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
        }

        // TODO: running old binaries always upgrades to a newer version, so either fork from the binary version's source code (git integration) or run the latest versioned binary
        const auto builder_module_version = get_module_version(modules_dir, BUILDER_MODULE_NAME);
        if (VERSION < builder_module_version) {
            relaunch_newer_version(modules_dir, artifacts_dir, module_name, builder_module_version);
        }

        std::unordered_map<std::string, module_t*> discovered_modules_by_name;
        module_t* module = discover_modules(modules_dir, module_name, artifacts_dir, discovered_modules_by_name);
        if (!module) {
            throw std::runtime_error(std::format("failed to discover dependencies for module '{}'", module_name));
        }

        validate_builder_dependency_dag(module);

        const auto sccs = tarjan(discovered_modules_by_name, module);

        version_modules(sccs, artifacts_dir);

        builder_api_t api(VERSION);
        builder_ctx_t ctx(module, sccs, artifacts_dir);

        print_planned_build_order(module);

        // link the builder plugin of the target module
        // i.e., link every builder plugins of target module's dependencies (both builder plugin dependencies, and module dependencies,
        // because the target module will need those modules to be available and linked against during `builder__link_module`
        link_builder_plugin_and_run_export_bundle_shared(&ctx, &api);

        // call the `builder__link_module` function of the target module's plugin
        // i.e., let the plugin call into link command line generation to produce its own artifacts
        api.assert_has_linked_artifacts(&ctx);

        // builder_build_self(sccs, artifacts_dir);
        // builder_build_module(*module, artifacts_dir, sccs);

        remove_stale_module_versions(discovered_modules_by_name, artifacts_dir);
    } catch (const module_exception_t& e) {
        module_t* module = e.module();
        std::cerr << std::format("{}: module '{}' error: {}", argv[0], module->name, e.what()) << std::endl;
        if (module->is_versioned) {
            std::filesystem::remove_all(module->artifact_dir);
        }
        return 1;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
