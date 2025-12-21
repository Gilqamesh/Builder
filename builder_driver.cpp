#include "builder.h"
#include "builder_plugin.h"
#include "builder_plugin_internal.h"
#include "compiler.h"
#include "module.h"
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

#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>

struct builder_ctx_t {
    const module_t& module;
};

static const char* modules_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.modules_dir.c_str();
}

static const char* artifact_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.artifact_dir.c_str();
}

static const char* src_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.src_dir.c_str();
}

static builder_api_t builder_api = {
    .version = VERSION,
    .modules_dir = &modules_dir_fn,
    .artifact_dir = &artifact_dir_fn,
    .src_dir = &src_dir_fn
};

static void log(const std::string& msg) {
    std::cout << std::format("[{}] {}", BUILDER_DRIVER, msg) << std::endl;
}

struct scc_t {
    int id;
    std::vector<module_t*> modules;
    std::unordered_set<scc_t*> deps;
    std::filesystem::path scc_static_lib;
    std::filesystem::path scc_static_pic_lib;
    uint64_t version;
};

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
        for (const auto& module : scc->modules) {
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
        for (const auto& module : scc.modules) {
            std::cout << " - " << module->name << std::endl;
        }
        std::cout << " deps:" << std::endl;
        for (const auto& dep : scc.deps) {
            std::cout << std::format("   - Component {}", dep->id) << std::endl;
        }
    }
}

static std::vector<scc_t*> tarjan(const std::unordered_map<std::string, module_t*>& modules_repository) {
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

        for (const auto& builder_dependency : metadata.module->builder_dependencies) {
            auto dep_it = metadata_by_module.find(builder_dependency);
            if (dep_it == metadata_by_module.end()) {
                throw std::runtime_error(std::format("module '{}' not found in metadata_by_module", builder_dependency->name));
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
                .version = 0
            };
            scc->id = result.size();
            while (true) {
                scc_metadata_t* dep = S.top();
                S.pop();
                dep->on_stack = false;
                dep->module->scc_id = scc->id;
                scc->modules.push_back(dep->module);
                if (dep->module == metadata.module) {
                    break ;
                }
            }
            result.push_back(scc);
        }
    };

    for (const auto& [module_name, module] : modules_repository) {
        if (metadata_by_module.emplace(module, scc_metadata_t{ .index = -1, .lowlink = -1, .on_stack = false, .module = module }).second == false) {
            throw std::runtime_error(std::format("duplicate modules in modules_repository for module '{}'", module_name));
        }
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
    const auto module_dir = modules_dir / BUILDER_MODULE_NAME;
    const auto builder_driver_cpp = module_dir / BUILDER_DRIVER_CPP;
    if (!std::filesystem::exists(builder_driver_cpp)) {
        throw std::runtime_error(std::format("file does not exist '{}'", builder_driver_cpp.string()));
    }

    const auto builder_plugin_cpp = module_dir / BUILDER_PLUGIN_CPP;

    std::string lib_src;
    for (const auto& entry : std::filesystem::directory_iterator(module_dir)) {
        const auto& path = entry.path();
        if (path.extension() != ".cpp") {
            continue ;
        }

        if (!lib_src.empty()) {
            lib_src += " ";
        }

        const auto stem = path.stem().string();
        lib_src += path.string();
    }

    const auto tmp_binary_path = std::filesystem::temp_directory_path() / std::format("{}@{}", BUILDER_DRIVER, new_version);
    const auto tmp_binary_path_str = tmp_binary_path.string();

    const auto compile_next_version_binary_command = std::format("clang++ -g -std=c++23 -I$(dirname {}) -o {} {} -DVERSION={}",
        modules_dir.string(),
        tmp_binary_path_str,
        lib_src,
        new_version
    );

    // TODO: when using system, paths needs to be escaped as they may contain spaces -> move to platform layer
    // TODO: temporary file creation is not atomic -> move to platform layer
    std::cout << compile_next_version_binary_command << std::endl;
    if (std::system(compile_next_version_binary_command.c_str()) != 0) {
        unlink(tmp_binary_path_str.c_str());
        throw std::runtime_error(std::format("failed to compile temporary next version of builder binary '{}'", tmp_binary_path_str));
    }

    if (!std::filesystem::exists(tmp_binary_path)) {
        unlink(tmp_binary_path_str.c_str());
        throw std::runtime_error(std::format("temporary next version of builder binary '{}' was not created, something went wrong", tmp_binary_path_str));
    }

    std::vector<std::string> args = {
        tmp_binary_path_str,
        modules_dir.string(),
        target_module,
        artifacts_dir.string()
    };
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    std::string run_tmp_command;
    for (const auto& arg : args) {
        if (!run_tmp_command.empty()) {
            run_tmp_command += " ";
        }
        run_tmp_command += arg;
    }
    std::cout << run_tmp_command << std::endl;

    int fd = open(tmp_binary_path_str.c_str(), O_RDONLY);
    const auto errno_val = errno;
    unlink(tmp_binary_path_str.c_str());
    if (fd == -1) {
        throw std::runtime_error(std::format("failed to open temporary next version of builder binary '{}', errno: {}, errno_msg: {}", tmp_binary_path_str, errno_val, std::strerror(errno_val)));
    }
    extern char **environ;
    fexecve(fd, argv.data(), environ);

    close(fd);
    throw std::runtime_error(std::format("fexecve failed to run temporary next version of builder binary '{}', errno: {}, errno_msg: {}", tmp_binary_path_str, errno, std::strerror(errno)));
}

static std::filesystem::path create_artifact_dir(const std::string& module_name, const std::filesystem::path& artifacts_dir, uint64_t version) {
    return artifacts_dir / module_name / (module_name + "@" + std::to_string(version));
}

static module_t* discover_dependencies(const std::filesystem::path& modules_dir, const std::string& module_name, const std::filesystem::path& artifacts_dir, std::unordered_map<std::string, module_t*>& modules_repository) {
    const auto module_dir = modules_dir / module_name;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    auto it = modules_repository.find(module_name);
    if (it == modules_repository.end()) {
        it = modules_repository.emplace(module_name, new module_t{
            .name = module_name,
            .src_dir = module_dir,
            .modules_dir = modules_dir,
            .artifact_dir = "",
            .version = get_module_version(modules_dir, module_name),
            .scc_id = 0
        }).first;
        it->second->artifact_dir = create_artifact_dir(module_name, artifacts_dir, it->second->version);
    } else {
        return it->second;
    }
    module_t* module = it->second;

    const auto deps_json_path = module_dir / DEPS_JSON;
    if (!std::filesystem::exists(deps_json_path)) {
        throw std::runtime_error(std::format("file does not exist '{}'", deps_json_path.string()));
    }

    nlohmann::json deps_json;
    {
        std::ifstream ifs(deps_json_path);
        if (!ifs) {
            throw std::runtime_error(std::format("failed to open file '{}'", deps_json_path.string()));
        }

        try {
            deps_json = nlohmann::json::parse(ifs);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("failed to parse json file '{}': {}", deps_json_path.string(), e.what()));
        }
    }

    constexpr std::string_view builder_deps_key = "builder_deps";
    auto builder_deps_it = deps_json.find(builder_deps_key);
    if (builder_deps_it == deps_json.end()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), builder_deps_key));
    }
    if (!builder_deps_it->is_array()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), builder_deps_key));
    }
    const auto& builder_deps = builder_deps_it->get_ref<const nlohmann::json::array_t&>();
    if (builder_deps.size() != std::unordered_set<std::string>(builder_deps.begin(), builder_deps.end()).size()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array has duplicates", deps_json_path.string(), builder_deps_key));
    }
    for (const auto& builder_dep : builder_deps) {
        if (!builder_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), builder_deps_key));
        }
        const std::string builder_dep_str = builder_dep.get<std::string>();
        if (builder_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), builder_deps_key));
        }

        module->builder_dependencies.push_back(discover_dependencies(modules_dir, builder_dep_str, artifacts_dir, modules_repository));
    }

    constexpr std::string_view module_deps_key = "module_deps";
    auto module_deps_it = deps_json.find(module_deps_key);
    if (module_deps_it == deps_json.end()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), module_deps_key));
    }
    if (!module_deps_it->is_array()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), module_deps_key));
    }
    const auto& module_deps = module_deps_it->get_ref<const nlohmann::json::array_t&>();
    if (module_deps.size() != std::unordered_set<std::string>(module_deps.begin(), module_deps.end()).size()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array has duplicates", deps_json_path.string(), module_deps_key));
    }
    for (const auto& module_dep : module_deps) {
        if (!module_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_deps_key));
        }
        const std::string module_dep_str = module_dep.get<std::string>();
        if (module_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_deps_key));
        }

        module->module_dependencies.push_back(discover_dependencies(modules_dir, module_dep_str, artifacts_dir, modules_repository));
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
        throw std::runtime_error("builder dependency cycle detected");
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

static void run_builder_build_self(module_t& module, const std::vector<scc_t*>& sccs) {
    log(std::format("run builder build self for module '{}', version {}", module.name, module.version));

    builder_ctx_t builder_ctx = {
        .module = module
    };

    if (module.name == BUILDER_MODULE_NAME) {
        builder__build_self(&builder_ctx, &builder_api);
    } else {
        std::vector<std::filesystem::path> shared_library_deps;
        shared_library_deps.push_back(
            compiler_t::update_object_file(
                module.src_dir / BUILDER_PLUGIN_CPP,
                { module.modules_dir.parent_path() },
                {},
                module.artifact_dir / BUILDER_PLUGIN_O,
                true
            )
        );

        scc_t* scc = sccs[module.scc_id];
        for (const auto& dep : scc->deps) {
            visit_sccs_rev_topo(dep, sccs, [&shared_library_deps](scc_t* scc) {
                shared_library_deps.push_back(scc->scc_static_pic_lib);
            });
        }

        const auto builder_plugin_so = compiler_t::update_shared_libary(shared_library_deps, module.artifact_dir / BUILDER_PLUGIN_SO);
        if (!std::filesystem::exists(builder_plugin_so)) {
            throw std::runtime_error(std::format("builder plugin '{}' was not created, something went wrong", builder_plugin_so.string()));
        }

        // TODO: move to platform layer
        void* builder_handle = dlopen(builder_plugin_so.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_handle) {
            const auto message = std::format("failed to load builder plugin '{}': {}", builder_plugin_so.string(), dlerror());
            throw std::runtime_error(message);
        }

        builder__build_self_t builder__build_self = (builder__build_self_t)dlsym(builder_handle, BUILDER_BUILD_SELF);
        if (!builder__build_self) {
            const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_BUILD_SELF, builder_plugin_so.string(), dlerror());
            dlclose(builder_handle);
            throw std::runtime_error(message);
        }

        try {
            builder__build_self(&builder_ctx, &builder_api);
            dlclose(builder_handle);
        } catch (...) {
            dlclose(builder_handle);
            throw ;
        }
    }
}

static void builder_build_self(scc_t* scc, const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir, std::vector<bool>& visited) {
    if (visited[scc->id]) {
        return ;
    }
    visited[scc->id] = true;

    for (const auto& dep : scc->deps) {
        builder_build_self(dep, sccs, artifacts_dir, visited);
    }

    for (auto& module : scc->modules) {
        if (!std::filesystem::exists(module->artifact_dir)) {
            if (!std::filesystem::create_directories(module->artifact_dir)) {
                throw std::runtime_error(std::format("failed to create artifact directory '{}'", module->artifact_dir.string()));
            }
            
            try {
                run_builder_build_self(*module, sccs);
            } catch (...) {
                std::filesystem::remove_all(module->artifact_dir);
                throw ;
            }

            // TODO: refactor
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

    // NOTE: fnv1a
    uint64_t hash = 14695981039346656037ull;
    std::vector<std::string> ordered_names;
    for (auto& module : scc->modules) {
        for (unsigned char c : module->name) {
            hash ^= c;
            hash *= 1099511628211ull;
        }
    }

    const auto scc_hash_dir = artifacts_dir / SCC_DIR / std::to_string(hash);
    const auto scc_version_dir = scc_hash_dir / std::to_string(scc->version);
    scc->scc_static_lib = scc_version_dir / API_STATIC_LIB_NAME;
    scc->scc_static_pic_lib = scc_version_dir / API_STATIC_PIC_LIB_NAME;
    if (!std::filesystem::exists(scc_version_dir)) {
        if (!std::filesystem::create_directories(scc_version_dir)) {
            throw std::runtime_error(std::format("failed to create SCC artifact directory '{}'", scc_version_dir.string()));
        }

        try {
            {
                std::vector<std::filesystem::path> archives;
                for (auto& module : scc->modules) {
                    const auto archive = module->artifact_dir / API_STATIC_LIB_NAME;
                    if (!std::filesystem::exists(archive)) {
                        throw std::runtime_error(std::format("expected archive '{}' to exist for module '{}'", archive.string(), module->name));
                    }
                    archives.push_back(archive);
                }
                compiler_t::bundle_static_libraries(archives, scc->scc_static_lib);
            }


            {
                std::vector<std::filesystem::path> archives;
                for (auto& module : scc->modules) {
                    const auto archive = module->artifact_dir / API_STATIC_PIC_LIB_NAME;
                    if (!std::filesystem::exists(archive)) {
                        throw std::runtime_error(std::format("expected archive '{}' to exist for module '{}'", archive.string(), module->name));
                    }
                    archives.push_back(archive);
                }
                compiler_t::bundle_static_libraries(archives, scc->scc_static_pic_lib);
            }
        } catch (...) {
            std::filesystem::remove_all(scc_version_dir);
            throw ;
        }

        // remove all scc_static_pic_lib up to the latest version
        for (const auto& entry : std::filesystem::directory_iterator(scc_hash_dir)) {
            if (!entry.is_directory()) {
                continue ;
            }

            const auto path = entry.path();
            const auto stem = path.stem().string();
            const auto version_str = stem;
            const uint64_t version = std::stoull(version_str);
            if (version < scc->version) {
                std::filesystem::remove_all(path);
            }
        }
    }
}

static void builder_build_self(const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir) {
    std::vector<bool> visited(sccs.size(), false);
    for (auto& scc : sccs) {
        builder_build_self(scc, sccs, artifacts_dir, visited);
    }
}

static void builder_build_module(module_t& module, const std::filesystem::path& artifacts_dir, const std::vector<scc_t*>& sccs) {
    if (!std::filesystem::exists(module.artifact_dir)) {
        throw std::runtime_error(std::format("artifact directory '{}' does not exist for module '{}'", module.artifact_dir.string(), module.name));
    }

    const auto version_file = module.artifact_dir / std::to_string(VERSION);
    if (std::filesystem::exists(version_file)) {
        return ;
    }

    try {
        log(std::format("run builder build module for module '{}', version {}", module.name, module.version));

        std::string static_libs;
        visit_sccs_rev_topo(sccs[module.scc_id], sccs, [&static_libs](scc_t* scc) {
            if (!static_libs.empty()) {
                static_libs += " ";
            }
            static_libs += scc->scc_static_lib.string();
        });

        builder_ctx_t builder_ctx = {
            .module = module
        };

        if (module.name == BUILDER_MODULE_NAME) {
            builder__build_module(&builder_ctx, &builder_api, static_libs.c_str());
        } else {
            const auto builder_plugin_so = module.artifact_dir / BUILDER_PLUGIN_SO;
            if (!std::filesystem::exists(builder_plugin_so)) {
                throw std::runtime_error(std::format("builder plugin '{}' does not exist", builder_plugin_so.string()));
            }

            void* builder_handle = dlopen(builder_plugin_so.string().c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!builder_handle) {
                const auto message = std::format("failed to load builder plugin '{}': {}", builder_plugin_so.string(), dlerror());
                throw std::runtime_error(message);
            }

            builder__build_module_t builder__build_module = (builder__build_module_t)dlsym(builder_handle, BUILDER_BUILD_MODULE);
            if (!builder__build_module) {
                const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_BUILD_MODULE, builder_plugin_so.string(), dlerror());
                dlclose(builder_handle);
                throw std::runtime_error(message);
            }

            try {
                builder__build_module(&builder_ctx, &builder_api, static_libs.c_str());
                dlclose(builder_handle);
            } catch (...) {
                dlclose(builder_handle);
                throw ;
            }
        }
        std::ofstream ofs(version_file);
    } catch (...) {
        std::filesystem::remove_all(module.artifact_dir);
        throw ;
    }

    // TODO: refactor
    for (const auto& entry : std::filesystem::directory_iterator(artifacts_dir / module.name)) {
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
        if (version < module.version) {
            std::filesystem::remove_all(path);
        }
    }
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

    for (const auto& module : scc->modules) {
        max_dep_version = std::max(max_dep_version, module->version);
    }

    for (auto& module : scc->modules) {
        module->version = max_dep_version;
        module->artifact_dir = create_artifact_dir(module->name, artifacts_dir, module->version);
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

        std::unordered_map<std::string, module_t*> modules_repository;
        module_t* module = discover_dependencies(modules_dir, module_name, artifacts_dir, modules_repository);
        if (!module) {
            throw std::runtime_error(std::format("failed to discover dependencies for module '{}'", module_name));
        }

        validate_builder_dependency_dag(module);

        const auto sccs = tarjan(modules_repository);

        version_modules(sccs, artifacts_dir);

        print_planned_build_order(module);

        builder_build_self(sccs, artifacts_dir);
        builder_build_module(*module, artifacts_dir, sccs);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
