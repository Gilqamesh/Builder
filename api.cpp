#include "api.h"

#include <format>
#include <stdexcept>
#include <type_traits>

namespace kernel {

static std::vector<phase::output_artifact_t> artifacts(const phase::binary_phase_t::output_t& output) {
    std::vector<phase::output_artifact_t> result;
    result.reserve(1 + output.artifacts.size());
    result.push_back(output.cli);
    result.insert(result.end(), output.artifacts.begin(), output.artifacts.end());
    return result;
}

static std::vector<filesystem::path_t> find_artifacts(
    const std::vector<phase::output_artifact_t>& artifacts,
    const filesystem::find_include_predicate_t& include_predicate
) {
    std::vector<filesystem::path_t> result;

    for (const auto& artifact : artifacts) {
        if (include_predicate(artifact.path)) {
            result.push_back(artifact.path);
        }
    }

    return result;
}

static phase::output_artifact_t find_artifact(
    const std::vector<phase::output_artifact_t>& artifacts,
    const filesystem::relative_path_t& relative_path
) {
    const phase::output_artifact_t* result = nullptr;

    for (const auto& artifact : artifacts) {
        if (artifact.relative_path == relative_path) {
            if (result != nullptr) {
                throw std::runtime_error(std::format("kernel::find: output has more than one artifact at relative path '{}'", relative_path));
            }

            result = &artifact;
        }
    }

    if (result == nullptr) {
        throw std::runtime_error(std::format("kernel::find: output has no artifact at relative path '{}'", relative_path));
    }

    return *result;
}

std::vector<filesystem::path_t> find(
    const phase::output_artifacts_t& output,
    const filesystem::find_include_predicate_t& include_predicate
) {
    return find_artifacts(output.artifacts, include_predicate);
}

std::vector<filesystem::path_t> find(
    const phase::binary_phase_t::output_t& output,
    const filesystem::find_include_predicate_t& include_predicate
) {
    return find_artifacts(artifacts(output), include_predicate);
}

phase::output_artifact_t find(
    const phase::output_artifacts_t& output,
    const filesystem::relative_path_t& relative_path
) {
    return find_artifact(output.artifacts, relative_path);
}

phase::output_artifact_t find(
    const phase::binary_phase_t::output_t& output,
    const filesystem::relative_path_t& relative_path
) {
    return find_artifact(artifacts(output), relative_path);
}

static std::vector<filesystem::path_t> resolve_source_files(
    const phase::source_phase_t::output_t& source_output,
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
    const std::vector<phase::interface_phase_t::output_t>& interface_outputs
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

filesystem::path_t create_library(
    const phase::library_phase_t& library_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = library_phase.build<phase::source_phase_t>();
    const auto interface_outputs = library_phase.build_closure<phase::interface_phase_t>();

    return compiler::create_library(
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

static phase::output_artifact_t default_library_artifact(
    const phase::library_phase_t& library_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
) {
    const auto stem = library_phase.module().name().stem();
    if (stem.empty()) {
        throw std::runtime_error("kernel::default_library: module name stem must not be empty");
    }

    const auto relative_output_path = [&]() {
        switch (library_phase.module_config().library_type) {
            case module_config::library_type_t::STATIC:
                return filesystem::relative_path_t(std::format("lib{}.a", stem));
            case module_config::library_type_t::SHARED:
                return filesystem::relative_path_t(std::format("lib{}.so", stem));
            default:
                throw std::runtime_error(std::format("kernel::default_library: unknown library_type {}", static_cast<std::underlying_type_t<module_config::library_type_t>>(library_phase.module_config().library_type)));
        }
    }();

    return phase::output_artifact_t {
        .path = create_library(
            library_phase,
            relative_source_files,
            defines,
            relative_output_path
        ),
        .relative_path = relative_output_path
    };
}

static compiler::link_inputs_t binary_link_inputs(
    const graph::module_t& module,
    module_config::module_config_t module_config
) {
    bool static_libraries = false;
    switch (module_config.library_type) {
        case module_config::library_type_t::STATIC: static_libraries = true; break ;
        case module_config::library_type_t::SHARED: break ;
        default: throw std::runtime_error(std::format("kernel::binary_link_inputs: unknown library_type {}", static_cast<std::underlying_type_t<module_config::library_type_t>>(module_config.library_type)));
    }

    const auto closure_groups = module.closure_groups();

    compiler::link_inputs_t result;
    for (auto group_it = closure_groups.rbegin(); group_it != closure_groups.rend(); ++group_it) {
        compiler::link_input_group_t group {
            .libraries = {},
            .static_library_group = false
        };

        for (auto module_it = group_it->rbegin(); module_it != group_it->rend(); ++module_it) {
            phase::output_artifacts_t output {
                .root = filesystem::path_t("/"),
                .artifacts = {}
            };
            const phase::library_phase_t library_phase(**module_it, module_config, output);
            const auto library_output = library_phase.build<phase::library_phase_t>();
            for (const auto& library : library_output.artifacts) {
                group.libraries.push_back(library.path);
            }
        }

        if (!group.libraries.empty()) {
            group.static_library_group = static_libraries && 1 < group.libraries.size();
            result.groups.push_back(group);
        }
    }

    return result;
}

filesystem::path_t create_binary(
    const phase::binary_phase_t& binary_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines,
    const filesystem::relative_path_t& relative_output_path
) {
    const auto source_output = binary_phase.build<phase::source_phase_t>();
    const auto interface_outputs = binary_phase.build_closure<phase::interface_phase_t>();
    const auto link_inputs = binary_link_inputs(
        binary_phase.module(),
        binary_phase.module_config()
    );

    return compiler::create_binary(
        binary_phase.build_dir(),
        source_output.root,
        include_dirs_from_outputs(interface_outputs),
        resolve_source_files(source_output, relative_source_files),
        defines,
        link_inputs,
        binary_phase.build_dir() / relative_output_path
    );
}

static filesystem::path_t default_cli_artifact(
    const phase::binary_phase_t& binary_phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
) {
    return create_binary(
        binary_phase,
        relative_source_files,
        defines,
        filesystem::relative_path_t("cli")
    );
}

phase::default_library_t default_library(
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
) {
    return phase::default_library_t {
        .relative_source_files = relative_source_files,
        .defines = defines
    };
}

phase::default_cli_t default_cli(
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
) {
    return phase::default_cli_t {
        .relative_source_files = relative_source_files,
        .defines = defines
    };
}

namespace phase {

void library_phase_t::add_library(const default_library_t& request) const {
    add_library(default_library_artifact(
        *this,
        request.relative_source_files,
        request.defines
    ));
}

void binary_phase_t::add_cli(const default_cli_t& request) const {
    add_cli(default_cli_artifact(
        *this,
        request.relative_source_files,
        request.defines
    ));
}

} // namespace phase

} // namespace kernel
