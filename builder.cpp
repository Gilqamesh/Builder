#include "compiler.h"
#include "phase.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {

std::string quote_define_value(std::string_view value) {
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

std::vector<std::pair<std::string, std::string>> tool_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CXX_COMPILER_PATH", quote_define_value(kernel::cpp_builder::compiler::CXX_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_CC_COMPILER_PATH", quote_define_value(kernel::cpp_builder::compiler::CC_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(kernel::cpp_builder::compiler::AR_PATH) }
    };
}

std::vector<kernel::cpp_builder::filesystem::relative_path_t> kernel_library_source_files() {
    return {
        kernel::cpp_builder::filesystem::relative_path_t("filesystem.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("process.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("compiler.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("shared_library.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("graph.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("phase.cpp"),
        kernel::cpp_builder::filesystem::relative_path_t("module_builder.cpp")
    };
}

} // namespace

extern "C" void phase__interface(const kernel::cpp_builder::builder::interface_phase_t* phase) {
    const auto source_outputs = phase->materialize<kernel::cpp_builder::builder::source_phase_t>();
    const auto& source_output = phase->current_output<kernel::cpp_builder::builder::source_phase_t>(source_outputs);
    const auto& module_source_dir = source_output.source_root;

    for (const auto& interface : kernel::cpp_builder::filesystem::find(module_source_dir, kernel::cpp_builder::filesystem::find_include_predicate_t::h_file || kernel::cpp_builder::filesystem::find_include_predicate_t::hpp_file, kernel::cpp_builder::filesystem::find_descend_predicate_t::descend_all)) {
        kernel::cpp_builder::builder::install_interface(*phase, interface, module_source_dir.relative(interface));
    }
}

extern "C" void phase__library(const kernel::cpp_builder::builder::library_phase_t* phase) {
    const auto library_name = [&]() {
        switch (phase->library_type) {
            case kernel::cpp_builder::builder::library_type_t::STATIC: return kernel::cpp_builder::filesystem::relative_path_t("libcpp_builder.a");
            case kernel::cpp_builder::builder::library_type_t::SHARED: return kernel::cpp_builder::filesystem::relative_path_t("libcpp_builder.so");
            default: throw std::runtime_error(std::format("kernel::cpp_builder::phase__library: unknown library_type {}", static_cast<std::underlying_type_t<kernel::cpp_builder::builder::library_type_t>>(phase->library_type)));
        }
    }();

    switch (phase->library_type) {
        case kernel::cpp_builder::builder::library_type_t::STATIC: {
            kernel::cpp_builder::compiler::create_static_library(
                *phase,
                kernel_library_source_files(),
                tool_path_defines(),
                library_name
            );
        } break ;
        case kernel::cpp_builder::builder::library_type_t::SHARED: {
            kernel::cpp_builder::compiler::create_shared_library(
                *phase,
                kernel_library_source_files(),
                tool_path_defines(),
                {},
                library_name
            );
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::phase__library: unknown library_type {}", static_cast<std::underlying_type_t<kernel::cpp_builder::builder::library_type_t>>(phase->library_type)));
    }
}

extern "C" void phase__binary(const kernel::cpp_builder::builder::binary_phase_t* phase) {
    const auto library_outputs = phase->materialize<kernel::cpp_builder::builder::library_phase_t>();
    std::vector<std::vector<kernel::cpp_builder::filesystem::path_t>> library_groups;
    for (const auto& library_output : library_outputs) {
        if (!library_output.libraries.empty()) {
            library_groups.push_back(library_output.libraries);
        }
    }

    kernel::cpp_builder::compiler::create_binary(
        *phase,
        { kernel::cpp_builder::filesystem::relative_path_t("cli.cpp") },
        tool_path_defines(),
        library_groups,
        true,
        kernel::cpp_builder::filesystem::relative_path_t("cli")
    );
}
