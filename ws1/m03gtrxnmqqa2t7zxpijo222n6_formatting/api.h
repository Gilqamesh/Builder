#ifndef M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_API_H
# define M03GTRXNMQQA2T7ZXPIJO222N6_FORMATTING_API_H

# if !defined(__cpp_impl_reflection) || __cpp_impl_reflection < 202506L
#  error "Structural formatting requires C++26 reflection (GCC 16: -std=c++26 -freflection)."
# endif

# include <cstddef>
# include <format>
# include <meta>
# include <string>
# include <string_view>
# include <type_traits>
# include <utility>

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

/**
 * @brief Supplies structural formatting through inheritance by explicit std::formatter specializations.
 *
 * Records print Type{member=value, ...}, with direct public bases before direct
 * members, each in declaration order. A base uses its own formatter. Empty records
 * print Type{}. Names retain their source spelling; template argument names are
 * not included. Member values use their existing formatters.
 *
 * Enums print the first declared enumerator matching the value, or invalid(n)
 * using the numeric underlying value. Flags and semantic aliases need custom
 * formatters when that representation is insufficient.
 *
 * Only the empty format specification is supported. Records with inaccessible
 * subobjects, unions, and subobjects lacking a const-compatible formatter require
 * a custom specialization; inaccessible state is never silently omitted. Values
 * are borrowed through const references. No pointer dereferencing is performed.
 */
struct reflected_formatter_t {
    constexpr auto parse(std::format_parse_context& ctx);

    template <typename T>
    auto format(const T& formatted, auto& ctx) const;
};

} // namespace m03gtrxnmqqa2t7zxpijo222n6_formatting

namespace std {

template <>
struct formatter<m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t>;

} // namespace std

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

constexpr auto reflected_formatter_t::parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
        throw std::format_error("reflected_formatter_t requires an empty format specification");
    }
    return it;
}

template <typename T>
auto reflected_formatter_t::format(const T& formatted, auto& ctx) const {
    auto out = ctx.out();
    if constexpr (std::is_enum_v<T>) {
        bool matched = false;
        template for (constexpr auto enumerator : std::define_static_array(std::meta::enumerators_of(^^T))) {
            if (!matched && formatted == [:enumerator:]) {
                out = std::format_to(out, "{}", std::meta::identifier_of(enumerator));
                matched = true;
            }
        }
        if (!matched) {
            // Promote character-sized underlying types so the fallback is numeric.
            out = std::format_to(out, "invalid({})", +std::to_underlying(formatted));
        }
    } else if constexpr (std::is_class_v<T>) {
        static constexpr auto access = std::meta::access_context::unprivileged();
        static_assert(!std::meta::has_inaccessible_subobjects(^^T, access),
            "reflected_formatter_t requires public subobjects; provide a custom formatter for private state");
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
        template for (constexpr auto base : bases) {
            using base_t = [:std::meta::type_of(base):];
            static_assert(std::formattable<const base_t, char>,
                "reflected_formatter_t requires a formatter for each base");
        }
        template for (constexpr auto member : members) {
            using member_t = std::remove_reference_t<decltype((formatted.[:member:]))>;
            static_assert(std::formattable<const member_t, char>,
                "reflected_formatter_t requires a formatter for each member");
        }
        static constexpr auto pattern = std::define_static_string([] {
            std::string result(type_name);
            result += "{{";
            bool first = true;
            for (std::size_t index = 0; index < bases.size(); ++index) {
                if (!first) {
                    result += ", ";
                }
                first = false;
                result += "{}";
            }
            for (auto member : members) {
                if (!first) {
                    result += ", ";
                }
                first = false;
                result += std::meta::has_identifier(member) ? std::meta::identifier_of(member) : "(unnamed-member)";
                result += "={}";
            }
            result += "}}";
            return result;
        }());
        out = [&]<std::size_t... Base, std::size_t... Member>(std::index_sequence<Base...>, std::index_sequence<Member...>) {
            return std::format_to(out, pattern, formatted.[:bases[Base]:]...,
                static_cast<const std::remove_reference_t<decltype((formatted.[:members[Member]:]))>&>(formatted.[:members[Member]:])...);
        }(std::make_index_sequence<bases.size()>{}, std::make_index_sequence<members.size()>{});
    } else {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>,
            "reflected_formatter_t supports records and enums; unions require a custom formatter");
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
