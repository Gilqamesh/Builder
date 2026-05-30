#include "compiler.h"

#include "process.h"

#include <format>
#include <stdexcept>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>

namespace kernel {

namespace cpp_builder {

namespace compiler {

static std::vector<filesystem::path_t> resolve_source_files(
    const builder::output_t& source_output,
    const std::vector<filesystem::relative_path_t>& relative_source_files
) {
    std::vector<filesystem::path_t> source_files;
    source_files.reserve(relative_source_files.size());

    for (const auto& relative_source_file : relative_source_files) {
        const auto source_file = source_output.root / relative_source_file;
        bool materialized = false;

        for (const auto& source : source_output.artifacts) {
            if (source.path == source_file) {
                materialized = true;
                break ;
            }
        }

        if (!materialized) {
            throw std::runtime_error(std::format("cpp_compiler::resolve_source_files: source file '{}' was not materialized by source phase", source_file));
        }

        source_files.push_back(source_file);
    }

    return source_files;
}

static std::vector<filesystem::path_t> create_object_files(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<builder::output_t>& interface_outputs,
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
    process_prefix_args.push_back("-g");

    for (const auto& [key, value] : define_key_values) {
        process_prefix_args.push_back(std::format("-D{}={}", key, value));
    }

    for (const auto& interface_output : interface_outputs) {
        process_prefix_args.push_back(std::format("-I{}", interface_output.root));
    }

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

        std::vector<process::process_arg_t> process_args;
        if (source_file.extension() == ".c") {
            process_args.push_back(KERNEL_CPP_BUILDER_CC_COMPILER_PATH);
        } else {
            process_args.push_back(KERNEL_CPP_BUILDER_CXX_COMPILER_PATH);
            process_args.push_back("-std=c++23");
        }
        process_args.insert(process_args.end(), process_prefix_args.begin(), process_prefix_args.end());
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
    const std::vector<builder::output_t>& interface_outputs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::path_t& static_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        interface_outputs,
        {},
        source_files,
        define_key_values,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!filesystem::exists(static_library_dir)) {
        filesystem::create_directories(static_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(KERNEL_CPP_BUILDER_AR_PATH);
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
    const std::vector<builder::output_t>& interface_outputs,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const builder::link_inputs_t& link_inputs,
    const std::vector<filesystem::path_t>& external_libraries,
    const filesystem::path_t& shared_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        interface_outputs,
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
    process_args.push_back(KERNEL_CPP_BUILDER_CXX_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-shared");
    process_args.push_back("-o");
    process_args.push_back(shared_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    for (const auto& group : link_inputs.groups) {
        if (group.static_library_group) {
            process_args.push_back("-Wl,--start-group");
        }
        for (const auto& library : group.libraries) {
            if (!filesystem::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler::create_shared_library: library does not exist '{}'", library));
            }

            process_args.push_back(library);
        }
        if (group.static_library_group) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    for (const auto& library : external_libraries) {
        if (!filesystem::exists(library)) {
            throw std::runtime_error(std::format("cpp_compiler::create_shared_library: library does not exist '{}'", library));
        }

        process_args.push_back(library);
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
    const std::vector<builder::output_t>& interface_outputs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const builder::link_inputs_t& link_inputs,
    const filesystem::path_t& binary
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        interface_outputs,
        {},
        source_files,
        define_key_values,
        true
    );

    const auto binary_dir = binary.parent();
    if (!filesystem::exists(binary_dir)) {
        filesystem::create_directories(binary_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(KERNEL_CPP_BUILDER_CXX_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-std=c++23");
    process_args.push_back("-o");
    process_args.push_back(binary);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    for (const auto& group : link_inputs.groups) {
        if (group.static_library_group) {
            process_args.push_back("-Wl,--start-group");
        }
        for (const auto& library : group.libraries) {
            if (!filesystem::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler::create_binary: library does not exist '{}'", library));
            }
            process_args.push_back(library);
        }
        if (group.static_library_group) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    for (const auto& group : link_inputs.groups) {
        for (const auto& library : group.libraries) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library.parent()));
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
    const auto source_output = phase.materialize<builder::source_phase_t>();
    const auto interface_outputs = phase.materialize_all<builder::interface_phase_t>();

    return create_static_library_impl(
        phase.build_dir(),
        source_output.root,
        interface_outputs,
        resolve_source_files(source_output, relative_source_files),
        define_key_values,
        phase.build_dir() / relative_output_path
    );
}

filesystem::path_t create_shared_library(
    const builder::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& external_libraries,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = phase.materialize<builder::source_phase_t>();
    const auto interface_outputs = phase.materialize_all<builder::interface_phase_t>();
    const auto link_inputs = phase.link_inputs();

    return create_shared_library_impl(
        phase.build_dir(),
        source_output.root,
        interface_outputs,
        {},
        resolve_source_files(source_output, relative_source_files),
        define_key_values,
        link_inputs,
        external_libraries,
        phase.build_dir() / relative_output_path
    );
}

filesystem::path_t create_binary(
    const builder::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = phase.materialize<builder::source_phase_t>();
    const auto interface_outputs = phase.materialize_all<builder::interface_phase_t>();
    const auto link_inputs = phase.link_inputs();

    return create_binary_impl(
        phase.build_dir(),
        source_output.root,
        interface_outputs,
        resolve_source_files(source_output, relative_source_files),
        define_key_values,
        link_inputs,
        phase.build_dir() / relative_output_path
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
        {},
        include_dirs,
        { builder_source_file },
        define_key_values,
        builder::link_inputs_t {},
        libraries,
        builder_plugin
    );
}

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel
