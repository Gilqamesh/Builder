#ifndef FUNCTION_ID
# define FUNCTION_ID

# include "libc.h"

struct function_id_t {
    std::string ns;
    std::string name;
    std::optional<std::chrono::system_clock::time_point> creation_time;

    bool operator==(const function_id_t& other) const;
};

template <>
struct std::hash<function_id_t> {
    size_t operator()(const function_id_t& fid) const noexcept {
        size_t h1 = std::hash<std::string>{}(fid.ns);
        size_t h2 = std::hash<std::string>{}(fid.name);
        size_t h3 = fid.creation_time.transform([](const auto& tp) {
            return std::hash<int64_t>{}(std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count());
        }).value_or(0);

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

#endif // FUNCTION_ID
