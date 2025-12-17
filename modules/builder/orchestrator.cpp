#include <modules/builder/builder.h>
#include <modules/builder/builder_internal.h>
#include <modules/builder/compiler.h>
#include <modules/builder/module.h>
#include <modules/builder/external/nlohmann/json.hpp>

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

static const char* root_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.root_dir.c_str();
}

static const char* artifact_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.artifact_dir.c_str();
}

static const char* module_dir_fn(builder_ctx_t* ctx) {
    return ctx->module.module_dir.c_str();
}

static builder_api_t builder_api = {
    .version = VERSION,
    .root_dir = &root_dir_fn,
    .artifact_dir = &artifact_dir_fn,
    .module_dir = &module_dir_fn
};

void log(const std::string& msg) {
    std::cout << std::format("[{}] {}", ORCHESTRATOR_BIN, msg) << std::endl;
}

struct scc_t {
    int id;
    std::vector<module_t*> modules;
    std::unordered_set<scc_t*> deps;
};

void connect_components(std::vector<scc_t>& components) {
    for (int id = 0; id < components.size(); ++id) {
        auto& scc = components[id];
        for (const auto& module : scc.modules) {
            for (const auto& dep : module->builder_dependencies) {
                if (dep->scc_id != id) {
                    scc.deps.insert(&components[dep->scc_id]);
                }
            }
            for (const auto& dep : module->module_dependencies) {
                if (dep->scc_id != id) {
                    scc.deps.insert(&components[dep->scc_id]);
                }
            }
        }
    }
}

void print_strongly_connected_components(const std::vector<scc_t>& sccs) {
    std::cout << "Strongly connected components:" << std::endl;
    for (const auto& scc : sccs) {
        std::cout << std::format("Component {}:", scc.id) << std::endl;
        for (const auto& module : scc.modules) {
            std::cout << " - " << module->module_name << std::endl;
        }
        std::cout << " deps:" << std::endl;
        for (const auto& dep : scc.deps) {
            std::cout << std::format("   - Component {}", dep->id) << std::endl;
        }
    }
}

std::vector<scc_t> tarjan(const std::unordered_map<std::string, module_t*>& modules_repository) {
    std::vector<scc_t> result;

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
                throw std::runtime_error(std::format("module '{}' not found in metadata_by_module", module_dependency->module_name));
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
                throw std::runtime_error(std::format("module '{}' not found in metadata_by_module", builder_dependency->module_name));
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
            scc_t scc;
            scc.id = result.size();
            while (true) {
                scc_metadata_t* dep = S.top();
                S.pop();
                dep->on_stack = false;
                dep->module->scc_id = scc.id;
                scc.modules.push_back(dep->module);
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

uint64_t get_module_version(const std::string& module_name, const std::filesystem::path& root_dir) {
    // TODO: compare builder bin file hash instead of timestamp for more robust check

    const auto module_dir = root_dir / MODULES_DIR / module_name;
    auto latest_module_file = std::filesystem::file_time_type::min();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(module_dir)) {
        latest_module_file = std::max(latest_module_file, std::filesystem::last_write_time(entry.path()));
    }
    const auto module_version = static_cast<uint64_t>(latest_module_file.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min());

    return module_version;
}

void relaunch_newer_version(const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, const std::string& target_module, uint64_t new_version) {
    const auto module_dir = root_dir / MODULES_DIR / BUILDER;
    const auto orchestrator_cpp = module_dir / ORCHESTRATOR_CPP;
    if (!std::filesystem::exists(orchestrator_cpp)) {
        throw std::runtime_error(std::format("file does not exist '{}'", orchestrator_cpp.string()));
    }

    const auto builder_cpp = module_dir / BUILDER_CPP;

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

    const auto tmp_binary_path = std::filesystem::temp_directory_path() / std::format("{}@{}", ORCHESTRATOR_BIN, new_version);
    const auto tmp_binary_path_str = tmp_binary_path.string();

    const auto compile_next_version_binary_command = std::format("clang++ -g -std=c++23 -I{} -o {} {} -DVERSION={}",
        root_dir.string(),
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
        root_dir.string(),
        artifacts_dir.string(),
        target_module
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

std::filesystem::path create_artifact_dir(const std::string& module_name, const std::filesystem::path& artifacts_dir, uint64_t version) {
    return artifacts_dir / module_name / (module_name + "@" + std::to_string(version));
}

void discover_dependencies(const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, const std::filesystem::path& modules_dir, const std::string& module_name, std::unordered_map<std::string, module_t*>& modules_repository, std::stack<std::string>& visiting_stack) {
    const auto module_dir = modules_dir / module_name;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    auto it = modules_repository.find(module_name);
    if (it == modules_repository.end()) {
        it = modules_repository.emplace(module_name, new module_t{
            .module_name = module_name,
            .module_dir = module_dir,
            .root_dir = root_dir,
            .artifact_dir = "",
            .state = module_t::module_state_t::NOT_DISCOVERED,
            .ran_builder_build_self = false,
            .version = get_module_version(module_name, root_dir),
            .scc_id = 0
        }).first;
        it->second->artifact_dir = create_artifact_dir(module_name, artifacts_dir, it->second->version);
    }
    module_t* module = it->second;

    switch (module->state) {
        case module_t::module_state_t::NOT_DISCOVERED: {
        } break ;
        case module_t::module_state_t::DISCOVERING: {
            std::string message = "circular dependency detected between modules: ";
            while (!visiting_stack.empty()) {
                const auto& visiting_module_name = visiting_stack.top();
                visiting_stack.pop();
                message += visiting_module_name;
                if (!visiting_stack.empty()) {
                    message += " -> ";
                }
            }
            throw std::runtime_error(message);
        } break ;
        case module_t::module_state_t::DISCOVERED: {
            return ;
        } break ;
        case module_t::module_state_t::BUILT_SELF: {
            return ;
        } break ;
        case module_t::module_state_t::BUILT_MODULE: {
            return ;
        } break ;
        default: {
            throw std::runtime_error("unhandled visit state");
        }
    }
    module->state = module_t::module_state_t::DISCOVERING;

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
    constexpr std::string_view module_deps_key = "module_deps";

    if (!deps_json.contains(builder_deps_key) || !deps_json[builder_deps_key].is_array()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), builder_deps_key));
    }
    if (!deps_json.contains(module_deps_key) || !deps_json[module_deps_key].is_array()) {
        throw std::runtime_error(std::format("invalid deps json file '{}': missing '{}' array", deps_json_path.string(), module_deps_key));
    }

    for (const auto& builder_dep : deps_json[builder_deps_key]) {
        if (!builder_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), builder_deps_key));
        }
        const std::string builder_dep_str = builder_dep.get<std::string>();
        if (builder_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), builder_deps_key));
        }

        if (module->module_name == BUILDER && builder_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(builder_dep_str);
        if (it == modules_repository.end()) {
            const auto module_version = get_module_version(builder_dep_str, root_dir);
            it = modules_repository.emplace(builder_dep_str, new module_t{
                .module_name = builder_dep_str,
                .module_dir = modules_dir / builder_dep_str,
                .root_dir = root_dir,
                .artifact_dir = "",
                .state = module_t::module_state_t::NOT_DISCOVERED,
                .version = module_version
            }).first;
            it->second->artifact_dir = create_artifact_dir(builder_dep_str, artifacts_dir, module_version);
        }

        if (std::find(module->builder_dependencies.begin(), module->builder_dependencies.end(), it->second) != module->builder_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), builder_dep_str, builder_deps_key));
        }

        module->builder_dependencies.push_back(it->second);

        visiting_stack.push(builder_dep_str);
        discover_dependencies(root_dir, artifacts_dir, modules_dir, builder_dep_str, modules_repository, visiting_stack);
        visiting_stack.pop();
    }

    module->state = module_t::module_state_t::DISCOVERED;

    for (const auto& runtime_dep : deps_json[module_deps_key]) {
        if (!runtime_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_deps_key));
        }
        const std::string runtime_dep_str = runtime_dep.get<std::string>();
        if (runtime_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_deps_key));
        }

        if (module->module_name == BUILDER && runtime_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(runtime_dep_str);
        if (it == modules_repository.end()) {
            const auto module_version = get_module_version(runtime_dep_str, root_dir);
            const auto artifact_dir = artifacts_dir / runtime_dep_str / (runtime_dep_str + "@" + std::to_string(module_version));

            it = modules_repository.emplace(runtime_dep_str, new module_t{
                .module_name = runtime_dep_str,
                .module_dir = modules_dir / runtime_dep_str,
                .root_dir = root_dir,
                .artifact_dir = artifact_dir,
                .state = module_t::module_state_t::NOT_DISCOVERED,
                .version = module_version
            }).first;
        }

        if (std::find(module->module_dependencies.begin(), module->module_dependencies.end(), it->second) != module->module_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), runtime_dep_str, module_deps_key));
        }

        module->module_dependencies.push_back(it->second);

        discover_dependencies(root_dir, artifacts_dir, modules_dir, runtime_dep_str, modules_repository, visiting_stack);
    }
}

void discover_dependencies(std::unordered_map<std::string, module_t*>& modules_repository, const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, const std::string& module_name) {
    const auto modules_dir = root_dir / MODULES_DIR;

    if (!std::filesystem::exists(modules_dir)) {
        throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
    }

    std::stack<std::string> visiting_stack;

    visiting_stack.push(module_name);
    discover_dependencies(root_dir, artifacts_dir, modules_dir, module_name, modules_repository, visiting_stack);
    visiting_stack.pop();
}

void run_builder_build_self(module_t& module) {
    log(std::format("run builder build self for module '{}', version {}", module.module_name, get_module_version(module.module_name, module.root_dir)));

    builder_ctx_t builder_ctx = {
        .module = module
    };

    if (module.module_name == BUILDER) {
        builder__build_self(&builder_ctx, &builder_api);
    } else {
        const auto builder_cpp = module.module_dir / BUILDER_CPP;
        if (!std::filesystem::exists(builder_cpp)) {
            throw std::runtime_error(std::format("file does not exist '{}'", builder_cpp.string()));
        }
        const auto builder_o = module.artifact_dir / (BUILDER + std::string(".o"));
        const auto builder_so = module.artifact_dir / BUILDER_SO;

        std::vector<std::filesystem::path> shared_library_deps;
        for (const auto& dependency : module.builder_dependencies) {
            const auto dependency_so = dependency->artifact_dir / API_SO_NAME;
            if (!std::filesystem::exists(dependency_so)) {
                throw std::runtime_error(std::format("dependency shared library does not exist '{}'", dependency_so.string()));
            }
            shared_library_deps.push_back(dependency_so);
        }
        shared_library_deps.push_back(
            compiler_t::update_object_file(
                builder_cpp,
                {},
                { module.root_dir },
                {},
                builder_o,
                true
            )
        );

        compiler_t::update_shared_libary(shared_library_deps, builder_so);

        if (!std::filesystem::exists(builder_so)) {
            throw std::runtime_error(std::format("builder plugin '{}' was not created, something went wrong", builder_so.string()));
        }

        // TODO: move to platform layer
        void* builder_handle = dlopen(builder_so.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_handle) {
            const auto message = std::format("failed to load builder plugin '{}': {}", builder_so.string(), dlerror());
            throw std::runtime_error(message);
        }

        builder__build_self_t builder__build_self = (builder__build_self_t)dlsym(builder_handle, BUILDER_BUILD_SELF);
        if (!builder__build_self) {
            const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_BUILD_SELF, builder_so.string(), dlerror());
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

    module.ran_builder_build_self = true;
}

void build_self_order(module_t& module, const std::function<bool(module_t&)>& f, std::unordered_set<module_t*>& visited) {
    if (visited.insert(&module).second == false) {
        return ;
    }

    for (size_t i = 0; i < module.builder_dependencies.size(); ++i) {
        build_self_order(*module.builder_dependencies[i], f, visited);
    }

    if (!f(module)) {
        return ;
    }

    for (size_t i = 0; i < module.module_dependencies.size(); ++i) {
        build_self_order(*module.module_dependencies[i], f, visited);
    }
}

void build_self_order(module_t& module, const std::function<bool(module_t&)>& f) {
    std::unordered_set<module_t*> visited;
    build_self_order(module, f, visited);
}

void builder_build_self(module_t& module) {
    build_self_order(module, [](module_t& module) {
        if (module_t::module_state_t::BUILT_SELF <= module.state) {
            return false;
        }

        if (!std::filesystem::exists(module.artifact_dir)) {
            if (!std::filesystem::create_directories(module.artifact_dir)) {
                throw std::runtime_error(std::format("failed to create artifact directory '{}'", module.artifact_dir.string()));
            }

            try {
                run_builder_build_self(module);
            } catch (...) {
                std::filesystem::remove_all(module.artifact_dir);
                throw ;
            }
        }

        module.state = module_t::module_state_t::BUILT_SELF;

        return true;
    });
}

void collect_bundles_in_linking_order(const module_t& module, const std::vector<scc_t>& sccs, size_t index, std::vector<bool>& visited, std::vector<std::filesystem::path>& bundles) {
    if (visited[index]) {
        return ;
    }
    visited[index] = true;

    std::vector<std::filesystem::path> archives;
    for (const auto& module : sccs[index].modules) {
        archives.push_back(module->artifact_dir / API_LIB_NAME);
    }
    bundles.push_back(compiler_t::bundle_static_libraries(archives, module.artifact_dir / (std::to_string(index) + BUNDLE_NAME)));

    for (const auto& dep_scc : sccs[index].deps) {
        collect_bundles_in_linking_order(module, sccs, dep_scc->id, visited, bundles);
    }
}

std::vector<std::filesystem::path> collect_bundles_in_linking_order(const module_t& module, const std::vector<scc_t>& sccs) {
    std::vector<std::filesystem::path> result;

    std::vector<bool> visited(sccs.size(), false);
    collect_bundles_in_linking_order(module, sccs, module.scc_id, visited, result);
    
    return result;
}

void builder_build_module(module_t& module, const std::vector<scc_t>& sccs) {
    if (
        module_t::module_state_t::BUILT_MODULE <= module.state ||
        !module.ran_builder_build_self
    ) {
        return ;
    }


    if (!std::filesystem::exists(module.artifact_dir)) {
        throw std::runtime_error(std::format("artifact directory '{}' does not exist for module '{}'", module.artifact_dir.string(), module.module_name));
    }

    log(std::format("run builder build module for module '{}', version {}", module.module_name, get_module_version(module.module_name, module.root_dir)));

    const auto bundles = collect_bundles_in_linking_order(module, sccs);
    std::string bundles_str;
    for (const auto& bundle : bundles) {
        if (!bundles_str.empty()) {
            bundles_str += " ";
        }
        bundles_str += bundle.string();
    }


    builder_ctx_t builder_ctx = {
        .module = module
    };

    if (module.module_name == BUILDER) {
        builder__build_module(&builder_ctx, &builder_api, bundles_str.c_str());
    } else {
        const auto builder_so = module.artifact_dir / BUILDER_SO;
        if (!std::filesystem::exists(builder_so)) {
            throw std::runtime_error(std::format("builder plugin '{}' does not exist", builder_so.string()));
        }

        void* builder_handle = dlopen(builder_so.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_handle) {
            const auto message = std::format("failed to load builder plugin '{}': {}", builder_so.string(), dlerror());
            throw std::runtime_error(message);
        }

        builder__build_module_t builder__build_module = (builder__build_module_t)dlsym(builder_handle, BUILDER_BUILD_MODULE);
        if (!builder__build_module) {
            const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_BUILD_MODULE, builder_so.string(), dlerror());
            dlclose(builder_handle);
            throw std::runtime_error(message);
        }

        try {
            builder__build_module(&builder_ctx, &builder_api, bundles_str.c_str());
            dlclose(builder_handle);
        } catch (...) {
            dlclose(builder_handle);
            throw ;
        }
    }

    module.state = module_t::module_state_t::BUILT_MODULE;
}

uint64_t version_modules(int index, const std::vector<scc_t>& sccs, std::unordered_map<int, uint64_t>& version_by_index, const std::filesystem::path& artifacts_dir) {
    if (version_by_index.find(index) != version_by_index.end()) {
        return version_by_index[index];
    }

    uint64_t max_dep_version = 0;

    for (const auto& dep : sccs[index].deps) {
        max_dep_version = std::max(max_dep_version, version_modules(dep->id, sccs, version_by_index, artifacts_dir));
    }

    for (const auto& module : sccs[index].modules) {
        max_dep_version = std::max(max_dep_version, module->version);
    }

    for (auto& module : sccs[index].modules) {
        module->version = max_dep_version;
        module->artifact_dir = create_artifact_dir(module->module_name, artifacts_dir, module->version);
    }

    return version_by_index[index] = max_dep_version;
}

void version_modules(const std::vector<scc_t>& sccs, const std::filesystem::path& artifacts_dir) {
    std::unordered_map<int, uint64_t> version_by_index;
    for (int i = 0; i < sccs.size(); ++i) {
        version_modules(i, sccs, version_by_index, artifacts_dir);
    }
}

int main(int argc, char **argv) {
    try {
        if (argc < 4) {
            throw std::runtime_error(std::format("usage: {} <root_dir> <artifacts_dir> <module_name>", argv[0]));
        }

        const auto root_dir = std::filesystem::path(argv[1]);
        const auto artifacts_dir = std::filesystem::path(argv[2]);
        const auto module_name = std::string(argv[3]);

        const auto builder_module_version = get_module_version(BUILDER, root_dir);
        // TODO: running old binaries always upgrades to a newer version, so either fork from the binary version's source code (git integration) or run the latest versioned binary
        if (VERSION < builder_module_version) {
            relaunch_newer_version(root_dir, artifacts_dir, module_name, builder_module_version);
        }

        std::unordered_map<std::string, module_t*> modules_repository;
        discover_dependencies(modules_repository, root_dir, artifacts_dir, module_name);
        auto it = modules_repository.find(module_name);
        if (it == modules_repository.end()) {
            throw std::runtime_error(std::format("module '{}' not found in modules_repository after discovery", module_name));
        }
        module_t* module = it->second;

        const auto sccs = tarjan(modules_repository);

        version_modules(sccs, artifacts_dir);

        log(std::format("dependency graph:"));
        build_self_order(*module, [](module_t& module) {
            std::cout << std::format("  module '{}', version {}", module.module_name, module.version) << std::endl;
            std::cout << std::format("    builder_deps:") << std::endl;
            for (const auto& dep : module.builder_dependencies) {
                std::cout << std::format("      {}", dep->module_name) << std::endl;
            }
            std::cout << std::format("    module_deps:") << std::endl;
            for (const auto& dep : module.module_dependencies) {
                std::cout << std::format("      {}", dep->module_name) << std::endl;
            }
            return true;
        });

        log(std::format("planned build self order:"));
        build_self_order(*module, [](module_t& module) {
            std::cout << std::format("  module '{}', version {}", module.module_name, module.version) << std::endl;
            return true;
        });

        builder_build_self(*module);
        builder_build_module(*module, sccs);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
