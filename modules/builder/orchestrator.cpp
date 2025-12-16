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

void populate_dependencies(cpp_module_t& cpp_module, const std::filesystem::path& modules_dir, std::unordered_map<std::string, cpp_module_t*>& modules_repository, std::stack<std::string>& visiting_stack) {
    switch (cpp_module.state) {
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
    cpp_module.state = VISITING;

    if (!std::filesystem::exists(modules_dir)) {
        throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
    }

    const auto module_dir = modules_dir / cpp_module.module_name;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

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

        if (cpp_module.module_name == BUILDER && builder_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(builder_dep_str);
        if (it == modules_repository.end()) {
            it = modules_repository.emplace(builder_dep_str, new cpp_module_t{
                .module_name = builder_dep_str,
                .module_dir = modules_dir / builder_dep_str,
                .root_dir = cpp_module.root_dir,
                .state = NOT_VISITED
            }).first;
        }

        if (std::find(cpp_module.builder_dependencies.begin(), cpp_module.builder_dependencies.end(), it->second) != cpp_module.builder_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), builder_dep_str, builder_deps_key));
        }

        cpp_module.builder_dependencies.push_back(it->second);

        visiting_stack.push(builder_dep_str);
        populate_dependencies(*it->second, modules_dir, modules_repository, visiting_stack);
        visiting_stack.pop();
    }

    cpp_module.state = VISITED;

    for (const auto& runtime_dep : deps_json[module_deps_key]) {
        if (!runtime_dep.is_string()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_deps_key));
        }
        const std::string runtime_dep_str = runtime_dep.get<std::string>();
        if (runtime_dep_str.empty()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_deps_key));
        }

        if (cpp_module.module_name == BUILDER && runtime_dep_str == BUILDER) {
            continue ;
        }

        auto it = modules_repository.find(runtime_dep_str);
        if (it == modules_repository.end()) {
            it = modules_repository.emplace(runtime_dep_str, new cpp_module_t{
                .module_name = runtime_dep_str,
                .module_dir = modules_dir / runtime_dep_str,
                .root_dir = cpp_module.root_dir,
                .state = NOT_VISITED
            }).first;
        }

        if (std::find(cpp_module.module_dependencies.begin(), cpp_module.module_dependencies.end(), it->second) != cpp_module.module_dependencies.end()) {
            throw std::runtime_error(std::format("invalid deps json file '{}': duplicate entry '{}' in '{}' array", deps_json_path.string(), runtime_dep_str, module_deps_key));
        }

        cpp_module.module_dependencies.push_back(it->second);

        populate_dependencies(*it->second, modules_dir, modules_repository, visiting_stack);
    }
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

void populate_dependencies(cpp_module_t& cpp_module, const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir) {
    std::unordered_map<std::string, cpp_module_t*> modules_repository;
    std::stack<std::string> visiting_stack;

    const auto modules_dir = root_dir / MODULES_DIR;

    for (const auto& entry : std::filesystem::directory_iterator(modules_dir)) {
        if (entry.is_directory()) {
            const auto entry_path = entry.path();
            const auto module_name = entry_path.filename().string();
            const auto module_version = get_module_version(module_name, root_dir);
            const auto artifact_dir = artifacts_dir / module_name / (module_name + "@" + std::to_string(module_version));

            modules_repository.emplace(module_name, new cpp_module_t{
                .module_name = module_name,
                .module_dir = entry_path,
                .root_dir = root_dir,
                .artifact_dir = artifact_dir,
                .state = NOT_VISITED,
                .version = module_version
            });
        }
    }

    visiting_stack.push(cpp_module.module_name);
    populate_dependencies(cpp_module, modules_dir, modules_repository, visiting_stack);
    visiting_stack.pop();
}

void build_builder_artifacts(cpp_module_t& cpp_module, c_module_t* c_module, const std::filesystem::path& root_dir, const std::filesystem::path& artifacts_dir) {
    if (BUILT <= cpp_module.state) {
        return ;
    }

    for (size_t i = 0; i < cpp_module.builder_dependencies.size(); ++i) {
        build_builder_artifacts(*cpp_module.builder_dependencies[i], c_module->builder_dependencies[i], root_dir, artifacts_dir);
    }

    if (!std::filesystem::exists(cpp_module.artifact_dir)) {
        if (!std::filesystem::create_directories(cpp_module.artifact_dir)) {
            throw std::runtime_error(std::format("failed to create artifact directory '{}'", cpp_module.artifact_dir.string()));
        }
    }

    std::cout << std::format("Build builder artifacts for module '{}' version {}", cpp_module.module_name, cpp_module) << std::endl;

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

            c_module__build_builder_artifacts_t c_module__build_builder_artifacts = (c_module__build_builder_artifacts_t)dlsym(builder_handle, BUILDER_PLUGIN_SYMBOL);
            if (!c_module__build_builder_artifacts) {
                const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_PLUGIN_SYMBOL, builder_so.string(), dlerror());
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
        build_builder_artifacts(*cpp_module.module_dependencies[i], c_module->module_dependencies[i], root_dir, artifacts_dir);
    }
}

void build_module_artifacts(const cpp_module_t& cpp_module, c_module_t* c_module) {
    std::cout << std::format("Build module artifacts for module '{}', version {}", cpp_module.module_name, get_module_version(cpp_module.module_name, cpp_module.root_dir)) << std::endl;

    if (cpp_module.module_name == BUILDER) {
        c_module__build_module_artifacts(c_module);
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

        c_module__build_module_artifacts_t c_module__build_module_artifacts = (c_module__build_module_artifacts_t)dlsym(builder_handle, BUILDER_PLUGIN_SYMBOL);
        if (!c_module__build_module_artifacts) {
            const auto message = std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_PLUGIN_SYMBOL, builder_so.string(), dlerror());
            dlclose(builder_handle);
            throw std::runtime_error(message);
        }

        try {
            c_module__build_module_artifacts(c_module);
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

    for (const auto& dep : module.builder_dependencies) {
        std::cout << module.module_name << ": L(" << dep->module_name << ")" << std::endl;
        print_graph(*dep, visited);
    }
    for (const auto& dep : module.module_dependencies) {
        std::cout << module.module_name << ": R(" << dep->module_name << ")" << std::endl;
        print_graph(*dep, visited);
    }
}

void print_graph(const cpp_module_t& module) {
    std::unordered_set<std::string> visited;
    print_graph(module, visited);
}

int main(int argc, char **argv) {
    try {
        if (argc < 4) {
            throw std::runtime_error(std::format("usage: {} <root_dir> <artifacts_dir> <module_name>", argv[0]));
        }

        const auto root_dir = std::filesystem::path(argv[1]);
        const auto artifacts_dir = std::filesystem::path(argv[2]);
        const auto module_name = std::string(argv[3]);

        cpp_module_t cpp_module = {
            .module_name = argv[3],
            .module_dir = root_dir / MODULES_DIR / argv[3],
            .root_dir = root_dir,
            .artifact_dir = artifacts_dir / argv[3]
        };

        if (!std::filesystem::exists(cpp_module.module_dir)) {
            throw std::runtime_error(std::format("module directory does not exist '{}'", cpp_module.module_dir.string()));
        }

        const auto builder_module_version = get_module_version(BUILDER, root_dir);
        // TODO: running old binaries always upgrades to a newer version, so either fork from the binary version's source code (git integration) or run the latest versioned binary
        if (VERSION < builder_module_version) {
            relaunch_newer_version(root_dir, artifacts_dir, module_name, builder_module_version);
        }


        populate_dependencies(cpp_module, root_dir, artifacts_dir);

        print_graph(cpp_module);

        c_module_t* c_module = from_cpp_module(cpp_module);
        build_builder_artifacts(cpp_module, c_module, root_dir, artifacts_dir);
        build_module_artifacts(cpp_module, c_module);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
