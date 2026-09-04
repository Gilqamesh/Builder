#include "byte_stream.h"

#include <algorithm>
#include <fstream>
#include <format>
#include <stdexcept>

namespace m03gagbht2l61mj6qitacwbmea_byte_stream {

byte_stream_t::byte_stream_t()
{
}

byte_stream_t::byte_stream_t(const byte_stream_t& other): byte_stream_t(other.bytes())
{
}

byte_stream_t::byte_stream_t(byte_stream_t&& other) noexcept:
    m_bytes(std::move(other.m_bytes))
{
}

byte_stream_t& byte_stream_t::operator=(const byte_stream_t& other) {
    if (this != &other) {
        byte_stream_t tmp(other);
        std::swap(m_bytes, tmp.m_bytes);
    }
    return *this;
}

byte_stream_t& byte_stream_t::operator=(byte_stream_t&& other) noexcept {
    if (this != &other) {
        m_bytes = std::move(other.m_bytes);
    }
    return *this;
}

byte_stream_t::byte_stream_t(std::span<const std::byte> bytes): byte_stream_t(std::vector<std::byte>(bytes.begin(), bytes.end()))
{
}

byte_stream_t::byte_stream_t(std::vector<std::byte> bytes):
    m_bytes(std::move(bytes))
{
}

std::span<const std::byte> byte_stream_t::bytes() const& noexcept {
    return std::as_bytes(std::span<const std::byte>(m_bytes));
}

std::span<std::byte> byte_stream_t::bytes() & noexcept {
    return std::as_writable_bytes(std::span<std::byte>(m_bytes));
}

void byte_stream_t::push_back(std::byte value) {
    m_bytes.push_back(value);
}

void byte_stream_t::append(std::span<const std::byte> bytes) {
    const std::vector<std::byte> appended(bytes.begin(), bytes.end());
    m_bytes.insert(m_bytes.end(), appended.begin(), appended.end());
}

void byte_stream_t::append(byte_stream_t&& other) {
    append(other.bytes());
}

void byte_stream_t::clear() noexcept {
    m_bytes.clear();
}

std::size_t byte_stream_t::size() const noexcept {
    return m_bytes.size();
}

bool byte_stream_t::empty() const noexcept {
    return m_bytes.empty();
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

    return byte_stream_t(std::move(bytes));
}

std::string byte_stream_t::to_radix(uint32_t radix) const {
    if (radix < 2 || 36 < radix) {
        throw std::invalid_argument(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::to_radix: invalid radix {}", radix));
    }

    std::vector<std::byte> bytes = m_bytes;

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

byte_stream_t byte_stream_t::from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::ios_base::failure(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_file: cannot open file '{}'", path.string()));
    }

    byte_stream_t result;
    constexpr std::size_t chunk_size = 64 * 1024;
    while (true) {
        const std::size_t offset = result.m_bytes.size();
        if (result.m_bytes.max_size() - offset < chunk_size) {
            throw std::length_error(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_file: file '{}' exceeds the maximum byte stream size", path.string()));
        }
        result.m_bytes.resize(offset + chunk_size);
        file.read(reinterpret_cast<char*>(result.m_bytes.data() + offset), static_cast<std::streamsize>(chunk_size));
        const std::size_t bytes_read = static_cast<std::size_t>(file.gcount());
        result.m_bytes.resize(offset + bytes_read);
        if (bytes_read == chunk_size) {
            continue;
        }
        if (file.bad() || !file.eof()) {
            throw std::ios_base::failure(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_file: cannot read file '{}'", path.string()));
        }
        return result;
    }
}

void byte_stream_t::to_file(const std::filesystem::path& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::ios_base::failure(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::to_file: cannot open file '{}'", path.string()));
    }

    constexpr std::size_t chunk_size = 64 * 1024;
    std::size_t offset = 0;
    while (offset < m_bytes.size()) {
        const std::size_t bytes_to_write = std::min(chunk_size, m_bytes.size() - offset);
        file.write(reinterpret_cast<const char*>(m_bytes.data() + offset), static_cast<std::streamsize>(bytes_to_write));
        if (!file) {
            throw std::ios_base::failure(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::to_file: cannot write file '{}'", path.string()));
        }
        offset += bytes_to_write;
    }
    file.close();
    if (!file) {
        throw std::ios_base::failure(std::format("m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::to_file: cannot finish writing file '{}'", path.string()));
    }
}

} // namespace m03gagbht2l61mj6qitacwbmea_byte_stream
