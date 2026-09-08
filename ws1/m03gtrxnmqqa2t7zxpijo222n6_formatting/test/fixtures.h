#ifndef M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_FIXTURES_H
# define M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_FIXTURES_H

# include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

# include <array>
# include <cstddef>
# include <cstdint>
# include <string>
# include <vector>

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

struct extent_t { std::size_t width; std::size_t height; };
struct extended_extent_t { std::size_t width; std::size_t height; std::size_t depth; };
struct empty_t {};
enum class state_t : std::int8_t { ready = 1, alias = ready, done = 7 };
enum class wide_state_t : std::uint64_t { high = 0xffffffffffffffffULL };
struct record_t { extent_t extent; state_t state; std::string label; std::array<int, 2> counts; };
struct base_t { int count; };
struct derived_t : base_t { int extra; };
struct left_t : virtual base_t {};
struct right_t : virtual base_t {};
struct diamond_t : left_t, right_t {};
struct bits_t { unsigned count : 3; unsigned : 0; unsigned active : 1; };
struct borrowed_t { const extent_t& extent; };
struct custom_t { int count; };
struct custom_record_t { custom_t custom; };
struct noncopyable_t {
    int count;
    explicit noncopyable_t(int count);
    noncopyable_t(const noncopyable_t&) = delete;
};
struct noncopyable_record_t { noncopyable_t noncopyable; };

template <typename T>
struct box_t { T item; };

struct summary_t { std::string label; double fraction; std::vector<record_t> records; bool enabled; };
struct custom_range_t : std::vector<int> {};

} // namespace m03gtrxnmqqa2t7zxpijo222n6_formatting

namespace std {

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::extent_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::extended_extent_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::empty_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::state_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::wide_state_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::record_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::base_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::derived_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::left_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::right_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::diamond_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::bits_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::borrowed_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_record_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::noncopyable_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::noncopyable_record_t>;

template <typename T>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::box_t<T>>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::summary_t>;

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_range_t>;

} // namespace std

namespace std {
template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::extent_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::extended_extent_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::empty_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::state_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::wide_state_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::record_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::base_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::derived_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::left_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::right_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::diamond_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::bits_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::borrowed_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {
    auto format(const m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_t& custom, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "custom:{}", custom.count);
        return out;
    }
};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_record_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::noncopyable_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::noncopyable_record_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <typename T>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::box_t<T>>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::summary_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_range_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {
    auto format(const m03gtrxnmqqa2t7zxpijo222n6_formatting::custom_range_t& custom_range, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "custom-range:{}", custom_range.size());
        return out;
    }
};

} // namespace std

#endif // M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_FIXTURES_H
