#include "api.h"
#include "phase.h"

namespace kernel {

static std::vector<binding::binding_t> tool_path_defines() {
    return {
        binding::binding_t("KERNEL_CXX_COMPILER_PATH", KERNEL_CXX_COMPILER_PATH),
        binding::binding_t("KERNEL_CC_COMPILER_PATH", KERNEL_CC_COMPILER_PATH),
        binding::binding_t("KERNEL_AR_PATH", KERNEL_AR_PATH)
    };
}

static std::vector<filesystem::relative_path_t> kernel_library_source_files() {
    return {
        filesystem::relative_path_t("filesystem.cpp"),
        filesystem::relative_path_t("process.cpp"),
        filesystem::relative_path_t("binding.cpp"),
        filesystem::relative_path_t("api.cpp"),
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
    phase->add_library(kernel::default_library(kernel::kernel_library_source_files(),
        kernel::tool_path_defines()
    ));
}

BUILDER_EXTERN void phase__binary(const kernel::phase::binary_phase_t* phase) {
    phase->add_cli(kernel::default_cli({ kernel::filesystem::relative_path_t("cli.cpp") },
        kernel::tool_path_defines()
    ));
}

} // namespace kernel
