#ifndef M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
# define M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

# include <format>
# include <string>

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

/** @brief Selects the algorithm for a download checksum. */
enum class checksum_algorithm_t {
    sha256,
    sha3_256
};

/** @brief Holds a supported algorithm and a validated lowercase hexadecimal digest. */
class checksum_t {
public:
    /** @brief Rejects unsupported algorithms and digests other than 64 lowercase hexadecimal characters. */
    checksum_t(checksum_algorithm_t checksum_algorithm, std::string digest);

    checksum_algorithm_t algorithm() const;
    const std::string& digest() const;

private:
    checksum_algorithm_t m_checksum_algorithm;
    std::string m_digest;
};

/**
 * @brief Locks a download URL to one expected checksum.
 * SHA3-256 verification requires GNU cksum with SHA3 support.
 *
 * @code
 * source_lock_t{.url = url, .checksum = {checksum_algorithm_t::sha3_256, expected_hex}};
 * @endcode
 */
struct source_lock_t {
    std::string url;
    checksum_t checksum;
};

/**
 * @brief Downloads source_lock.url to output_path, verifies its checksum, and returns output_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t fetch(const source_lock_t& source_lock, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path);

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download

namespace std {

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_algorithm_t>;

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_t>;

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_algorithm_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

} // namespace std

#endif // M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
