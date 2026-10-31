#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto library = phase->build_library(
        {phase->source("download.cpp"), phase->source("helpers.cpp")},
        {m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t("M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_CKSUM_PATH", "/usr/bin/cksum")}
    );
    phase->install_library(library);
}

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download
