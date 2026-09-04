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
 * @brief Owns a byte sequence and provides alternate representations for it.
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
     * @brief Constructs a byte_stream_t from a byte sequence.
     *
     * @param bytes The byte sequence to store.
     */
    explicit byte_stream_t(std::span<const std::byte> bytes);

    byte_stream_t(std::vector<std::byte> bytes);

    std::span<const std::byte> bytes() const& noexcept;
    std::span<std::byte> bytes() & noexcept;

    std::span<const std::byte> bytes() const && = delete;
    std::span<std::byte> bytes() && = delete;

    void push_back(std::byte value);
    void append(std::span<const std::byte> bytes);
    void append(byte_stream_t&& other);
    void clear() noexcept;

    std::size_t size() const noexcept;
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
     * Fails on invalid input.
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
     * Fails on invalid input.
     */
    std::string to_radix(uint32_t radix) const;

    /**
     * @brief Constructs a byte_stream_t from a file.
     *
     * @param path The path to the file to read.
     * @return A byte_stream_t containing the file contents.
     *
     * Fails if the file cannot be opened or read.
     */
    static byte_stream_t from_file(const std::filesystem::path& path);

    /**
     * @brief Writes the bytes to a file.
     *
     * @param path The path to the file to write.
     *
     * Fails if the file cannot be opened or written.
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
