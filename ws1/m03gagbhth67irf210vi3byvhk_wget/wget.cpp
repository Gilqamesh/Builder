#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>

#ifndef M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_PATH
# error M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_PATH must be defined by the owning builder
#endif

namespace m03gagbhth67irf210vi3byvhk_wget {

static std::string wget_string() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("wget::download: host tool '{}' does not exist or is not a regular file", result));
    }

    return result.string();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t download(
    const std::string& url,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_path
) {
    if (url.empty()) {
        throw std::runtime_error("wget::download: URL must not be empty");
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_path)) {
        throw std::runtime_error(std::format("wget::download: install path '{}' already exists", install_path));
    }

    const auto parent = install_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            wget_string(),
            "-O",
            install_path.string(),
            url
        }));

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_path)) {
            throw std::runtime_error(std::format("wget::download: expected downloaded file '{}' to exist but it does not", install_path));
        }
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(install_path);
        throw ;
    }

    return install_path;
}

} // namespace m03gagbhth67irf210vi3byvhk_wget
