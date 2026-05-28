#include "compiler.h"

#include "module_builder.h"
#include "process.h"

#include <format>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>
#include <unordered_set>

namespace kernel {

namespace cpp_builder {

namespace compiler {

static std::vector<filesystem::path_t> resolve_source_files(
    const filesystem::path_t& source_root,
    const std::vector<filesystem::relative_path_t>& relative_source_files
) {
    std::vector<filesystem::path_t> source_files;
    source_files.reserve(relative_source_files.size());

    for (const auto& relative_source_file : relative_source_files) {
        source_files.push_back(source_root / relative_source_file);
    }

    return source_files;
}

static void append_module_dependency_interfaces(
    builder::library_type_t library_type,
    graph::module_t& module,
    std::unordered_set<const graph::module_t*>& visited,
    std::vector<filesystem::path_t>& include_dirs
) {
    for (auto* dependency : module.dependencies) {
        if (!visited.insert(dependency).second) {
            continue ;
        }

        builder::module_builder_t dependency_builder(*dependency->workspace->workspace_ecosystem, *dependency);
        builder::phase_chain_t dependency_phase_chain(dependency_builder, *dependency, library_type);
        const auto& dependency_interfaces = dependency_phase_chain.interface.materialize<builder::interface_phase_t>();
        include_dirs.insert(include_dirs.end(), dependency_interfaces.interfaces.begin(), dependency_interfaces.interfaces.end());

        append_module_dependency_interfaces(library_type, *dependency, visited, include_dirs);
    }
}

static std::vector<filesystem::path_t> library_include_dirs(const builder::library_phase_t& phase) {
    const auto& interface_output = phase.materialize<builder::interface_phase_t>();
    auto include_dirs = interface_output.interfaces;

    std::unordered_set<const graph::module_t*> visited;
    append_module_dependency_interfaces(phase.library_type, phase.module(), visited, include_dirs);

    return include_dirs;
}

static std::vector<filesystem::path_t> binary_include_dirs(const builder::binary_phase_t& phase) {
    const auto& interface_output = phase.materialize<builder::interface_phase_t>();
    auto include_dirs = interface_output.interfaces;

    std::unordered_set<const graph::module_t*> visited;
    append_module_dependency_interfaces(phase.library_type, phase.module(), visited, include_dirs);

    return include_dirs;
}

static std::vector<filesystem::path_t> create_object_files(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool is_position_independent
) {
    std::vector<filesystem::path_t> result;
    result.reserve(source_files.size());

    if (!filesystem::exists(build_dir)) {
        filesystem::create_directories(build_dir);
    }

    std::vector<process::process_arg_t> process_prefix_args;
    process_prefix_args.push_back(CXX_COMPILER_PATH);
    process_prefix_args.push_back("-g");
    process_prefix_args.push_back("-std=c++23");

    std::string define_flags_str;
    for (const auto& [key, value] : define_key_values) {
        process_prefix_args.push_back(std::format("-D{}={}", key, value));
    }

    std::string include_dirs_str;
    for (const auto& include_dir : include_dirs) {
        process_prefix_args.push_back(std::format("-I{}", include_dir));
    }

    for (const auto& source_file : source_files) {
        if (!filesystem::exists(source_file)) {
            throw std::runtime_error(std::format("cpp_compiler::create_object_files: source file does not exist '{}'", source_file));
        }

        const auto rel = source_dir.relative(source_file);
        auto object_file = build_dir / rel;
        object_file.extension(".o");

        const auto object_file_dir = object_file.parent();
        if (!filesystem::exists(object_file_dir)) {
            filesystem::create_directories(object_file_dir);
        }

        std::vector<process::process_arg_t> process_args = process_prefix_args;
        if (is_position_independent) {
            process_args.push_back("-fPIC");
        }
        process_args.push_back("-c");
        process_args.push_back(source_file);
        process_args.push_back("-o");
        process_args.push_back(object_file);

        const int process_result = process::create_and_wait(process_args);
        if (process_result != 0) {
            throw std::runtime_error(std::format("cpp_compiler::create_object_files: failed to compile '{}', compilation result: {}", source_file, process_result));
        }

        result.push_back(object_file);
    }

    return result;
}

static filesystem::path_t create_static_library_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::path_t& static_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!filesystem::exists(static_library_dir)) {
        filesystem::create_directories(static_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(AR_PATH);
    process_args.push_back("rcs");
    process_args.push_back(static_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_static_library: failed to create static library '{}', command exited with code {}", static_library, process_result));
    }

    if (!filesystem::exists(static_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_static_library: expected output static library '{}' to exist but it does not", static_library));
    }

    return static_library;
}

static filesystem::path_t create_shared_library_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::path_t& shared_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        true
    );

    const auto shared_library_dir = shared_library.parent();
    if (!filesystem::exists(shared_library_dir)) {
        filesystem::create_directories(shared_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(CXX_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-shared");
    process_args.push_back("-o");
    process_args.push_back(shared_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    for (const auto& dso : dsos) {
        if (!filesystem::exists(dso)) {
            throw std::runtime_error(std::format("cpp_compiler::create_shared_library: dso does not exist '{}'", dso));
        }

        process_args.push_back(dso);
    }

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_shared_library: failed to create shared library '{}', command exited with code {}", shared_library, process_result));
    }

    if (!filesystem::exists(shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_shared_library: expected output shared library '{}' to exist but it does not", shared_library));
    }

    return shared_library;
}

static filesystem::path_t create_binary_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::path_t& binary
) {
    const auto object_files = create_object_files(build_dir, source_dir, include_dirs, source_files, define_key_values, TEMP_assume_all_link_inputs_are_shared);

    const auto binary_dir = binary.parent();
    if (!filesystem::exists(binary_dir)) {
        filesystem::create_directories(binary_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(CXX_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-std=c++23");
    process_args.push_back("-o");
    process_args.push_back(binary);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    // TODO: mixed static/shared interleave of start/end group, typify 'library_groups'
    for (auto library_group_it = library_groups.rbegin(); library_group_it != library_groups.rend(); ++library_group_it) {
        if (!TEMP_assume_all_link_inputs_are_shared && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--start-group");
        }
        for (const auto& library : *library_group_it) {
            if (!filesystem::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler::create_binary: library does not exist '{}'", library));
            }
            process_args.push_back(library);
        }
        if (!TEMP_assume_all_link_inputs_are_shared && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    for (const auto& library_group : library_groups) {
        for (const auto& library_group : library_group) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library_group.parent()));
        }
    }
    
    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: failed to create binary '{}', command exited with code {}", binary, process_result));
    }

    if (!filesystem::exists(binary)) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

filesystem::path_t create_static_library(
    const builder::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto& source_output = phase.materialize<builder::source_phase_t>();
    const auto include_dirs = library_include_dirs(phase);

    return create_static_library_impl(
        phase.build_dir(),
        source_output.source_root,
        include_dirs,
        resolve_source_files(source_output.source_root, relative_source_files),
        define_key_values,
        phase.install_dir() / relative_output_path
    );
}

filesystem::path_t create_shared_library(
    const builder::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto& source_output = phase.materialize<builder::source_phase_t>();
    const auto include_dirs = library_include_dirs(phase);

    return create_shared_library_impl(
        phase.build_dir(),
        source_output.source_root,
        include_dirs,
        resolve_source_files(source_output.source_root, relative_source_files),
        define_key_values,
        dsos,
        phase.install_dir() / relative_output_path
    );
}

filesystem::path_t create_binary(
    const builder::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto& source_output = phase.materialize<builder::source_phase_t>();
    const auto include_dirs = binary_include_dirs(phase);

    return create_binary_impl(
        phase.build_dir(),
        source_output.source_root,
        include_dirs,
        resolve_source_files(source_output.source_root, relative_source_files),
        define_key_values,
        library_groups,
        TEMP_assume_all_link_inputs_are_shared,
        phase.install_dir() / relative_output_path
    );
}

filesystem::path_t create_builder_shared_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const filesystem::path_t& builder_source_file,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& libraries,
    const filesystem::path_t& builder_plugin
) {
    return create_shared_library_impl(
        build_dir,
        source_dir,
        include_dirs,
        { builder_source_file },
        define_key_values,
        libraries,
        builder_plugin
    );
}

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel
