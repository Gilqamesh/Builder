#include "uuidv7.h"

#include <chrono>
#include <stdexcept>
#include <format>
#include <mutex>

#include <cstring>
#include <cerrno>
#include <cstddef>

#include <sys/random.h>
#include <sys/types.h>

namespace uuidv7 {

// RFC9562 5.7: UUID Version 7
// 48 bits timestamp, 4 bits version (0b0111), 12 bits rand_a, 2 bits variant (0b10), 62 bits rand_b
uuidv7 uuidv7::generate() {
    std::array<std::uint8_t, 16> array;
    std::size_t index = 0;
    while (index < array.size()) {
        ssize_t bytes_copied = getrandom(array.data() + index, array.size() - index, 0);
        if (bytes_copied < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::format("uuidv7::generate: getrandom failed: {}", std::strerror(errno)));
        }

        if (bytes_copied == 0) {
            throw std::runtime_error("uuidv7::generate: getrandom returned 0 bytes");
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
        throw std::runtime_error("uuidv7::generate: system clock is before the Unix epoch");
    }

    auto timestamp_millis = static_cast<std::uint64_t>(millis);

    auto random_a = static_cast<std::uint16_t>(0);
    auto random_b = static_cast<std::uint64_t>(0);

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (initialized && timestamp_millis <= last_timestamp_millis) {
            timestamp_millis = last_timestamp_millis;

            constexpr auto max_62_bit_value = (static_cast<std::uint64_t>(1) << 62) - 1;
            if (last_random_b == max_62_bit_value) {
                throw std::runtime_error("uuidv7::generate: too many UUIDs generated in the same millisecond");
            }

            ++last_random_b;
        } else {
            initialized = true;
            last_timestamp_millis = timestamp_millis;

            last_random_a = static_cast<std::uint16_t>(((array[6] & 0x0F) << 8) | array[7]); // 12 random bits from random bytes

            last_random_b = static_cast<std::uint64_t>(array[8] & 0x3F);
            for (std::size_t i = 0; i < 7; ++i) {
                last_random_b = (last_random_b << 8) | array[9 + i];
            }
            last_random_b &= (static_cast<std::uint64_t>(1) << 61) - 1; // 61 random bits; reserve the top rand_b bit as 0 for increment headroom
        }

        random_a = last_random_a;
        random_b = last_random_b;
    }

    for (std::size_t i = 0; i < 6; ++i) {
        array[i] = static_cast<std::uint8_t>((timestamp_millis >> ((5 - i) * 8)) & 0xFF);
    }

    array[6] = static_cast<std::uint8_t>(0x70 | ((random_a >> 8) & 0x0F)); // version 7 for first 4 bits, then bits 8-11 of random_a
    array[7] = static_cast<std::uint8_t>(random_a & 0xFF); // least significant 8 bits of random_a

    array[8] = static_cast<std::uint8_t>(0x80 | ((random_b >> 56) & 0x3F)); // variant 0b10 for first 2 bits, then bits 56-61 of random_b
    for (std::size_t i = 0; i < 7; ++i) {
        array[9 + i] = static_cast<std::uint8_t>((random_b >> ((6 - i) * 8)) & 0xFF);
    }

    return uuidv7(array);
}

std::span<const std::uint8_t> uuidv7::bytes() const {
    return m_bytes;
}

uuidv7::uuidv7(std::array<std::uint8_t, 16> bytes):
    m_bytes(bytes)
{
}

} // namespace uuidv7
