#ifndef M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H
# define M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H

# include <span>
# include <string>
# include <string_view>
# include <vector>
# include <cstdint>
# include <format>
# include <cstddef>

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

    /**
     * @brief Constructs a byte_stream_t from a byte sequence.
     *
     * @param bytes The byte sequence to store.
     */
    explicit byte_stream_t(std::span<const std::byte> bytes);

    /**
     * @brief Returns a view over the stored bytes.
     *
     * @return A non-owning view over the stored bytes.
     */
    std::span<const std::byte> bytes() const noexcept;

    /**
     * @brief Returns the number of bytes stored.
     *
     * @return The number of bytes stored.
     */
    size_t size() const noexcept;

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
     * @throws std::invalid_argument on invalid input.
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
     * @throws std::invalid_argument on invalid input.
     */
    std::string to_radix(uint32_t radix) const;

private:
    std::vector<std::byte> m_bytes;
};

} // namespace m03gagbht2l61mj6qitacwbmea_byte_stream

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

        out = std::format_to(out, "0x{}", byte_stream.to_radix(16));

        return out;
    }
};

} // namespace std


#endif // M03GAGBHT2L61MJ6QITACWBMEA_BYTE_STREAM_BYTE_STREAM_H
