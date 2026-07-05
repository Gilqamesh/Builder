#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <vector>

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH
# error M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH must be defined by bootstrap
#endif

#ifndef M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH
# error M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH must be defined by bootstrap
#endif

static bool is_library_source(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& source) {
    const auto relative_path = source.relative_path();
    return relative_path.extension() == ".cpp"
        && relative_path.string() != m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::BUILDER_CPP
        && relative_path.string() != m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::CLI_CPP;
}

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    phase->install_headers_from_source();
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    std::vector<m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::built_t> source_files;
    std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t> defines;
    for (const auto& source : m03gagbhsnusi43zogoacgj2ez_filesystem::find(
        sources.root(),
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::cpp_file,
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all
    )) {
        if (is_library_source(source)) {
            const auto relative_path = source.relative_path().string();
            if (relative_path == "cxx_toolchain.cpp") {
                defines.push_back(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH", M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH));
                defines.push_back(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH", M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH));
                defines.push_back(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH", M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH));
            } else if (relative_path == "build_phases.cpp") {
                defines.push_back(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH", M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH));
            }
            source_files.push_back(phase->build(source));
        }
    }

    if (source_files.empty()) {
        return ;
    }

    const auto library = phase->build_library(
        source_files,
        defines
    );
    phase->install_library(library);
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    phase->install_cli({});
}
