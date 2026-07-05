#include "uuid.h"

#include <stdexcept>
#include <format>
#include <mutex>

#include <cstring>
#include <cerrno>
#include <cstddef>

#include <sys/random.h>
#include <sys/types.h>

namespace m03gagbhtft23yhjwpp881tfmc_uuid {

static unsigned parse_version(std::span<const std::byte> bytes) {
    return static_cast<unsigned>((bytes[6] & std::byte(0b11110000)) >> 4);
}

static void validate_uuidv7(std::span<const std::byte> bytes) {
    const auto got_version = parse_version(bytes);
    const auto expected_version = 7;
    if (got_version != expected_version) {
        throw std::invalid_argument(std::format("uuid::uuid: expected UUID version {}, got {}", expected_version, got_version));
    }

    const auto got_variant = static_cast<unsigned>((bytes[8] & std::byte(0b11000000)) >> 6);
    const auto expected_variant = 2;
    if (got_variant != expected_variant) {
        throw std::invalid_argument(std::format("uuid::uuid: expected UUID variant {}, got {}", expected_variant, got_variant));
    }
}

uuid::uuid(std::span<const std::byte> bytes):
    m_bytes{}
{
    if (bytes.size() != m_bytes.size()) {
        throw std::invalid_argument(
            std::format("uuid::uuid: expected {} bytes, got {}", m_bytes.size(), bytes.size())
        );
    }

    if (bytes.data() == nullptr) {
        throw std::invalid_argument("uuid::uuid: byte span data is null");
    }

    switch (parse_version(bytes)) {
        case 7: {
            validate_uuidv7(bytes);
        } break ;
        default: {
            throw std::invalid_argument(std::format("uuid::uuid: unsupported UUID version {}", parse_version(bytes)));
        } break ;
    }

    std::memcpy(m_bytes.data(), bytes.data(), m_bytes.size());
}

uuid uuid::generate(unsigned version) {
    switch (version) {
        case 7: {
            return generate_uuidv7();
        } break ;
        default: {
            throw std::invalid_argument(std::format("uuid::generate: unsupported UUID version {}", version));
        } break ;
    }
}

std::span<const std::byte> uuid::bytes() const noexcept {
    return m_bytes;
}

unsigned uuid::version() const noexcept {
    return parse_version(m_bytes);
}

std::chrono::system_clock::time_point uuid::timestamp() const {
    const auto uuid_version = version();

    switch (uuid_version) {
        case 7: {
            return timestamp_uuidv7();
        } break ;
        default: {
            throw std::logic_error(std::format("uuid::timestamp: UUID version {} has no supported timestamp representation", uuid_version));
        } break ;
    }
}

uuid::uuid(std::array<std::byte, 16> bytes):
    m_bytes(bytes)
{
}

// 48 bits timestamp, 4 bits version (0b0111), 12 bits rand_a, 2 bits variant (0b10), 62 bits rand_b
uuid uuid::generate_uuidv7() {
    std::array<std::byte, 16> array;
    std::size_t index = 0;
    while (index < array.size()) {
        ssize_t bytes_copied = getrandom(array.data() + index, array.size() - index, 0);
        if (bytes_copied < 0) {
            if (errno == EINTR) {
                continue ;
            }
            throw std::runtime_error(std::format("uuid::generate: getrandom failed: {}", std::strerror(errno)));
        }

        if (bytes_copied == 0) {
            throw std::runtime_error("uuid::generate: getrandom returned 0 bytes");
        }

        index += static_cast<std::size_t>(bytes_copied);
    }

    static std::mutex mutex;
    static bool initialized = false;
    static auto last_timestamp_millis = static_cast<std::uint64_t>(0);
    static auto last_random_a = static_cast<std::uint16_t>(0);
    static auto last_random_b = static_cast<std::uint64_t>(0);

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    if (millis < 0) {
        throw std::runtime_error("uuid::generate: system clock is before the Unix epoch");
    }

    auto timestamp_millis = static_cast<std::uint64_t>(millis);
    constexpr auto max_uuidv7_timestamp_millis = (static_cast<std::uint64_t>(1) << 48) - 1;
    if (max_uuidv7_timestamp_millis < timestamp_millis) {
        throw std::runtime_error("uuid::generate: Unix timestamp exceeds UUIDv7 48-bit timestamp range");
    }

    auto random_a = static_cast<std::uint16_t>(0);
    auto random_b = static_cast<std::uint64_t>(0);

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (initialized && timestamp_millis <= last_timestamp_millis) {
            timestamp_millis = last_timestamp_millis;

            constexpr auto max_62_bit_value = (static_cast<std::uint64_t>(1) << 62) - 1;
            if (last_random_b == max_62_bit_value) {
                throw std::runtime_error("uuid::generate: too many UUIDs generated in the same millisecond");
            }

            ++last_random_b;
        } else {
            initialized = true;
            last_timestamp_millis = timestamp_millis;

            last_random_a = static_cast<std::uint16_t>((static_cast<std::uint16_t>(array[6] & std::byte(0b00001111)) << 8) | static_cast<std::uint16_t>(array[7])); // 12 random bits from random bytes

            last_random_b = static_cast<std::uint64_t>(array[8] & std::byte(0b00111111));
            for (std::size_t i = 0; i < 7; ++i) {
                last_random_b = (last_random_b << 8) | static_cast<std::uint64_t>(array[9 + i]);
            }
            last_random_b &= (static_cast<std::uint64_t>(1) << 61) - 1; // 61 random bits; reserve the top rand_b bit as 0 for increment headroom
        }

        random_a = last_random_a;
        random_b = last_random_b;
    }

    for (std::size_t i = 0; i < 6; ++i) {
        array[i] = std::byte(timestamp_millis >> ((5 - i) * 8));
    }

    array[6] = std::byte(0b01110000) | (std::byte(random_a >> 8) & std::byte(0b00001111)); // version 7 for first 4 bits, then bits 8-11 of random_a
    array[7] = std::byte(random_a); // least significant 8 bits of random_a

    array[8] = std::byte(0b10000000) | (std::byte((random_b >> 56) & 0b00111111)); // variant 0b10 for first 2 bits, then bits 56-61 of random_b
    for (std::size_t i = 0; i < 7; ++i) {
        array[9 + i] = std::byte(random_b >> ((6 - i) * 8));
    }

    return uuid(array);
}

std::chrono::system_clock::time_point uuid::timestamp_uuidv7() const {
    std::uint64_t timestamp_millis = 0;
    for (std::size_t i = 0; i < 6; ++i) {
        timestamp_millis = (timestamp_millis << 8) | static_cast<std::uint64_t>(m_bytes[i]);
    }

    constexpr auto max_time_point_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::time_point::max().time_since_epoch()).count();
    if (max_time_point_millis < 0 || static_cast<std::uint64_t>(max_time_point_millis) < timestamp_millis) {
        throw std::range_error(std::format("uuid::timestamp: timestamp {} is not representable", timestamp_millis));
    }

    return std::chrono::system_clock::time_point(std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(timestamp_millis)));
}

} // namespace m03gagbhtft23yhjwpp881tfmc_uuid
