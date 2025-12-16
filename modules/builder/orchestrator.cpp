#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

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

struct scc_t {
    int id;
    std::vector<cpp_module_t*> modules;
    std::unordered_set<scc_t*> deps;
};

void connect_components(std::vector<scc_t>& components) {
    for (int id = 0; id < components.size(); ++id) {
        auto& scc = components[id];
        for (const auto& cpp_module : scc.modules) {
            for (const auto& dep : cpp_module->builder_dependencies) {
                if (dep->scc_id != id) {
                    scc.deps.insert(&components[dep->scc_id]);
                }
            }
            for (const auto& dep : cpp_module->module_dependencies) {
                if (dep->scc_id != id) {
                    scc.deps.insert(&components[dep->scc_id]);
                }
            }
        }
    }
}

std::vector<scc_t> tarjan(const std::unordered_map<std::string, cpp_module_t*>& modules_repository) {
    std::vector<scc_t> result;

    struct scc_metadata_t {
        int index;
        int lowlink;
        bool on_stack;
        cpp_module_t* cpp_module;
    };

    std::unordered_map<cpp_module_t*, scc_metadata_t> metadata_by_module;
    int index = 0;
    std::stack<scc_metadata_t*> S;

    std::function<void(scc_metadata_t&)> strong_connect;
    strong_connect = [&](scc_metadata_t& metadata) {
        metadata.index = index;
        metadata.lowlink = index;
        ++index;
        S.push(&metadata);
        metadata.on_stack = true;

        for (const auto& module_dependency : metadata.cpp_module->module_dependencies) {
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

        for (const auto& builder_dependency : metadata.cpp_module->builder_dependencies) {
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
                dep->cpp_module->scc_id = scc.id;
                scc.modules.push_back(dep->cpp_module);
                if (dep->cpp_module == metadata.cpp_module) {
                    break ;
                }
            }
            result.push_back(scc);
        }
    };

    for (const auto& [module_name, cpp_module] : modules_repository) {
        if (metadata_by_module.emplace(cpp_module, scc_metadata_t{ .index = -1, .lowlink = -1, .on_stack = false, .cpp_module = cpp_module }).second == false) {
            throw std::runtime_error(std::format("duplicate modules in modules_repository for module '{}'", module_name));
        }
    }

    for (auto& [cpp_module, metadata] : metadata_by_module) {
        if (metadata.index == -1) {
            strong_connect(metadata);
        }
    }

    connect_components(result);

    std::cout << "Strongly connected components:" << std::endl;
    for (const auto& scc : result) {
        std::cout << std::format("Component {}:", scc.id) << std::endl;
        for (const auto& module : scc.modules) {
            std::cout << " - " << module->module_name << std::endl;
        }
        std::cout << " deps:" << std::endl;
        for (const auto& dep : scc.deps) {
            std::cout << std::format("   - Component {}", dep->id) << std::endl;
        }
    }

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

void discover_dependencies(const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, const std::filesystem::path& modules_dir, const std::string& module_name, std::unordered_map<std::string, cpp_module_t*>& modules_repository, std::stack<std::string>& visiting_stack) {
    const auto module_dir = modules_dir / module_name;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    auto it = modules_repository.find(module_name);
    if (it == modules_repository.end()) {
        const auto module_version = get_module_version(module_name, root_dir);
        const auto artifact_dir = artifacts_dir / module_name / (module_name + "@" + std::to_string(module_version));

        it = modules_repository.emplace(module_name, new cpp_module_t{
            .module_name = module_name,
            .module_dir = module_dir,
            .root_dir = root_dir,
            .artifact_dir = artifact_dir,
            .state = NOT_VISITED,
            .version = module_version
        }).first;
    }
    cpp_module_t* cpp_module = it->second;

    switch (cpp_module->state) {
        case NOT_VISITED: {
        } break ;
        case VISITING: {
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
        case VISITED: {
            return ;
        } break ;
        case BUILT: {
            return ;
        } break ;
        default: {
            throw std::runtime_error("unhandled visit state");
        }
    }
    cpp_module->state = VISITING;

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

        if (cpp_module->module_name == BUILDER && builder_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(builder_dep_str);
        if (it == modules_repository.end()) {
            const auto module_version = get_module_version(builder_dep_str, root_dir);
            const auto artifact_dir = artifacts_dir / builder_dep_str / (builder_dep_str + "@" + std::to_string(module_version));

            it = modules_repository.emplace(builder_dep_str, new cpp_module_t{
                .module_name = builder_dep_str,
                .module_dir = modules_dir / builder_dep_str,
                .root_dir = root_dir,
                .artifact_dir = artifact_dir,
                .state = NOT_VISITED,
                .version = module_version
            }).first;
        }

        if (std::find(cpp_module->builder_dependencies.begin(), cpp_module->builder_dependencies.end(), it->second) != cpp_module->builder_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), builder_dep_str, builder_deps_key));
        }

        cpp_module->builder_dependencies.push_back(it->second);

        visiting_stack.push(builder_dep_str);
        discover_dependencies(root_dir, artifacts_dir, modules_dir, builder_dep_str, modules_repository, visiting_stack);
        visiting_stack.pop();
    }

    cpp_module->state = VISITED;

    for (const auto& runtime_dep : deps_json[module_deps_key]) {
        if (!runtime_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_deps_key));
        }
        const std::string runtime_dep_str = runtime_dep.get<std::string>();
        if (runtime_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_deps_key));
        }

        if (cpp_module->module_name == BUILDER && runtime_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(runtime_dep_str);
        if (it == modules_repository.end()) {
            const auto module_version = get_module_version(runtime_dep_str, root_dir);
            const auto artifact_dir = artifacts_dir / runtime_dep_str / (runtime_dep_str + "@" + std::to_string(module_version));

            it = modules_repository.emplace(runtime_dep_str, new cpp_module_t{
                .module_name = runtime_dep_str,
                .module_dir = modules_dir / runtime_dep_str,
                .root_dir = root_dir,
                .artifact_dir = artifact_dir,
                .state = NOT_VISITED,
                .version = module_version
            }).first;
        }

        if (std::find(cpp_module->module_dependencies.begin(), cpp_module->module_dependencies.end(), it->second) != cpp_module->module_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), runtime_dep_str, module_deps_key));
        }

        cpp_module->module_dependencies.push_back(it->second);

        discover_dependencies(root_dir, artifacts_dir, modules_dir, runtime_dep_str, modules_repository, visiting_stack);
    }
}

void discover_dependencies(std::unordered_map<std::string, cpp_module_t*>& modules_repository, const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, const std::string& module_name) {
    const auto modules_dir = root_dir / MODULES_DIR;

    if (!std::filesystem::exists(modules_dir)) {
        throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
    }

    std::stack<std::string> visiting_stack;

    visiting_stack.push(module_name);
    discover_dependencies(root_dir, artifacts_dir, modules_dir, module_name, modules_repository, visiting_stack);
    visiting_stack.pop();
}

void build_builder_artifacts(cpp_module_t& cpp_module, c_module_t* c_module, const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir, cpp_module_t& target_module) {
    if (BUILT <= cpp_module.state) {
        return ;
    }

    for (size_t i = 0; i < cpp_module.builder_dependencies.size(); ++i) {
        build_builder_artifacts(*cpp_module.builder_dependencies[i], c_module->builder_dependencies[i], root_dir, artifacts_dir, target_module);
    }

    if (!std::filesystem::exists(cpp_module.artifact_dir)) {
        if (!std::filesystem::create_directories(cpp_module.artifact_dir)) {
            throw std::runtime_error(std::format("failed to create artifact directory '{}'", cpp_module.artifact_dir.string()));
        }
    }

    std::cout << std::format("Build builder artifacts for module '{}' version {}", cpp_module.module_name, cpp_module.version) << std::endl;

    try {
        if (cpp_module.module_name == BUILDER) {
            c_module__build_builder_artifacts(c_module);
        } else {
            const auto builder_cpp = cpp_module.module_dir / BUILDER_CPP;
            if (!std::filesystem::exists(builder_cpp)) {
                throw std::runtime_error(std::format("file does not exist '{}'", builder_cpp.string()));
            }
            const auto builder_d = cpp_module.artifact_dir / (BUILDER + std::string(".d"));
            const auto builder_o = cpp_module.artifact_dir / (BUILDER + std::string(".o"));
            const auto builder_so = cpp_module.artifact_dir / BUILDER_SO;

            std::vector<std::filesystem::path> shared_library_deps;
            for (const auto& dependency : cpp_module.builder_dependencies) {
                const auto dependency_so = dependency->artifact_dir / API_SO_NAME;
                if (!std::filesystem::exists(dependency_so)) {
                    throw std::runtime_error(std::format("dependency so does not exist '{}'", dependency_so.string()));
                }
                shared_library_deps.push_back(dependency_so);
            }
            shared_library_deps.push_back(
                compiler_t::update_object_file(
                    builder_cpp,
                    {},
                    { root_dir },
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

            c_module__build_builder_artifacts_t c_module__build_builder_artifacts = (c_module__build_builder_artifacts_t)dlsym(builder_handle, BUILD_BUILDER_ARTIFACTS);
            if (!c_module__build_builder_artifacts) {
                const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILD_BUILDER_ARTIFACTS, builder_so.string(), dlerror());
                dlclose(builder_handle);
                throw std::runtime_error(message);
            }

            try {
                c_module__build_builder_artifacts(c_module);
                dlclose(builder_handle);
            } catch (...) {
                dlclose(builder_handle);
                throw ;
            }
        }
    } catch (...) {
        std::filesystem::remove_all(cpp_module.artifact_dir);
        throw ;
    }

    cpp_module.state = BUILT;
    c_module->state = BUILT;

    for (size_t i = 0; i < cpp_module.module_dependencies.size(); ++i) {
        build_builder_artifacts(*cpp_module.module_dependencies[i], c_module->module_dependencies[i], root_dir, artifacts_dir, target_module);
    }
}

void collect_bundles_in_linking_order(const cpp_module_t& cpp_module, const std::vector<scc_t>& sccs, size_t index, std::vector<bool>& visited, std::vector<std::filesystem::path>& bundles) {
    if (visited[index]) {
        return ;
    }
    visited[index] = true;

    std::vector<std::filesystem::path> archives;
    for (const auto& cpp_module : sccs[index].modules) {
        archives.push_back(cpp_module->artifact_dir / API_LIB_NAME);
    }
    bundles.push_back(compiler_t::bundle_static_libraries(archives, cpp_module.artifact_dir / (std::to_string(index) + BUNDLE_NAME)));

    for (const auto& dep_scc : sccs[index].deps) {
        collect_bundles_in_linking_order(cpp_module, sccs, dep_scc->id, visited, bundles);
    }
}

std::vector<std::filesystem::path> collect_bundles_in_linking_order(const cpp_module_t& cpp_module, const std::vector<scc_t>& sccs) {
    std::vector<std::filesystem::path> result;

    std::vector<bool> visited(sccs.size(), false);
    collect_bundles_in_linking_order(cpp_module, sccs, cpp_module.scc_id, visited, result);
    
    return result;
}

void build_module_artifacts(const cpp_module_t& cpp_module, c_module_t* c_module, const std::vector<scc_t>& sccs) {
    std::cout << std::format("Build module artifacts for module '{}', version {}", cpp_module.module_name, get_module_version(cpp_module.module_name, cpp_module.root_dir)) << std::endl;

    const auto bundles = collect_bundles_in_linking_order(cpp_module, sccs);
    std::string bundles_str;
    for (const auto& bundle : bundles) {
        if (!bundles_str.empty()) {
            bundles_str += " ";
        }
        bundles_str += bundle.string();
    }

    if (cpp_module.module_name == BUILDER) {
        c_module__build_module_artifacts(c_module, bundles_str.c_str());
    } else {
        const auto builder_so = cpp_module.artifact_dir / BUILDER_SO;
        if (!std::filesystem::exists(builder_so)) {
            throw std::runtime_error(std::format("builder plugin '{}' does not exist", builder_so.string()));
        }

        void* builder_handle = dlopen(builder_so.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_handle) {
            const auto message = std::format("failed to load builder plugin '{}': {}", builder_so.string(), dlerror());
            throw std::runtime_error(message);
        }

        c_module__build_module_artifacts_t c_module__build_module_artifacts = (c_module__build_module_artifacts_t)dlsym(builder_handle, BUILD_MODULE_ARTIFACTS);
        if (!c_module__build_module_artifacts) {
            const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILD_MODULE_ARTIFACTS, builder_so.string(), dlerror());
            dlclose(builder_handle);
            throw std::runtime_error(message);
        }

        try {
            c_module__build_module_artifacts(c_module, bundles_str.c_str());
            dlclose(builder_handle);
        } catch (...) {
            dlclose(builder_handle);
            throw ;
        }
    }
}

c_module_t* from_cpp_module(const cpp_module_t& cpp_module, std::unordered_map<std::string, c_module_t*>& visited) {
    auto it = visited.find(cpp_module.module_name);
    if (it != visited.end()) {
        return it->second;
    }

    c_module_t* c_module = static_cast<c_module_t*>(std::malloc(sizeof(c_module_t)));
    visited.emplace(cpp_module.module_name, c_module);

    c_module->module_name = cpp_module.module_name.c_str();
    c_module->module_dir = cpp_module.module_dir.c_str();
    c_module->root_dir = cpp_module.root_dir.c_str();
    c_module->artifact_dir = cpp_module.artifact_dir.c_str();
    c_module->builder_dependencies = static_cast<c_module_t**>(std::malloc(sizeof(c_module_t*) * cpp_module.builder_dependencies.size()));
    c_module->builder_dependencies_count = static_cast<unsigned int>(cpp_module.builder_dependencies.size());
    for (size_t i = 0; i < cpp_module.builder_dependencies.size(); ++i) {
        c_module->builder_dependencies[i] = from_cpp_module(*cpp_module.builder_dependencies[i], visited);
    }
    c_module->module_dependencies = static_cast<c_module_t**>(std::malloc(sizeof(c_module_t*) * cpp_module.module_dependencies.size()));
    c_module->module_dependencies_count = static_cast<unsigned int>(cpp_module.module_dependencies.size());
    for (size_t i = 0; i < cpp_module.module_dependencies.size(); ++i) {
        c_module->module_dependencies[i] = from_cpp_module(*cpp_module.module_dependencies[i], visited);
    }
    c_module->state = cpp_module.state;
    c_module->version = cpp_module.version;
    c_module->scc_id = cpp_module.scc_id;
    return c_module;
}

c_module_t* from_cpp_module(const cpp_module_t& cpp_module) {
    std::unordered_map<std::string, c_module_t*> visited;
    return from_cpp_module(cpp_module, visited);
}

void print_graph(const cpp_module_t& module, std::unordered_set<std::string>& visited) {
    if (visited.insert(module.module_name).second == false) {
        return ;
    }

    std::cout << std::format("Module '{}', version {}", module.module_name, module.version) << std::endl;
    for (const auto& dep : module.builder_dependencies) {
        std::cout << std::format("  - B({})", dep->module_name) << std::endl;
    }
    for (const auto& dep : module.module_dependencies) {
        std::cout << std::format("  - R({})", dep->module_name) << std::endl;
    }

    for (const auto& dep : module.builder_dependencies) {
        print_graph(*dep, visited);
    }
    for (const auto& dep : module.module_dependencies) {
        print_graph(*dep, visited);
    }
}

void print_graph(const cpp_module_t& module) {
    std::unordered_set<std::string> visited;
    print_graph(module, visited);
}

uint64_t version_modules(int index, const std::vector<scc_t>& sccs, std::unordered_map<int, uint64_t>& version_by_index) {
    if (version_by_index.find(index) != version_by_index.end()) {
        return version_by_index[index];
    }

    uint64_t max_dep_version = 0;

    for (const auto& dep : sccs[index].deps) {
        max_dep_version = std::max(max_dep_version, version_modules(dep->id, sccs, version_by_index));
    }

    for (const auto& cpp_module : sccs[index].modules) {
        max_dep_version = std::max(max_dep_version, cpp_module->version);
    }

    std::cout << std::format("Current index: {}, max_dep_version: {}", index, max_dep_version) << std::endl;
    for (auto& cpp_module : sccs[index].modules) {
        cpp_module->version = max_dep_version;
    }

    return version_by_index[index] = max_dep_version;
}

void version_modules(const std::vector<scc_t>& sccs) {
    std::unordered_map<int, uint64_t> version_by_index;
    for (int i = 0; i < sccs.size(); ++i) {
        version_modules(i, sccs, version_by_index);
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

        std::unordered_map<std::string, cpp_module_t*> modules_repository;
        discover_dependencies(modules_repository, root_dir, artifacts_dir, module_name);
        auto it = modules_repository.find(module_name);
        if (it == modules_repository.end()) {
            throw std::runtime_error(std::format("module '{}' not found in modules_repository after discovery", module_name));
        }
        cpp_module_t* cpp_module = it->second;

        std::cout << "Print graph before versioning:" << std::endl;
        print_graph(*cpp_module);

        const auto sccs = tarjan(modules_repository);

        version_modules(sccs);

        std::cout << "Print graph after versioning:" << std::endl;
        print_graph(*cpp_module);

        c_module_t* c_module = from_cpp_module(*cpp_module);
        build_builder_artifacts(*cpp_module, c_module, root_dir, artifacts_dir, *cpp_module);
        build_module_artifacts(*cpp_module, c_module, sccs);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
