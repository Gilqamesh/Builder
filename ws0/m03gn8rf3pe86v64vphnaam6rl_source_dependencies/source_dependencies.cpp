#include "source_dependencies.h"

#include <m03gn7qllwpi68ovctow4jrccj_lexer/lexer.h>
#include <m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed/bootstrap_seed.h>

#include <algorithm>
#include <format>
#include <functional>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <vector>

namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies {

namespace bootstrap_seed = m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed;
namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace lexer = m03gn7qllwpi68ovctow4jrccj_lexer;
namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;

using module_list_t = std::vector<workspace_graph::module_t*>;
using source_files_provider_t = std::function<std::vector<filesystem::rooted_path_t>(const workspace_graph::module_t& module)>;

struct module_ptr_less_t {
    bool operator()(const workspace_graph::module_t* a, const workspace_graph::module_t* b) const {
        if (a == b) {
            return false;
        }
        if (a->workspace() == b->workspace()) {
            return a->name() < b->name();
        }
        return a->workspace() < b->workspace();
    }
};

using module_set_t = std::set<workspace_graph::module_t*, module_ptr_less_t>;

static std::optional<filesystem::rooted_path_t> rooted_regular_file(
    const filesystem::path_t& root,
    const std::filesystem::path& include_path
) {
    if (include_path.empty() || include_path.is_absolute()) {
        return std::nullopt;
    }

    try {
        const auto relative_path = filesystem::relative_path_t(include_path);
        const auto path = root / relative_path;

        if (filesystem::exists(path) && filesystem::is_regular_file(path)) {
            return filesystem::rooted_path_t(root, relative_path);
        }
    } catch (const std::exception&) {
    }

    return std::nullopt;
}

static std::optional<filesystem::rooted_path_t> resolve_regular_include(
    const filesystem::rooted_path_t& current_file,
    const std::vector<filesystem::path_t>& source_roots,
    const std::filesystem::path& include_path
) {
    if (const auto path = rooted_regular_file(current_file.path().parent(), include_path)) {
        return path;
    }

    for (const auto& root : source_roots) {
        if (const auto path = rooted_regular_file(root, include_path)) {
            return path;
        }
    }

    return std::nullopt;
}

static workspace_graph::module_t* module_from_include(
    const workspace_graph::module_t& module,
    const std::filesystem::path& include_path,
    dependency_mode_t dependency_mode
) {
    if (include_path.empty() || include_path.is_absolute()) {
        return nullptr;
    }

    const auto path_it = include_path.begin();
    if (path_it == include_path.end()) {
        return nullptr;
    }

    std::optional<workspace_graph::module_name_t> dependency_name;
    try {
        dependency_name = workspace_graph::module_name_t(path_it->string());
    } catch (const std::invalid_argument&) {
        return nullptr;
    }

    if (*dependency_name == module.name()) {
        return nullptr;
    }
    if (!module.workspace().graph().module_names().contains(*dependency_name)) {
        return nullptr;
    }
    auto* dependency = module.workspace().graph().discover_module(*dependency_name);

    switch (dependency_mode) {
        case dependency_mode_t::MODULE:
            if (!(dependency->workspace() <= module.workspace())) {
                throw std::runtime_error(std::format(
                    "m03gn8rf3pe86v64vphnaam6rl_source_dependencies::module_from_include: module (workspace: {}, module: {}) cannot depend on later workspace module (workspace: {}, module: {})",
                    module.workspace(),
                    module,
                    dependency->workspace(),
                    *dependency
                ));
            }
            break ;
        case dependency_mode_t::BUILDER: {
            const bool bootstrap_group_dependency =
                bootstrap_seed::is_module(module)
                && bootstrap_seed::is_module(*dependency);
            if (!(dependency->workspace() < module.workspace()) && !bootstrap_group_dependency) {
                throw std::runtime_error(std::format(
                    "m03gn8rf3pe86v64vphnaam6rl_source_dependencies::module_from_include: builder (workspace: {}, module: {}) cannot depend on same or later workspace module (workspace: {}, module: {})",
                    module.workspace(),
                    module,
                    dependency->workspace(),
                    *dependency
                ));
            }
        } break ;
    }

    return dependency;
}

static std::vector<filesystem::path_t> source_roots(const std::vector<filesystem::rooted_path_t>& source_files) {
    std::vector<filesystem::path_t> result;

    for (const auto& source_file : source_files) {
        const auto already_present = std::find_if(result.begin(), result.end(), [&](const auto& root) {
            return root == source_file.root();
        }) != result.end();

        if (!already_present) {
            result.push_back(source_file.root());
        }
    }

    return result;
}

static module_list_t module_list(const module_set_t& modules) {
    module_list_t result;
    result.reserve(modules.size());

    for (auto* module : modules) {
        result.push_back(module);
    }

    return result;
}

static void collect_direct_dependencies(
    const workspace_graph::module_t& module,
    const filesystem::rooted_path_t& source_file,
    const std::vector<filesystem::path_t>& source_roots,
    const workspace_graph::module_t* excluded_module,
    dependency_mode_t dependency_mode,
    std::set<std::string>& visited_files,
    scan_t& scan,
    module_set_t& dependencies
) {
    const auto path = source_file.path();
    if (!visited_files.insert(path.string()).second) {
        return ;
    }
    scan.local_files.push_back(source_file);

    std::ifstream ifs(path.string());
    if (!ifs.is_open()) {
        throw std::runtime_error(std::format("m03gn8rf3pe86v64vphnaam6rl_source_dependencies::collect_direct_dependencies: failed to open source file '{}'", path));
    }

    for (const auto& include_path : lexer::include_paths(ifs)) {
        if (auto* dependency = module_from_include(module, include_path, dependency_mode)) {
            if (dependency != excluded_module) {
                dependencies.insert(dependency);
            }
            continue;
        }

        if (const auto regular_include = resolve_regular_include(source_file, source_roots, include_path)) {
            collect_direct_dependencies(module, *regular_include, source_roots, excluded_module, dependency_mode, visited_files, scan, dependencies);
        }
    }
}

scan_t scan_sources(
    const workspace_graph::module_t& module,
    const std::vector<filesystem::rooted_path_t>& source_files,
    const workspace_graph::module_t* excluded_module,
    dependency_mode_t dependency_mode
) {
    scan_t result;
    module_set_t dependencies;
    std::set<std::string> visited_files;
    const auto roots = source_roots(source_files);

    for (const auto& source_file : source_files) {
        collect_direct_dependencies(module, source_file, roots, excluded_module, dependency_mode, visited_files, result, dependencies);
    }
    result.dependencies = module_list(dependencies);

    return result;
}

static bool is_library_source_file(const filesystem::rooted_path_t& source_file) {
    const auto path = source_file.path();
    const auto relative_path = source_file.relative_path().string();

    if (path.filename() == workspace_graph::BUILDER_CPP || path.filename() == workspace_graph::CLI_CPP) {
        return false;
    }
    if (relative_path.starts_with("cli/") || relative_path.starts_with("test/")) {
        return false;
    }

    const auto extension = source_file.relative_path().extension();
    return extension == ".c" || extension == ".cpp" || extension == ".h" || extension == ".hpp";
}

std::vector<filesystem::rooted_path_t> library_source_files(const filesystem::path_t& source_root) {
    const auto source_files = filesystem::find(
        source_root,
        filesystem::find_include_predicate_t::is_regular,
        filesystem::find_descend_predicate_t::descend_all
    );

    std::vector<filesystem::rooted_path_t> result;
    result.reserve(source_files.size());
    for (const auto& source_file : source_files) {
        if (is_library_source_file(source_file)) {
            result.push_back(source_file);
        }
    }

    return result;
}

static void collect_library_dependency_closure(
    module_list_t& result,
    module_set_t& visited,
    workspace_graph::module_t* module,
    const workspace_graph::module_t* excluded_module,
    const source_files_provider_t& source_files_for_module
) {
    if (module == excluded_module || !visited.insert(module).second) {
        return ;
    }

    result.push_back(module);

    const auto source_files = source_files_for_module(*module);
    const auto dependencies = scan_sources(
        *module,
        source_files,
        excluded_module,
        dependency_mode_t::MODULE
    ).dependencies;

    for (auto* dependency : dependencies) {
        collect_library_dependency_closure(result, visited, dependency, excluded_module, source_files_for_module);
    }
}

module_list_t dependency_modules(
    const workspace_graph::module_t& module,
    const std::vector<filesystem::rooted_path_t>& source_files,
    bool include_owner,
    dependency_mode_t dependency_mode,
    const source_files_provider_t& source_files_for_module
) {
    module_list_t result;
    module_set_t visited;
    const auto excluded_module = include_owner ? nullptr : &module;
    const auto direct_dependencies = scan_sources(module, source_files, excluded_module, dependency_mode).dependencies;

    if (include_owner) {
        collect_library_dependency_closure(result, visited, const_cast<workspace_graph::module_t*>(&module), excluded_module, source_files_for_module);
    }

    for (auto* dependency : direct_dependencies) {
        collect_library_dependency_closure(result, visited, dependency, excluded_module, source_files_for_module);
    }

    return result;
}

std::vector<workspace_graph::module_name_t> module_names(const module_list_t& modules) {
    std::vector<workspace_graph::module_name_t> result;
    result.reserve(modules.size());

    for (const auto* module : modules) {
        result.push_back(module->name());
    }

    return result;
}

} // namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies
