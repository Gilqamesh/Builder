#ifndef FUNCTION_ID
# define FUNCTION_ID

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

struct function_id_t {
    std::string ns;
    std::string name;
    std::chrono::system_clock::time_point creation_time;

    bool operator==(const function_id_t& other) const;

    explicit operator bool() const;

    static std::string to_string(const function_id_t& function_id);
    static function_id_t from_string(const std::string& str);
};

template <>
struct std::hash<function_id_t> {
    size_t operator()(const function_id_t& function_id) const noexcept {
        size_t h1 = std::hash<std::string>{}(function_id.ns);
        size_t h2 = std::hash<std::string>{}(function_id.name);
        size_t h3 = std::hash<int64_t>{}(function_id.creation_time.time_since_epoch().count());

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

template <>
struct std::formatter<function_id_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(const function_id_t& function_id, FormatContext& ctx) const {
        return std::format_to(ctx.out(), function_id_t::to_string(function_id));
    }
};

#endif // FUNCTION_ID
