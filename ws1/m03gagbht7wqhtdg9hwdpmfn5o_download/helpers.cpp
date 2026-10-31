#include "helpers.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#ifndef M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_CKSUM_PATH
# error M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_CKSUM_PATH must be defined by the owning builder
#endif

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

void verify_sha3_256(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected) {
    namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
    const filesystem::path_t cksum(M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_CKSUM_PATH);
    if (!filesystem::exists(cksum) || !filesystem::is_regular_file(cksum)) {
        throw std::runtime_error(std::format("download::fetch: host tool '{}' does not exist or is not a regular file", cksum));
    }
    const auto checksum_path = path + ".sha3-256";
    if (filesystem::exists(checksum_path)) {
        throw std::runtime_error(std::format("download::fetch: checksum path '{}' already exists", checksum_path));
    }
    try {
        {
            std::ofstream output(checksum_path.string(), std::ios::binary);
            const auto filename = path.string();
            const bool escape_filename = filename.find_first_of("\\\n\r") != std::string::npos;
            if (escape_filename) {
                output << '\\';
            }
            output << expected << "  ";
            for (const char character : filename) {
                switch (character) {
                    case '\\': {
                        output << "\\\\";
                    } break;
                    case '\n': {
                        output << "\\n";
                    } break;
                    case '\r': {
                        output << "\\r";
                    } break;
                    default: {
                        output << character;
                    } break;
                }
            }
            output << '\n';
            output.close();
            if (!output) {
                throw std::runtime_error(std::format("download::fetch: failed to write checksum file '{}'", checksum_path));
            }
        }
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            cksum.string(),
            "--algorithm=sha3",
            "--length=256",
            "--check",
            "--strict",
            "--status",
            checksum_path.string()
        }));
        filesystem::remove(checksum_path);
    } catch (...) {
        filesystem::remove(checksum_path);
        throw;
    }
}

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download
