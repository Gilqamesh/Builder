#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>
#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>

#include <stdexcept>

namespace download {

static void validate_source_lock(const source_lock_t& source_lock) {
    if (source_lock.url.empty()) {
        throw std::runtime_error("download::fetch: source lock URL must not be empty");
    }

    if (source_lock.sha256.empty()) {
        throw std::runtime_error("download::fetch: source lock SHA-256 must not be empty");
    }
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t fetch(const source_lock_t& source_lock, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path) {
    validate_source_lock(source_lock);

    const auto result = wget::download(source_lock.url, output_path);
    try {
        sha256sum::verify(result, source_lock.sha256);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(result);
        throw ;
    }

    return result;
}

} // namespace download
