#include "download.h"
#include "helpers.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>
#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

checksum_t::checksum_t(checksum_algorithm_t checksum_algorithm, std::string digest):
    m_checksum_algorithm(checksum_algorithm),
    m_digest(std::move(digest))
{
    if (m_checksum_algorithm != checksum_algorithm_t::sha256 && m_checksum_algorithm != checksum_algorithm_t::sha3_256) {
        throw std::runtime_error("download::checksum_t: unsupported checksum algorithm");
    }
    if (m_digest.size() != 64 || !std::ranges::all_of(m_digest, [](char digit) {
        return ('0' <= digit && digit <= '9') || ('a' <= digit && digit <= 'f');
    })) {
        throw std::runtime_error(std::format("download::checksum_t: expected {} digest '{}' must be 64 lowercase hexadecimal characters", m_checksum_algorithm, m_digest));
    }
}

checksum_algorithm_t checksum_t::algorithm() const {
    return m_checksum_algorithm;
}

const std::string& checksum_t::digest() const {
    return m_digest;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t fetch(const source_lock_t& source_lock, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path) {
    if (source_lock.url.empty()) {
        throw std::runtime_error("download::fetch: source lock URL must not be empty");
    }

    const auto result = m03gagbhth67irf210vi3byvhk_wget::download(source_lock.url, output_path);
    try {
        if (source_lock.checksum.algorithm() == checksum_algorithm_t::sha256) {
            m03gagbhtbusaqidrtw6lnugr4_sha256sum::verify(result, source_lock.checksum.digest());
        } else {
            verify_sha3_256(result, source_lock.checksum.digest());
        }
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(result);
        throw;
    }

    return result;
}

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download
