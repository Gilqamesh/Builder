#include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>

#ifndef M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH
# error M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH must be defined by the owning builder
#endif

namespace m03gagbht3svcx3ign454lfup3_cmake {

void configure(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
) {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(source_dir)) {
        throw std::runtime_error(std::format("cmake::configure: source_dir '{}' does not exist", source_dir));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH);
    process_args.push_back("-S");
    process_args.push_back(source_dir);
    process_args.push_back("-B");
    process_args.push_back(build_dir);

    for (const auto& define_key_value : define_key_values) {
        process_args.push_back(std::format("-D{}={}", define_key_value.first, define_key_value.second));
    }

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });
}

void build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir, std::optional<std::size_t> n_jobs) {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake::build: build_dir '{}' does not exist", build_dir));
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH);
    process_args.push_back("--build");
    process_args.push_back(build_dir);

    if (n_jobs.has_value()) {
        process_args.push_back(std::format("-j{}", n_jobs.value()));
    } else {
        process_args.push_back("-j");
    }

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });
}

void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir) {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake::install: build_dir '{}' does not exist", build_dir));
    }

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t {
        .args = {
            M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH,
            "--install",
            build_dir
        }
    });
}

} // namespace m03gagbht3svcx3ign454lfup3_cmake
