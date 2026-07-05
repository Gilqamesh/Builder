#include "byte_stream.h"

#include <algorithm>
#include <stdexcept>
#include <format>

namespace m03gagbht2l61mj6qitacwbmea_byte_stream {

byte_stream_t::byte_stream_t()
{
}

byte_stream_t::byte_stream_t(std::span<const std::byte> bytes):
    m_bytes(bytes.begin(), bytes.end())
{
}

std::span<const std::byte> byte_stream_t::bytes() const noexcept {
    return m_bytes;
}

size_t byte_stream_t::size() const noexcept {
    return m_bytes.size();
}

byte_stream_t byte_stream_t::from_radix(std::string_view text, uint32_t radix) {
    if (radix < 2 || 36 < radix) {
        throw std::invalid_argument(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix: invalid radix {}", radix));
    }

    std::size_t begin = 0;

    while (begin < text.size() && text[begin] == '0') {
        ++begin;
    }

    std::vector<std::byte> bytes;

    while (begin < text.size()) {
        uint32_t carry;
        char digit = text[begin++];
        if ('0' <= digit && digit <= '9') {
            carry = static_cast<uint32_t>(digit - '0');
        } else if ('a' <= digit && digit <= 'z') {
            carry = static_cast<uint32_t>(digit - 'a' + 10);
        } else {
            throw std::invalid_argument(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix: invalid character '{}'", digit));
        }

        if (radix <= carry) {
            throw std::invalid_argument(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix: invalid character '{}' for radix {}", digit, radix));
        }

        for (auto& byte : bytes) {
            uint32_t value = static_cast<uint32_t>(byte) * radix + carry;

            byte = std::byte(value);
            carry = value >> 8;
        }

        while (carry != 0) {
            bytes.push_back(std::byte(carry));
            carry >>= 8;
        }
    }

    std::reverse(bytes.begin(), bytes.end());

    return byte_stream_t(bytes);
}

std::string byte_stream_t::to_radix(uint32_t radix) const {
    if (radix < 2 || 36 < radix) {
        throw std::invalid_argument(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::to_radix: invalid radix {}", radix));
    }

    std::vector<std::byte> bytes(m_bytes.begin(), m_bytes.end());

    std::size_t begin = 0;

    while (begin < bytes.size() && bytes[begin] == std::byte(0)) {
        ++begin;
    }

    if (begin == bytes.size()) {
        return "0";
    }

    std::string result;

    while (begin < bytes.size()) {
        uint32_t remainder = 0;

        for (std::size_t i = begin; i < bytes.size(); ++i) {
            uint32_t value = (remainder << 8) | static_cast<uint32_t>(bytes[i]);

            bytes[i] = std::byte(value / radix);
            remainder = value % radix;
        }

        static constexpr char DIGITS[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        result.push_back(DIGITS[remainder]);

        while (begin < bytes.size() && bytes[begin] == std::byte(0)) {
            ++begin;
        }
    }

    std::reverse(result.begin(), result.end());

    return result;
}

} // namespace m03gagbht2l61mj6qitacwbmea_byte_stream
