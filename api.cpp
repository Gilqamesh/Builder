#include "api.h"

#include "linker.h"

#include <format>
#include <stdexcept>
#include <type_traits>

namespace kernel {
    
static std::vector<filesystem::path_t> resolve_source_files(
    const phase::output_t& source_output,
    const std::vector<filesystem::relative_path_t>& relative_source_files
) {
    std::vector<filesystem::path_t> source_files;
    source_files.reserve(relative_source_files.size());

    for (const auto& relative_source_file : relative_source_files) {
        const auto source_file = source_output.root / relative_source_file;
        bool built = false;

        for (const auto& source : source_output.artifacts) {
            if (source.path == source_file) {
                built = true;
                break ;
            }
        }

        if (!built) {
            throw std::runtime_error(std::format("kernel::resolve_source_files: source file '{}' was not built by source phase", source_file));
        }

        source_files.push_back(source_file);
    }

    return source_files;
}

static std::vector<filesystem::path_t> include_dirs_from_outputs(
    const std::vector<phase::output_t>& interface_outputs
) {
    std::vector<filesystem::path_t> include_dirs;
    include_dirs.reserve(interface_outputs.size());

    for (const auto& interface_output : interface_outputs) {
        include_dirs.push_back(interface_output.root);
    }

    return include_dirs;
}

static compiler::library_type_t compiler_library_type(module_config::library_type_t library_type) {
    switch (library_type) {
        case module_config::library_type_t::STATIC: return compiler::library_type_t::STATIC;
        case module_config::library_type_t::SHARED: return compiler::library_type_t::SHARED;
        default:
            throw std::runtime_error(std::format("kernel::compiler_library_type: unknown library_type {}", static_cast<std::underlying_type_t<module_config::library_type_t>>(library_type)));
    }
}

static compiler::link_inputs_t compiler_link_inputs(
    const kernel::linker::link_inputs_t& link_inputs
) {
    compiler::link_inputs_t result;
    result.groups.reserve(link_inputs.groups.size());

    for (const auto& group : link_inputs.groups) {
        result.groups.push_back(compiler::link_input_group_t {
            .libraries = group.libraries,
            .static_library_group = group.static_library_group
        });
    }

    return result;
}

namespace compiler {

filesystem::path_t create_library(
    const phase::library_phase_t& library_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<define_t>& defines,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = library_phase.build<phase::source_phase_t>();
    const auto interface_outputs = library_phase.build_closure<phase::interface_phase_t>();

    return create_library(
        library_phase.build_dir(),
        source_output.root,
        include_dirs_from_outputs(interface_outputs),
        resolve_source_files(source_output, relative_source_files),
        defines,
        compiler_library_type(library_phase.module_config().library_type),
        compiler::link_inputs_t {},
        library_phase.build_dir() / relative_output_path
    );
}

filesystem::path_t create_binary(
    const phase::binary_phase_t& binary_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<define_t>& defines,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = binary_phase.build<phase::source_phase_t>();
    const auto interface_outputs = binary_phase.build_closure<phase::interface_phase_t>();
    const auto link_inputs = kernel::linker::binary_link_inputs(
        binary_phase.module(),
        binary_phase.module_config()
    );

    return create_binary(
        binary_phase.build_dir(),
        source_output.root,
        include_dirs_from_outputs(interface_outputs),
        resolve_source_files(source_output, relative_source_files),
        defines,
        compiler_link_inputs(link_inputs),
        binary_phase.build_dir() / relative_output_path
    );
}

} // namespace compiler

} // namespace kernel
