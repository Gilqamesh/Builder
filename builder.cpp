#include "api.h"
#include "phase.h"

#include <format>
#include <stdexcept>
#include <type_traits>

namespace kernel {

static std::vector<compiler::define_t> tool_path_defines() {
    return {
        compiler::define_t("KERNEL_CXX_COMPILER_PATH", KERNEL_CXX_COMPILER_PATH),
        compiler::define_t("KERNEL_CC_COMPILER_PATH", KERNEL_CC_COMPILER_PATH),
        compiler::define_t("KERNEL_AR_PATH", KERNEL_AR_PATH)
    };
}

static std::vector<filesystem::relative_path_t> kernel_library_source_files() {
    return {
        filesystem::relative_path_t("filesystem.cpp"),
        filesystem::relative_path_t("process.cpp"),
        filesystem::relative_path_t("api.cpp"),
        filesystem::relative_path_t("linker.cpp"),
        filesystem::relative_path_t("compiler.cpp"),
        filesystem::relative_path_t("shared_library.cpp"),
        filesystem::relative_path_t("graph.cpp"),
        filesystem::relative_path_t("phase.cpp")
    };
}

BUILDER_EXTERN void phase__source(const kernel::phase::source_phase_t* phase) {
    phase->add_source_tree();
}

BUILDER_EXTERN void phase__interface(const kernel::phase::interface_phase_t* phase) {
    phase->add_headers_from_source();
}

BUILDER_EXTERN void phase__library(const kernel::phase::library_phase_t* phase) {
    const auto library_name = [&]() {
        switch (phase->module_config().library_type) {
            case kernel::module_config::library_type_t::STATIC: return kernel::filesystem::relative_path_t("libkernel.a");
            case kernel::module_config::library_type_t::SHARED: return kernel::filesystem::relative_path_t("libkernel.so");
            default: throw std::runtime_error(std::format("kernel::phase__library: unknown library_type {}", static_cast<std::underlying_type_t<kernel::module_config::library_type_t>>(phase->module_config().library_type)));
        }
    }();

    const auto library = kernel::compiler::create_library(
        *phase,
        kernel::kernel_library_source_files(),
        kernel::tool_path_defines(),
        library_name
    );
    phase->add_library(library, library_name);
}

BUILDER_EXTERN void phase__binary(const kernel::phase::binary_phase_t* phase) {
    const auto binary_name = kernel::filesystem::relative_path_t("cli");

    const auto binary = kernel::compiler::create_binary(
        *phase,
        { kernel::filesystem::relative_path_t("cli.cpp") },
        kernel::tool_path_defines(),
        binary_name
    );
    phase->add_binary(binary, binary_name);
}

} // namespace kernel
