#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>
#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>
#include <m03gagbht9a02hx1qrv2qfgnp7_gzip/gzip.h>
#include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>

#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>

namespace google_test {

static constexpr auto GOOGLE_TEST_SOURCE_URL = "https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.15.0";
static constexpr auto GOOGLE_TEST_SOURCE_SHA256 = "7315acb6bf10e99f332c8a43f00d5fbb1ee6ca48c52f6b936991b216c586aaad";

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();

    const auto google_test_tar_gz = download::fetch(
        download::source_lock_t {
            .url = GOOGLE_TEST_SOURCE_URL,
            .sha256 = GOOGLE_TEST_SOURCE_SHA256
        },
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("google_test-1.15.0.tar.gz")
    );

    const auto google_test_tar = gzip::ungzip(
        google_test_tar_gz,
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("google_test-1.15.0.tar")
    );
    const auto google_test_extract_dir = tar::untar(
        google_test_tar,
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream")
    );
    const auto google_test_source_dir = google_test_extract_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("googletest-1.15.0");
    phase->install_source(google_test_source_dir);
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto gtest_interface = phase->build_interface_as(
        sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream/googletest-1.15.0/googletest/include/gtest"),
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("gtest")
    );
    const auto gmock_interface = phase->build_interface_as(
        sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream/googletest-1.15.0/googlemock/include/gmock"),
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("gmock")
    );

    phase->install_interface(gtest_interface);
    phase->install_interface(gmock_interface);
    phase->install_interface_compatibility(gtest_interface);
    phase->install_interface_compatibility(gmock_interface);
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto google_test_source_dir = sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream/googletest-1.15.0");
    const auto library_build_dir = phase->build_dir();
    const auto cmake_build_dir = library_build_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("m03gagbht3svcx3ign454lfup3_cmake");
    const auto shared_libs = phase->library_type() == m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED;

    cmake::configure(google_test_source_dir, cmake_build_dir, {
        { "BUILD_SHARED_LIBS", shared_libs ? "ON" : "OFF" },
        { "INSTALL_GTEST", "ON" },
        { "CMAKE_INSTALL_PREFIX", library_build_dir.string() }
    });

    cmake::build(cmake_build_dir, std::nullopt);
    cmake::install(cmake_build_dir);

    bool installed_library = false;
    for (const auto& artifact : m03gagbhsnusi43zogoacgj2ez_filesystem::find(
        library_build_dir,
        !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir,
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t([&](const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dir, std::size_t) {
            return dir != cmake_build_dir;
        })
    )) {
        const auto artifact_path = artifact.path();
        const auto filename = artifact_path.filename();
        if ((filename.starts_with("libgtest") || filename.starts_with("libgmock")) && (filename.find(".so") != std::string::npos || artifact_path.extension() == ".a")) {
            phase->install_library(artifact_path);
            installed_library = true;
        }
    }
    if (!installed_library) {
        throw std::runtime_error(std::format("libraries/m03gagbhszuoyfx4uxgozn8yrr_google_test: expected google_test libraries under '{}'", library_build_dir));
    }
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t*) {
}

} // namespace google_test
