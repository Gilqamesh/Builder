#include "compiler.h"
#include "phase.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace kernel {

namespace cpp_builder {

static std::string quote_define_value(std::string_view value) {
    std::string result = "\"";
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }
    result.push_back('"');
    return result;
}

static std::vector<std::pair<std::string, std::string>> tool_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CXX_COMPILER_PATH", quote_define_value(KERNEL_CPP_BUILDER_CXX_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_CC_COMPILER_PATH", quote_define_value(KERNEL_CPP_BUILDER_CC_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(KERNEL_CPP_BUILDER_AR_PATH) }
    };
}

static std::vector<filesystem::relative_path_t> kernel_library_source_files() {
    return {
        filesystem::relative_path_t("filesystem.cpp"),
        filesystem::relative_path_t("process.cpp"),
        filesystem::relative_path_t("compiler.cpp"),
        filesystem::relative_path_t("shared_library.cpp"),
        filesystem::relative_path_t("graph.cpp"),
        filesystem::relative_path_t("phase.cpp")
    };
}

BUILDER_EXTERN void phase__source(const kernel::cpp_builder::builder::source_phase_t* phase) {
    const auto source_root = phase->source_dir();

    for (const auto& source : kernel::cpp_builder::filesystem::find(source_root, !kernel::cpp_builder::filesystem::find_include_predicate_t::is_dir, kernel::cpp_builder::filesystem::find_descend_predicate_t::descend_all)) {
        phase->add_source(source, source_root.relative(source));
    }
}

BUILDER_EXTERN void phase__interface(const kernel::cpp_builder::builder::interface_phase_t* phase) {
    const auto source_output = phase->materialize<kernel::cpp_builder::builder::source_phase_t>();

    for (const auto& interface : source_output.artifacts) {
        if (kernel::cpp_builder::filesystem::find_include_predicate_t::h_file(interface.path) || kernel::cpp_builder::filesystem::find_include_predicate_t::hpp_file(interface.path)) {
            phase->add_interface(
                interface.path,
                interface.relative_path
            );
        }
    }
}

BUILDER_EXTERN void phase__library(const kernel::cpp_builder::builder::library_phase_t* phase) {
    const auto library_name = [&]() {
        switch (phase->library_type()) {
            case kernel::cpp_builder::builder::library_type_t::STATIC: return kernel::cpp_builder::filesystem::relative_path_t("libcpp_builder.a");
            case kernel::cpp_builder::builder::library_type_t::SHARED: return kernel::cpp_builder::filesystem::relative_path_t("libcpp_builder.so");
            default: throw std::runtime_error(std::format("kernel::cpp_builder::phase__library: unknown library_type {}", static_cast<std::underlying_type_t<kernel::cpp_builder::builder::library_type_t>>(phase->library_type())));
        }
    }();

    switch (phase->library_type()) {
        case kernel::cpp_builder::builder::library_type_t::STATIC: {
            const auto library = kernel::cpp_builder::compiler::create_static_library(
                *phase,
                kernel::cpp_builder::kernel_library_source_files(),
                kernel::cpp_builder::tool_path_defines(),
                library_name
            );
            phase->add_library(library, library_name);
        } break ;
        case kernel::cpp_builder::builder::library_type_t::SHARED: {
            const auto library = kernel::cpp_builder::compiler::create_shared_library(
                *phase,
                kernel::cpp_builder::kernel_library_source_files(),
                kernel::cpp_builder::tool_path_defines(),
                {},
                library_name
            );
            phase->add_library(library, library_name);
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::phase__library: unknown library_type {}", static_cast<std::underlying_type_t<kernel::cpp_builder::builder::library_type_t>>(phase->library_type())));
    }
}

BUILDER_EXTERN void phase__binary(const kernel::cpp_builder::builder::binary_phase_t* phase) {
    const auto binary_name = kernel::cpp_builder::filesystem::relative_path_t("cli");

    const auto binary = kernel::cpp_builder::compiler::create_binary(
        *phase,
        { kernel::cpp_builder::filesystem::relative_path_t("cli.cpp") },
        kernel::cpp_builder::tool_path_defines(),
        binary_name
    );
    phase->add_binary(binary, binary_name);
}

} // namespace cpp_builder

} // namespace kernel
