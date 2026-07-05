#include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <fstream>
#include <format>
#include <stdexcept>

#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH
# error M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH must be defined by the owning builder
#endif

namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum {

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t sha256sum_path() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("sha256sum::verify: host tool '{}' does not exist or is not a regular file", result));
    }

    return result;
}

static void validate_expected_sha256(const std::string& expected_sha256) {
    if (expected_sha256.size() != 64) {
        throw std::runtime_error(std::format(
            "sha256sum::verify: expected SHA-256 '{}' must be 64 lowercase hexadecimal characters",
            expected_sha256
        ));
    }

    for (const auto c : expected_sha256) {
        if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f'))) {
            throw std::runtime_error(std::format(
                "sha256sum::verify: expected SHA-256 '{}' must be 64 lowercase hexadecimal characters",
                expected_sha256
            ));
        }
    }
}

static void write_checksum_file(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
    const std::string& expected_sha256,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& checksum_path
) {
    std::ofstream ofs(checksum_path.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("sha256sum::verify: failed to write checksum file '{}'", checksum_path));
    }

    ofs << expected_sha256 << "  " << path.string() << "\n";
    if (!ofs) {
        throw std::runtime_error(std::format("sha256sum::verify: failed to write checksum file '{}'", checksum_path));
    }
}

void verify(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected_sha256) {
    validate_expected_sha256(expected_sha256);

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(path) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(path)) {
        throw std::runtime_error(std::format("sha256sum::verify: path '{}' does not exist or is not a regular file", path));
    }

    const auto checksum_path = path + ".sha256";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(checksum_path)) {
        throw std::runtime_error(std::format("sha256sum::verify: checksum path '{}' already exists", checksum_path));
    }

    try {
        write_checksum_file(path, expected_sha256, checksum_path);
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t {
            .args = {
                sha256sum_path(),
                "--status",
                "--check",
                checksum_path
            }
        });
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(checksum_path);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(checksum_path);
        throw ;
    }
}

} // namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum
