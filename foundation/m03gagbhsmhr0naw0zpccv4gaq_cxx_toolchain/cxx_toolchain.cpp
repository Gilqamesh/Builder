#include "cxx_toolchain.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH must be defined by bootstrap
#endif

namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain {

static bool is_valid_define_key(std::string_view key) {
    if (key.empty()) {
        return false;
    }

    const auto is_initial_char = [](char c) {
        return ('A' <= c && c <= 'Z')
            || ('a' <= c && c <= 'z')
            || c == '_';
    };
    const auto is_key_char = [&](char c) {
        return is_initial_char(c) || ('0' <= c && c <= '9');
    };

    if (!is_initial_char(key.front())) {
        return false;
    }

    for (const char c : key.substr(1)) {
        if (!is_key_char(c)) {
            return false;
        }
    }

    return true;
}

static std::string cxx_string_literal_replacement(const std::string& value) {
    std::string result("\"");

    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> build_object_files(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    bool is_position_independent
) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> result;
    result.reserve(source_files.size());

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_prefix_args;
    process_prefix_args.push_back("-g");

    for (const auto& define : defines) {
        process_prefix_args.push_back(std::format("-D{}={}", define.key(), cxx_string_literal_replacement(define.value())));
    }

    for (const auto& include_dir : include_dirs) {
        process_prefix_args.push_back(std::format("-I{}", include_dir));
    }

    for (const auto& source_file : source_files) {
        const auto source_path = source_file.path();

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(source_path)) {
            throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_object_files: source file does not exist '{}'", source_path));
        }

        auto object_file = build_dir / source_file.relative_path();
        object_file.extension(".o");

        const auto object_file_dir = object_file.parent();
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(object_file_dir)) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(object_file_dir);
        }

        std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
        if (source_path.extension() == ".c") {
            process_args.push_back(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH);
        } else {
            process_args.push_back(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH);
            process_args.push_back("-std=c++23");
        }
        process_args.insert(process_args.end(), process_prefix_args.begin(), process_prefix_args.end());
        if (is_position_independent) {
            process_args.push_back("-fPIC");
        }
        process_args.push_back("-c");
        process_args.push_back(source_path);
        process_args.push_back("-o");
        process_args.push_back(object_file);

        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });

        result.push_back(object_file);
    }

    return result;
}

static void append_runtime_library_paths(
    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& process_args,
    const link_inputs_t& link_inputs
) {
    for (const auto& group : link_inputs.groups) {
        for (const auto& library : group.libraries) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library.parent()));
        }
    }
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_archive_library_impl(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& static_library
) {
    const auto object_files = build_object_files(
        build_dir,
        include_dirs,
        source_files,
        defines,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(static_library_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(static_library_dir);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH);
    process_args.push_back("rcs");
    process_args.push_back(static_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(static_library)) {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: expected output static library '{}' to exist but it does not", static_library));
    }

    return static_library;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_dynamic_library_impl(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& shared_library
) {
    const auto object_files = build_object_files(
        build_dir,
        include_dirs,
        source_files,
        defines,
        true
    );

    const auto shared_library_dir = shared_library.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(shared_library_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(shared_library_dir);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH);
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
            if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(library)) {
                throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: library does not exist '{}'", library));
            }

            process_args.push_back(library);
        }
        if (group.static_library_group) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    append_runtime_library_paths(process_args, link_inputs);

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(shared_library)) {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: expected output shared library '{}' to exist but it does not", shared_library));
    }

    return shared_library;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_binary_impl(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary
) {
    const auto object_files = build_object_files(
        build_dir,
        include_dirs,
        source_files,
        defines,
        true
    );

    const auto binary_dir = binary.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(binary_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(binary_dir);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH);
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
            if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(library)) {
                throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary: library does not exist '{}'", library));
            }
            process_args.push_back(library);
        }
        if (group.static_library_group) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    append_runtime_library_paths(process_args, link_inputs);

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(binary)) {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

define_t::define_t(std::string key, std::string value):
    m_key(std::move(key)),
    m_value(std::move(value))
{
    if (!is_valid_define_key(m_key)) {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t::define_t: invalid compile define key '{}'", m_key));
    }
}

const std::string& define_t::key() const {
    return m_key;
}

const std::string& define_t::value() const {
    return m_value;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    library_type_t library_type,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
) {
    switch (library_type) {
        case library_type_t::STATIC:
            if (!link_inputs.groups.empty()) {
                throw std::runtime_error("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: static libraries do not support link inputs");
            }

            return build_archive_library_impl(
                build_dir,
                include_dirs,
                source_files,
                defines,
                output_path
            );
        case library_type_t::SHARED:
            return build_dynamic_library_impl(
                build_dir,
                include_dirs,
                source_files,
                defines,
                link_inputs,
                output_path
            );
        default:
            throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_binary(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
) {
    return build_binary_impl(
        build_dir,
        include_dirs,
        source_files,
        defines,
        link_inputs,
        output_path
    );
}

} // namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain
