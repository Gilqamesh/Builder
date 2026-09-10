#ifndef M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_API_H
# define M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_API_H

# if !defined(__cpp_impl_reflection) || __cpp_impl_reflection < 202506L
#  error "Structural formatting requires C++26 reflection (GCC 16: -std=c++26 -freflection)."
# endif

# include <cstddef>
# include <format>
# include <meta>
# include <ranges>
# include <string>
# include <string_view>
# include <tuple>
# include <type_traits>
# include <utility>

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

/**
 * @brief Produces structural diagnostics for records and named values for enums.
 *
 * Inherit this implementation in an explicit std::formatter specialization for
 * each selected type. Records expose all bases and nonstatic data members,
 * including private and protected subobjects, with bases before members in
 * declaration order. Each base and member must be formattable as a const value;
 * nested project types need their own formatter specializations. Unsupported
 * structures, including unions, require a custom formatter. Type owners also
 * supply custom formatting for semantic summaries or to omit internal state.
 *
 * Formatting borrows const values for the call and does not follow pointer
 * ownership graphs. Referenced data must remain valid while formatting; character
 * pointers and character arrays used as text must contain a valid null-terminated
 * string. Custom subobject formatters retain their own presentation.
 *
 * An empty specification or 0 expands records; 1 inlines leaf records; 2 uses a
 * single line; 3 additionally shortens text and ranges and rounds floating-point
 * values. Other specifications are rejected with std::format_error at runtime
 * (or diagnosed for checked format strings). Enum aliases use the first matching
 * enumerator; unnamed values print invalid(underlying-value).
 * Output is diagnostic, not a serialization or persistence format.
 *
 * @code{.cpp}
 * #include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>
 *
 * #include <format>
 * #include <iostream>
 *
 * namespace app {
 * struct point_t { int x; int y; };
 * } // namespace app
 *
 * template <>
 * struct std::formatter<app::point_t> : public m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};
 *
 * int main() {
 *     const app::point_t point{1, 2};
 *     std::cout << std::format("{}\n", point);   // Expanded
 *     std::cout << std::format("{:1}\n", point); // Inline leaf records
 *     std::cout << std::format("{:2}\n", point); // point_t { x: 1, y: 2 }
 *     std::cout << std::format("{:3}\n", point); // Shortened single line
 * }
 * @endcode
 */
struct reflected_formatter_t {
    /** @brief Selects the layout and shortening level from 0 through 3. */
    std::size_t level = 0;

    /** @brief Parses an empty specification or one level digit, resetting the default to 0. */
    constexpr auto parse(std::format_parse_context& ctx);

    /** @brief Writes the borrowed record or enum and returns the advanced output iterator. */
    template <typename T>
    auto format(const T& formatted, auto& ctx) const -> decltype(ctx.out());

private:
    template <typename T>
    static consteval bool reflected();

    template <typename T>
    static consteval bool text();

    template <typename T>
    static consteval bool range();

    template <typename T>
    static consteval bool tuple();

    template <typename T>
    static consteval bool composite();

    template <typename Out, typename T>
    Out write(Out out, const T& formatted, std::size_t depth) const;

    template <typename Out, typename T>
    Out write_record(Out out, const T& formatted, std::size_t depth) const;

    template <typename Out, typename T>
    Out write_range(Out out, const T& formatted, std::size_t depth) const;

    template <typename Out>
    Out write_text(Out out, std::string_view characters) const;
};

} // namespace m03gtrxnmqqa2t7zxpijo222n6_formatting

namespace std {

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t>;

} // namespace std

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

constexpr auto reflected_formatter_t::parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    level = 0;
    if (it != ctx.end() && '0' <= *it && *it <= '3') {
        level = static_cast<std::size_t>(*it++ - '0');
    }
    if (it != ctx.end() && *it != '}') {
        throw std::format_error("reflected_formatter_t requires an empty specification or a level from 0 to 3");
    }
    return it;
}

template <typename T>
auto reflected_formatter_t::format(const T& formatted, auto& ctx) const -> decltype(ctx.out()) {
    auto out = ctx.out();
    out = write_record(out, formatted, 0);
    return out;
}

template <typename T>
consteval bool reflected_formatter_t::reflected() {
    // Distinguish inherited format() from custom replacements.
    if constexpr (requires { &std::formatter<T>::template format<T, std::format_context>; }) {
        return std::is_same_v<decltype(&std::formatter<T>::template format<T, std::format_context>), decltype(&reflected_formatter_t::template format<T, std::format_context>)>;
    } else {
        return false;
    }
}

template <typename T>
consteval bool reflected_formatter_t::text() {
    if constexpr (std::is_class_v<T> && std::meta::has_template_arguments(^^T)) {
        return std::meta::template_of(^^T) == ^^std::basic_string || std::meta::template_of(^^T) == ^^std::basic_string_view;
    } else {
        return std::is_same_v<T, const char*> || std::is_same_v<T, char*> || (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>);
    }
}

template <typename T>
consteval bool reflected_formatter_t::range() {
    if constexpr (!text<T>() && std::ranges::input_range<const T>
        && !std::is_base_of_v<reflected_formatter_t, std::formatter<T>>) {
        return requires (std::formatter<T> formatter) { formatter.set_separator(std::string_view{}); } || std::format_kind<T> == std::range_format::map || std::format_kind<T> == std::range_format::set;
    } else {
        return false;
    }
}

template <typename T>
consteval bool reflected_formatter_t::tuple() {
    return !range<T>() && requires (std::formatter<T> formatter) {
        std::tuple_size<T>::value;
        formatter.set_separator(std::string_view{});
    } && !std::is_base_of_v<reflected_formatter_t, std::formatter<T>>;
}

template <typename T>
consteval bool reflected_formatter_t::composite() {
    return (reflected<T>() && std::is_class_v<T>) || range<T>() || tuple<T>();
}

template <typename Out, typename T>
Out reflected_formatter_t::write(Out out, const T& formatted, std::size_t depth) const {
    if constexpr (reflected<T>()) {
        out = write_record(out, formatted, depth);
    } else if constexpr (std::is_base_of_v<reflected_formatter_t, std::formatter<T>>) {
        out = std::format_to(out, "{}", formatted);
    } else if constexpr (text<T>()) {
        if constexpr (std::is_class_v<T>) {
            out = write_text(out, std::string_view(formatted.data(), formatted.size()));
        } else {
            out = write_text(out, std::string_view(formatted));
        }
    } else if constexpr (std::is_same_v<T, char>) {
        out = std::format_to(out, "{:?}", formatted);
    } else if constexpr (std::is_floating_point_v<T>) {
        if (level == 3) {
            out = std::format_to(out, "{:.3f}", formatted);
        } else {
            out = std::format_to(out, "{}", formatted);
        }
    } else if constexpr (range<T>()) {
        out = write_range(out, formatted, depth);
    } else if constexpr (tuple<T>()) {
        out = std::format_to(out, "(");
        std::apply([&](const auto&... elements) {
            std::size_t index = 0;
            ((out = std::format_to(out, "{}", index++ == 0 ? "" : ", "), out = write(out, elements, depth)), ...);
        }, formatted);
        out = std::format_to(out, ")");
    } else {
        out = std::format_to(out, "{}", formatted);
    }
    return out;
}

template <typename Out, typename T>
Out reflected_formatter_t::write_record(Out out, const T& formatted, std::size_t depth) const {
    if constexpr (std::is_enum_v<T>) {
        bool matched = false;
        template for (constexpr auto enumerator : std::define_static_array(std::meta::enumerators_of(^^T))) {
            if (!matched && formatted == [:enumerator:]) {
                out = std::format_to(out, "{}", std::meta::identifier_of(enumerator));
                matched = true;
            }
        }
        if (!matched) {
            out = std::format_to(out, "invalid({})", +std::to_underlying(formatted));
        }
    } else if constexpr (std::is_class_v<T>) {
        static constexpr auto access = std::meta::access_context::unchecked();
        static constexpr auto type_name = [] {
            if constexpr (std::meta::has_identifier(^^T)) {
                return std::meta::identifier_of(^^T);
            } else if constexpr (std::meta::has_template_arguments(^^T)) {
                return std::meta::identifier_of(std::meta::template_of(^^T));
            } else {
                return std::string_view("(unnamed-type)");
            }
        }();
        static constexpr auto bases = std::define_static_array(std::meta::bases_of(^^T, access));
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, access));
        static constexpr bool flat = [] {
            bool result = true;
            template for (constexpr auto base : bases) {
                using base_t = [:std::meta::type_of(base):];
                static_assert(std::formattable<const base_t, char>, "reflected_formatter_t requires a formatter for each base");
                result = result && !composite<base_t>();
            }
            template for (constexpr auto member : members) {
                using member_t = std::remove_reference_t<decltype((formatted.[:member:]))>;
                static_assert(std::formattable<const member_t, char>, "reflected_formatter_t requires a formatter for each member");
                result = result && !composite<std::remove_cv_t<member_t>>();
            }
            return result;
        }();
        constexpr bool empty = bases.empty() && members.empty();
        const bool multiline = !empty && (level == 0 || (level == 1 && !flat));
        out = std::format_to(out, "{}{}{{", type_name, empty ? "" : " ");
        bool first = true;
        const auto separator = [&] {
            if (!first) {
                out = std::format_to(out, ",");
            }
            first = false;
            if (multiline) {
                out = std::format_to(out, "\n{:>{}}", "", (depth + 1) * 2);
            } else {
                out = std::format_to(out, " ");
            }
        };
        template for (constexpr auto base : bases) {
            separator();
            out = write(out, formatted.[:base:], depth + 1);
        }
        template for (constexpr auto member : members) {
            separator();
            constexpr auto name = std::meta::has_identifier(member) ? std::meta::identifier_of(member) : "(unnamed-member)";
            out = std::format_to(out, "{}: ", name);
            // Explicit const binding also materializes bit-fields without copying normal members.
            using member_t = std::remove_reference_t<decltype((formatted.[:member:]))>;
            out = write(out, static_cast<const member_t&>(formatted.[:member:]), depth + 1);
        }
        if (multiline) {
            out = std::format_to(out, "\n{:>{}}", "", depth * 2);
        } else if constexpr (!empty) {
            out = std::format_to(out, " ");
        }
        out = std::format_to(out, "}}");
    } else {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>, "reflected_formatter_t supports records and enums; unions require a custom formatter");
    }
    return out;
}

template <typename Out, typename T>
Out reflected_formatter_t::write_range(Out out, const T& formatted, std::size_t depth) const {
    using element_t = std::remove_cvref_t<std::ranges::range_reference_t<const T>>;
    constexpr auto kind = std::format_kind<T>;
    constexpr bool associative = kind == std::range_format::map || kind == std::range_format::set;
    std::size_t count = 0;
    if constexpr (std::ranges::sized_range<const T>) {
        count = std::ranges::size(formatted);
    } else if constexpr (std::ranges::forward_range<const T>) {
        count = static_cast<std::size_t>(std::ranges::distance(formatted));
    }
    const bool multiline = level < 2 && (composite<element_t>() || 8 < count
        || (!std::ranges::sized_range<const T> && !std::ranges::forward_range<const T>));
    out = std::format_to(out, "{}", associative ? "{" : "[");
    std::size_t visited = 0;
    for (const auto& element : formatted) {
        if (level != 3 || visited < 3) {
            if (visited != 0) {
                out = std::format_to(out, ",");
            }
            if (multiline) {
                out = std::format_to(out, "\n{:>{}}", "", (depth + 1) * 2);
            } else if (visited != 0) {
                out = std::format_to(out, " ");
            }
            if constexpr (kind == std::range_format::map) {
                out = write(out, std::get<0>(element), depth + 1);
                out = std::format_to(out, ": ");
                out = write(out, std::get<1>(element), depth + 1);
            } else {
                out = write(out, element, depth + 1);
            }
        }
        ++visited;
        if constexpr (std::ranges::sized_range<const T> || std::ranges::forward_range<const T>) {
            if (level == 3 && visited == 3) {
                visited = count;
                break;
            }
        }
    }
    if (level == 3 && 3 < visited) {
        out = std::format_to(out, ", … (+{} items)", visited - 3);
    }
    if (multiline && visited != 0) {
        out = std::format_to(out, "\n{:>{}}", "", depth * 2);
    }
    out = std::format_to(out, "{}", associative ? "}" : "]");
    return out;
}

template <typename Out>
Out reflected_formatter_t::write_text(Out out, std::string_view characters) const {
    constexpr std::size_t prefix_size = 32;
    if (level == 3 && prefix_size < characters.size()) {
        std::string shortened(characters.substr(0, prefix_size));
        shortened += "…";
        out = std::format_to(out, "{:?} (length: {})", shortened, characters.size());
    } else {
        out = std::format_to(out, "{:?}", characters);
    }
    return out;
}

} // namespace m03gtrxnmqqa2t7zxpijo222n6_formatting

namespace std {

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

} // namespace std

#endif // M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_API_H
