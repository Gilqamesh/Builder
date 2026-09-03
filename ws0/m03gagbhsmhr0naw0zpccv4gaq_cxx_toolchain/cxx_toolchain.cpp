#include "cxx_toolchain.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH must be defined by bootstrap
#endif

namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain {

static std::string host_tool_string(std::string_view path, std::string_view context) {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(std::string(path));
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("{}: host tool '{}' does not exist or is not a regular file", context, result));
    }

    return result.string();
}

static std::string cxx_compiler_string() {
    return host_tool_string(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH, "m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cxx_compiler_string");
}

static std::string cc_compiler_string() {
    return host_tool_string(M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH, "m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cc_compiler_string");
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

    std::vector<std::string> process_prefix_args;
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

        std::vector<std::string> process_args;
        if (source_path.extension() == ".c") {
            process_args.push_back(cc_compiler_string());
        } else {
            process_args.push_back(cxx_compiler_string());
            process_args.push_back("-std=c++23");
        }
        process_args.insert(process_args.end(), process_prefix_args.begin(), process_prefix_args.end());
        if (is_position_independent) {
            process_args.push_back("-fPIC");
        }
        process_args.push_back("-c");
        process_args.push_back(source_path.string());
        process_args.push_back("-o");
        process_args.push_back(object_file.string());

        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t(std::move(process_args)));

        result.push_back(object_file);
    }

    return result;
}

static void append_runtime_library_paths(
    std::vector<std::string>& process_args,
    const link_inputs_t& link_inputs
) {
    for (const auto& library : link_inputs.libraries) {
        process_args.push_back(std::format("-Wl,-rpath,{}", library.parent()));
    }
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

    std::vector<std::string> process_args;
    process_args.push_back(cxx_compiler_string());
    process_args.push_back("-g");
    process_args.push_back("-shared");
    process_args.push_back("-o");
    process_args.push_back(shared_library.string());
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file.string());
    }

    for (const auto& library : link_inputs.libraries) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(library)) {
            throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library: library does not exist '{}'", library));
        }

        process_args.push_back(library.string());
    }

    append_runtime_library_paths(process_args, link_inputs);

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t(process_args));

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

    std::vector<std::string> process_args;
    process_args.push_back(cxx_compiler_string());
    process_args.push_back("-g");
    process_args.push_back("-std=c++23");
    process_args.push_back("-o");
    process_args.push_back(binary.string());
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file.string());
    }

    for (const auto& library : link_inputs.libraries) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(library)) {
            throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary: library does not exist '{}'", library));
        }
        process_args.push_back(library.string());
    }

    append_runtime_library_paths(process_args, link_inputs);

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t(process_args));

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(binary)) {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

define_t::define_t(std::string key, std::string value):
    m_key(std::move(key)),
    m_value(std::move(value))
{
    if (m_key.empty()) {
        throw std::runtime_error("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t: key cannot be empty");
    }


    const auto first_char = m_key.front();
    if (!('A' <= first_char && first_char <= 'Z') && !('a' <= first_char && first_char <= 'z') && first_char != '_') {
        throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t: invalid compile define key '{}'", m_key));
    }

    for (const char c : std::string_view(m_key).substr(1)) {
        if (!('A' <= c && c <= 'Z') && !('a' <= c && c <= 'z') && !('0' <= c && c <= '9') && c != '_') {
            throw std::runtime_error(std::format("m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t: invalid compile define key '{}'", m_key));
        }
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
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
) {
    return build_dynamic_library_impl(
        build_dir,
        include_dirs,
        source_files,
        defines,
        link_inputs,
        output_path
    );
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
