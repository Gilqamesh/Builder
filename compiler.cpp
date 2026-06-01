#include "compiler.h"

#include "process.h"

#include <format>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace kernel {

namespace compiler {

define_t::define_t(std::string key, std::string value):
    m_key(std::move(key)),
    m_replacement("\"")
{
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            m_replacement.push_back('\\');
        }
        m_replacement.push_back(c);
    }
    m_replacement.push_back('"');
}

const std::string& define_t::key() const {
    return m_key;
}

const std::string& define_t::replacement() const {
    return m_replacement;
}

static std::vector<filesystem::path_t> create_object_files(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    bool is_position_independent
) {
    std::vector<filesystem::path_t> result;
    result.reserve(source_files.size());

    if (!filesystem::exists(build_dir)) {
        filesystem::create_directories(build_dir);
    }

    std::vector<process::process_arg_t> process_prefix_args;
    process_prefix_args.push_back("-g");

    for (const auto& define : defines) {
        process_prefix_args.push_back(std::format("-D{}={}", define.key(), define.replacement()));
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
            process_args.push_back(KERNEL_CC_COMPILER_PATH);
        } else {
            process_args.push_back(KERNEL_CXX_COMPILER_PATH);
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

static void append_runtime_library_paths(
    std::vector<process::process_arg_t>& process_args,
    const link_inputs_t& link_inputs
) {
    for (const auto& group : link_inputs.groups) {
        for (const auto& library : group.libraries) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library.parent()));
        }
    }
}

static filesystem::path_t create_archive_library_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    const filesystem::path_t& static_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        defines,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!filesystem::exists(static_library_dir)) {
        filesystem::create_directories(static_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(KERNEL_AR_PATH);
    process_args.push_back("rcs");
    process_args.push_back(static_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_library: failed to create static library '{}', command exited with code {}", static_library, process_result));
    }

    if (!filesystem::exists(static_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_library: expected output static library '{}' to exist but it does not", static_library));
    }

    return static_library;
}

static filesystem::path_t create_dynamic_library_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& shared_library
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        defines,
        true
    );

    const auto shared_library_dir = shared_library.parent();
    if (!filesystem::exists(shared_library_dir)) {
        filesystem::create_directories(shared_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(KERNEL_CXX_COMPILER_PATH);
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
                throw std::runtime_error(std::format("cpp_compiler::create_library: library does not exist '{}'", library));
            }

            process_args.push_back(library);
        }
        if (group.static_library_group) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    append_runtime_library_paths(process_args, link_inputs);

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_library: failed to create shared library '{}', command exited with code {}", shared_library, process_result));
    }

    if (!filesystem::exists(shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_library: expected output shared library '{}' to exist but it does not", shared_library));
    }

    return shared_library;
}

static filesystem::path_t create_binary_impl(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& binary
) {
    const auto object_files = create_object_files(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        defines,
        true
    );

    const auto binary_dir = binary.parent();
    if (!filesystem::exists(binary_dir)) {
        filesystem::create_directories(binary_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(KERNEL_CXX_COMPILER_PATH);
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

    append_runtime_library_paths(process_args, link_inputs);
    
    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: failed to create binary '{}', command exited with code {}", binary, process_result));
    }

    if (!filesystem::exists(binary)) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

filesystem::path_t create_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    library_type_t library_type,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& output_path
) {
    switch (library_type) {
        case library_type_t::STATIC:
            if (!link_inputs.groups.empty()) {
                throw std::runtime_error("cpp_compiler::create_library: static libraries do not support link inputs");
            }

            return create_archive_library_impl(
                build_dir,
                source_dir,
                include_dirs,
                source_files,
                defines,
                output_path
            );
        case library_type_t::SHARED:
            return create_dynamic_library_impl(
                build_dir,
                source_dir,
                include_dirs,
                source_files,
                defines,
                link_inputs,
                output_path
            );
        default:
            throw std::runtime_error(std::format("cpp_compiler::create_library: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

filesystem::path_t create_binary(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& output_path
) {
    return create_binary_impl(
        build_dir,
        source_dir,
        include_dirs,
        source_files,
        defines,
        link_inputs,
        output_path
    );
}

} // namespace compiler

} // namespace kernel
