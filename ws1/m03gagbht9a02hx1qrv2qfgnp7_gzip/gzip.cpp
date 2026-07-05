#include <m03gagbht9a02hx1qrv2qfgnp7_gzip/gzip.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>

#ifndef M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_PATH
# error M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_PATH must be defined by the owning builder
#endif

namespace m03gagbht9a02hx1qrv2qfgnp7_gzip {

static std::string host_gzip_string() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("gzip: host tool '{}' does not exist or is not a regular file", result));
    }

    return result.string();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t gzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_gzip_path
) {
    if (install_gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip::gzip: install path '{}' must have .gz extension", install_gzip_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_gzip_path)) {
        throw std::runtime_error(std::format("gzip::gzip: install path '{}' already exists", install_gzip_path));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(file) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(file)) {
        throw std::runtime_error(std::format("gzip::gzip: input '{}' is not a regular file", file));
    }

    const auto parent = install_gzip_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    const auto temporary_input = install_gzip_path + ".input";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_input)) {
        throw std::runtime_error(std::format("gzip: temporary path '{}' already exists", temporary_input));
    }

    const auto temporary_output = temporary_input + ".gz";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_output)) {
        throw std::runtime_error(std::format("gzip: temporary path '{}' already exists", temporary_output));
    }

    try {
        m03gagbhsnusi43zogoacgj2ez_filesystem::copy(file, temporary_input);

        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            host_gzip_string(),
            "-n",
            temporary_input.string()
        }));

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_output)) {
            throw std::runtime_error(std::format("gzip::gzip: expected temporary output '{}' to exist but it does not", temporary_output));
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::rename_strict(temporary_output, install_gzip_path);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(temporary_input);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(temporary_output);
        throw ;
    }

    return install_gzip_path;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t ungzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& gzip_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_file
) {
    if (gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip::ungzip: gzip path '{}' must have .gz extension", gzip_path));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(gzip_path) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(gzip_path)) {
        throw std::runtime_error(std::format("gzip::ungzip: input '{}' is not a regular file", gzip_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_file)) {
        throw std::runtime_error(std::format("gzip::ungzip: install file '{}' already exists", install_file));
    }

    const auto parent = install_file.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    const auto temporary_input = install_file + ".input.gz";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_input)) {
        throw std::runtime_error(std::format("gzip: temporary path '{}' already exists", temporary_input));
    }

    const auto temporary_output = install_file + ".input";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_output)) {
        throw std::runtime_error(std::format("gzip: temporary path '{}' already exists", temporary_output));
    }

    try {
        m03gagbhsnusi43zogoacgj2ez_filesystem::copy(gzip_path, temporary_input);

        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            host_gzip_string(),
            "-d",
            temporary_input.string()
        }));

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(temporary_output)) {
            throw std::runtime_error(std::format("gzip::ungzip: expected temporary output '{}' to exist but it does not", temporary_output));
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::rename_strict(temporary_output, install_file);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(temporary_input);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(temporary_output);
        throw ;
    }

    return install_file;
}

} // namespace m03gagbht9a02hx1qrv2qfgnp7_gzip
