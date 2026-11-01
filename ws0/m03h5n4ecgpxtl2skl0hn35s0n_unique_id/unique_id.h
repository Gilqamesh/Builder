#ifndef M03H5N4ECGPXTL2SKL0HN35S0N_UNIQUE_ID_UNIQUE_ID_H
# define M03H5N4ECGPXTL2SKL0HN35S0N_UNIQUE_ID_UNIQUE_ID_H

# include <m03gagbhtft23yhjwpp881tfmc_uuid/uuid.h>

# include <cstddef>
# include <format>
# include <string>
# include <string_view>

namespace m03h5n4ecgpxtl2skl0hn35s0n_unique_id {

/** @brief Owns a UUIDv7 identity with one canonical base36 text representation. */
class unique_id_t {
public:
    static constexpr std::size_t encoded_size = 25;

    /**
     * @brief Parses exactly 25 lowercase base36 digits, including leading zeros.
     * @throws std::invalid_argument If text is malformed, exceeds 128 bits, or is not a valid UUIDv7.
     */
    explicit unique_id_t(std::string_view text);

    /** @brief Copies a UUIDv7; throws std::invalid_argument for an unsupported UUID. */
    explicit unique_id_t(const m03gagbhtft23yhjwpp881tfmc_uuid::uuid& uuid);

    /** @brief Generates an identity using the UUID module's UUIDv7 generator. Generation errors propagate. */
    static unique_id_t generate();

    /** @brief Encodes the network-order UUID bytes as 25 zero-padded lowercase base36 digits. */
    std::string string() const;

    m03gagbhtft23yhjwpp881tfmc_uuid::uuid uuid() const;

    bool operator==(const unique_id_t& other) const noexcept;

private:
    m03gagbhtft23yhjwpp881tfmc_uuid::uuid m_uuid;
};

} // namespace m03h5n4ecgpxtl2skl0hn35s0n_unique_id

namespace std {

template <>
struct formatter<m03h5n4ecgpxtl2skl0hn35s0n_unique_id::unique_id_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03h5n4ecgpxtl2skl0hn35s0n_unique_id::unique_id_t> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw format_error("unique_id_t requires an empty format specification");
        }
        return it;
    }

    auto format(const m03h5n4ecgpxtl2skl0hn35s0n_unique_id::unique_id_t& unique_id, auto& ctx) const {
        auto out = ctx.out();
        out = format_to(out, "{}", unique_id.string());
        return out;
    }
};

} // namespace std

#endif // M03H5N4ECGPXTL2SKL0HN35S0N_UNIQUE_ID_UNIQUE_ID_H
