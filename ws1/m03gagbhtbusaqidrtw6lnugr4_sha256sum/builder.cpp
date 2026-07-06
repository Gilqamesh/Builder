#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/api.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/api.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/api.h>

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    phase->install_api();
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto library = phase->build_library(
        { phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("sha256sum.cpp")) },
        {
            m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH", "/usr/bin/sha256sum")
        }
    );
    phase->install_library(library);
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    phase->install_cli({});
}
