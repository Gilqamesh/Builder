#include "sha256sum.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH
# error M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH must be defined by the owning builder
#endif

namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum {

static std::string sha256sum_string(std::string_view operation) {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("sha256sum::{}: host tool '{}' does not exist or is not a regular file", operation, result));
    }

    return result.string();
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

    const auto filename = path.string();
    const bool escape_filename = filename.find_first_of("\\\n\r") != std::string::npos;
    if (escape_filename) {
        ofs << '\\';
    }
    ofs << expected_sha256 << "  ";
    for (const char character : filename) {
        switch (character) {
            case '\\': {
                ofs << "\\\\";
            } break;
            case '\n': {
                ofs << "\\n";
            } break;
            case '\r': {
                ofs << "\\r";
            } break;
            default: {
                ofs << character;
            } break;
        }
    }
    ofs << '\n';
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
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            sha256sum_string("verify"),
            "--status",
            "--check",
            "--strict",
            checksum_path.string()
        }));
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(checksum_path);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(checksum_path);
        throw ;
    }
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t generate(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
    if (!filesystem::exists(path) || !filesystem::is_regular_file(path)) {
        throw std::runtime_error(std::format("sha256sum::generate: path '{}' does not exist or is not a regular file", path));
    }
    const auto checksum_path = path + ".sha256";
    if (filesystem::exists(checksum_path)) {
        throw std::runtime_error(std::format("sha256sum::generate: checksum path '{}' already exists", checksum_path));
    }

    auto temporary_pattern = (path.parent().to_native_path() / ".sha256sum-generate-XXXXXX").string();
    if (mkdtemp(temporary_pattern.data()) == nullptr) {
        throw std::runtime_error(std::format("sha256sum::generate: create scratch directory beside '{}': {}", path, std::strerror(errno)));
    }
    const auto temporary_dir = filesystem::path_t(temporary_pattern);
    const auto temporary_checksum = temporary_dir / filesystem::relative_path_t("checksum");
    const auto filename = path.filename();
    // GNU treats a bare '-' operand as stdin, even after '--'.
    const auto operand = filename == "-" ? "./-" : filename;
    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            "/bin/sh",
            "-ec",
            "exec \"$1\" -- \"$2\" > \"$3\"",
            "sha256sum::generate",
            sha256sum_string("generate"),
            operand,
            temporary_checksum.string()
        }, path.parent()));
        filesystem::rename_strict(temporary_checksum, checksum_path);
        filesystem::remove_all(temporary_dir);
    } catch (...) {
        std::error_code cleanup_error;
        std::filesystem::remove_all(temporary_dir.to_native_path(), cleanup_error);
        throw;
    }
    return checksum_path;
}

} // namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum
