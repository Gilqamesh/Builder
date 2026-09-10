#ifndef M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H
# define M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H

# include <span>
# include <string>
# include <string_view>
# include <vector>
# include <cstdint>
# include <format>
# include <cstddef>
# include <memory>
# include <filesystem>

namespace m03gagbht2l61mj6qitacwbmea_byte_stream {

/**
 * @brief Owns mutable bytes with binary file I/O and big-endian unsigned integer radix conversion.
 *
 * Radix conversion preserves the integer value, not leading zero bytes or a fixed width.
 * Copies own independent storage; moves transfer storage ownership.
 *
 * @code{.cpp}
 * #include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
 *
 * #include <cassert>
 * #include <cstddef>
 * #include <vector>
 *
 * int main() {
 *     using m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t;
 *     const byte_stream_t padded_byte_stream(std::vector<std::byte> {
 *         std::byte { 0x00 }, std::byte { 0x00 }, std::byte { 0x2a }
 *     });
 *     const auto text = padded_byte_stream.to_radix(16);
 *     const auto decoded_byte_stream = byte_stream_t::from_radix(text, 16);
 *     assert(text == "2a");
 *     assert(padded_byte_stream.size() == 3);
 *     assert(decoded_byte_stream.size() == 1); // The original width is lost.
 *     const auto bytes = decoded_byte_stream.bytes(); // Borrows decoded_byte_stream.
 *     assert(bytes[0] == std::byte { 0x2a });
 *     assert(byte_stream_t::from_radix("0000", 16).empty());
 * }
 * @endcode
 */
class byte_stream_t {
public:
    /**
     * @brief Constructs an empty byte_stream_t.
     */
    byte_stream_t();

    byte_stream_t(const byte_stream_t& other);
    byte_stream_t(byte_stream_t&& other) noexcept;
    byte_stream_t& operator=(const byte_stream_t& other);
    byte_stream_t& operator=(byte_stream_t&& other) noexcept;

    /**
     * @brief Copies a byte sequence into independently owned storage.
     *
     * @param bytes The byte sequence to store.
     */
    explicit byte_stream_t(std::span<const std::byte> bytes);

    /** @brief Takes ownership of the by-value vector's storage. */
    byte_stream_t(std::vector<std::byte> bytes);

    /**
     * @brief Borrows a read-only view of the stored bytes.
     *
     * The view does not extend storage lifetime. Non-self assignment, clear() and
     * destruction invalidate views of the previous contents. push_back() and append()
     * invalidate them if storage reallocates; otherwise an existing span keeps its old size.
     * Moving the stream transfers the storage and its borrowed views to the destination's
     * lifetime. Reacquire a view after operations that change the stored sequence.
     */
    std::span<const std::byte> bytes() const& noexcept;
    /**
     * @brief Borrows a writable view whose changes modify the stored bytes.
     *
     * The same lifetime and invalidation rules apply as for the read-only bytes() view.
     */
    std::span<std::byte> bytes() & noexcept;

    std::span<const std::byte> bytes() const && = delete;
    std::span<std::byte> bytes() && = delete;

    /** @brief Appends one byte, growing storage as needed. */
    void push_back(std::byte value);
    /**
     * @brief Copies bytes onto the end of the sequence, allowing a view into this stream.
     *
     * The input is copied before growth, so appending bytes() duplicates the sequence.
     */
    void append(std::span<const std::byte> bytes);
    /**
     * @brief Copies the bytes of other onto the end of the sequence.
     *
     * A distinct other retains its contents; this overload does not transfer its storage.
     */
    void append(byte_stream_t&& other);
    /** @brief Removes all bytes and invalidates borrowed views of them. */
    void clear() noexcept;

    /** @brief Returns the number of stored bytes. */
    std::size_t size() const noexcept;
    /** @brief Reports whether the sequence contains no bytes. */
    bool empty() const noexcept;

    /**
     * @brief Constructs a byte_stream_t from a lowercase unsigned integer string in the given radix.
     *
     * Leading zeros are ignored.
     * Empty input and "0" produce an empty byte sequence.
     *
     * @param text The string to parse. Only characters 0-9 and a-z are accepted.
     * @param radix The radix to use. Must be between 2 and 36.
     * @return A byte_stream_t containing the parsed value as big-endian bytes.
     *
     * @throws std::invalid_argument If radix is outside [2, 36], a character is not
     * a lowercase digit, or a digit is outside the chosen radix. No sign, whitespace
     * or special radix-prefix syntax is recognized.
     */
    static byte_stream_t from_radix(std::string_view text, uint32_t radix);

    /**
     * @brief Converts the bytes to a lowercase unsigned integer string in the given radix.
     *
     * The bytes are interpreted as a big-endian unsigned integer.
     * Leading zero bytes do not affect the result.
     * Empty input and all-zero input are returned as "0".
     *
     * @param radix The radix to use. Must be between 2 and 36.
     * @return The radix string. Only characters 0-9 and a-z are used.
     *
     * @throws std::invalid_argument If radix is outside [2, 36].
     */
    std::string to_radix(uint32_t radix) const;

    /**
     * @brief Constructs a byte_stream_t from a file.
     *
     * @param path The path to the file to read.
     * @return A byte_stream_t containing the file contents.
     *
     * Reads in binary mode without interpreting or removing bytes.
     * @throws std::ios_base::failure If the file cannot be opened or read.
     * @throws std::length_error If the read would exceed the maximum storage size.
     */
    static byte_stream_t from_file(const std::filesystem::path& path);

    /**
     * @brief Writes the bytes to a file.
     *
     * @param path The path to the file to write.
     *
     * Opens in binary mode and truncates an existing file, including for an empty stream.
     * A failed write can leave a partial file; this is not an atomic replacement.
     * @throws std::ios_base::failure If opening, writing or closing the file fails.
     */
    void to_file(const std::filesystem::path& path) const;

private:
    std::vector<std::byte> m_bytes;
};

} // namespace m03gagbht2l61mj6qitacwbmea_byte_stream

namespace std {

template <>
struct formatter<m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid byte_stream_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t& byte_stream, auto& ctx) const {
        auto out = ctx.out();
        
        out = std::format_to(out, "0x");
        for (const auto& byte : byte_stream.bytes()) {
            out = std::format_to(out, "{:02x}", static_cast<uint8_t>(byte));
        }

        return out;
    }
};

} // namespace std


#endif // M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H
