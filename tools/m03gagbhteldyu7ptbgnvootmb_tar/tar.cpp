#include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>

#ifndef M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_PATH
# error M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_PATH must be defined by the owning builder
#endif

namespace tar {

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t host_tar_path() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("tar: host tool '{}' does not exist or is not a regular file", result));
    }

    return result;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t tar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_tar_path
) {
    if (install_tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("tar::tar: install path '{}' must have .tar extension", install_tar_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_tar_path)) {
        throw std::runtime_error(std::format("tar::tar: install path '{}' already exists", install_tar_path));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(dir) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(dir)) {
        throw std::runtime_error(std::format("tar::tar: input '{}' is not a directory", dir));
    }

    const auto parent_install_tar_path = install_tar_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent_install_tar_path)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent_install_tar_path);
    }

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(host_tar_path());
    process_args.push_back("-cf");
    process_args.push_back(install_tar_path);
    if (dir.is_child(install_tar_path)) {
        process_args.push_back(std::format("--exclude={}", dir.relative(install_tar_path)));
    }
    process_args.push_back("-C");
    process_args.push_back(dir);
    process_args.push_back(".");

    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { .args = process_args });

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_tar_path) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(install_tar_path)) {
            throw std::runtime_error(std::format("tar::tar: expected output '{}' to exist as a regular file", install_tar_path));
        }
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(install_tar_path);
        throw ;
    }

    return install_tar_path;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t untar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& tar_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_dir
) {
    if (tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("tar::untar: tar path '{}' must have .tar extension", tar_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir) && !m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(install_dir)) {
        throw std::runtime_error(std::format("tar::untar: install path '{}' exists and is not a directory", install_dir));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(tar_path) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(tar_path)) {
        throw std::runtime_error(std::format("tar::untar: tar path '{}' does not exist or is not a regular file", tar_path));
    }

    const bool created_install_dir = !m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(install_dir);
    }

    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t {
            .args = {
                host_tar_path(),
                "-xf",
                tar_path,
                "-C",
                install_dir
            }
        });
    } catch (...) {
        if (created_install_dir) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
        }
        throw ;
    }

    return install_dir;
}

} // namespace tar
