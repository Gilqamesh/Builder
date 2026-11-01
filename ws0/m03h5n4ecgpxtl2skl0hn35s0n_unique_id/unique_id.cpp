#include "unique_id.h"

#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace m03h5n4ecgpxtl2skl0hn35s0n_unique_id {

unique_id_t::unique_id_t(std::string_view text):
    m_uuid([text] {
        if (text.size() != encoded_size) {
            throw std::invalid_argument(std::format("unique_id_t: expected {} lowercase base36 digits, got {}", encoded_size, text.size()));
        }

        const auto decoded = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix(text, 36);
        std::array<std::byte, 16> bytes{};
        if (bytes.size() < decoded.size()) {
            throw std::invalid_argument(std::format("unique_id_t: identity '{}' exceeds 128 bits", text));
        }
        std::ranges::copy(decoded.bytes(), bytes.end() - static_cast<std::ptrdiff_t>(decoded.size()));
        return m03gagbhtft23yhjwpp881tfmc_uuid::uuid(std::span<const std::byte>(bytes));
    }())
{
}

unique_id_t::unique_id_t(const m03gagbhtft23yhjwpp881tfmc_uuid::uuid& uuid):
    m_uuid(uuid.bytes())
{
}

unique_id_t unique_id_t::generate() {
    return unique_id_t(m03gagbhtft23yhjwpp881tfmc_uuid::uuid::generate(7));
}

std::string unique_id_t::string() const {
    auto text = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t(m_uuid.bytes()).to_radix(36);
    text.insert(0, encoded_size - text.size(), '0');
    return text;
}

m03gagbhtft23yhjwpp881tfmc_uuid::uuid unique_id_t::uuid() const {
    return m_uuid;
}

bool unique_id_t::operator==(const unique_id_t& other) const noexcept {
    return std::ranges::equal(m_uuid.bytes(), other.m_uuid.bytes());
}

} // namespace m03h5n4ecgpxtl2skl0hn35s0n_unique_id
