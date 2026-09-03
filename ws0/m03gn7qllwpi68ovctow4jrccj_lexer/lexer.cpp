#include "lexer.h"

#include <cctype>
#include <string>

namespace m03gn7qllwpi68ovctow4jrccj_lexer {

static bool is_at_end(std::istream& ifs);
static bool is_whitespace(std::istream& ifs);
static bool eat_if(std::istream& ifs, std::istream::char_type expected);
static std::istream::char_type eat(std::istream& ifs);
static std::istream::char_type peek(std::istream& ifs);
static void skip_whitespace(std::istream& ifs);
static void skip_comment(std::istream& ifs);
static void skip_string(std::istream& ifs);
static std::string read_identifier(std::istream& ifs);
static std::filesystem::path read_path(std::istream& ifs, std::istream::char_type expected);

static bool is_at_end(std::istream& ifs) {
    return ifs.peek() == std::istream::traits_type::eof();
}

static bool is_whitespace(std::istream& ifs) {
    return std::isspace(static_cast<unsigned char>(peek(ifs)));
}

static bool eat_if(std::istream& ifs, std::istream::char_type expected) {
    if (is_at_end(ifs) || peek(ifs) != expected) {
        return false;
    }

    eat(ifs);
    return true;
}

static std::istream::char_type eat(std::istream& ifs) {
    const auto c = ifs.get();
    return c == std::istream::traits_type::eof() ? '\0' : std::istream::traits_type::to_char_type(c);
}

static std::istream::char_type peek(std::istream& ifs) {
    const auto c = ifs.peek();
    return c == std::istream::traits_type::eof() ? '\0' : std::istream::traits_type::to_char_type(c);
}

static void skip_whitespace(std::istream& ifs) {
    while (!is_at_end(ifs) && is_whitespace(ifs)) {
        eat(ifs);
    }
}

static void skip_comment(std::istream& ifs) {
    eat(ifs);

    switch (peek(ifs)) {
        case '/': {
            while (!is_at_end(ifs) && peek(ifs) != '\n') {
                eat(ifs);
            }
        } break;
        case '*': {
            eat(ifs);

            auto previous = std::istream::char_type();
            while (!is_at_end(ifs)) {
                const auto current = eat(ifs);
                if (previous == '*' && current == '/') {
                    break;
                }
                previous = current;
            }
        } break;
    }
}

static void skip_string(std::istream& ifs) {
    eat(ifs);

    while (!is_at_end(ifs)) {
        if (eat_if(ifs, '\\')) {
            eat(ifs);
        } else if (eat_if(ifs, '"')) {
            break;
        } else {
            eat(ifs);
        }
    }
}

static std::string read_identifier(std::istream& ifs) {
    std::string result;

    while (!is_at_end(ifs)) {
        const auto c = peek(ifs);
        if (c != '_' && !std::isalnum(static_cast<unsigned char>(c))) {
            break;
        }
        result.push_back(eat(ifs));
    }

    return result;
}

static std::filesystem::path read_path(std::istream& ifs, std::istream::char_type expected) {
    std::string result;

    while (!is_at_end(ifs) && peek(ifs) != expected && peek(ifs) != '\n') {
        result.push_back(eat(ifs));
    }

    if (!eat_if(ifs, expected)) {
        return std::filesystem::path();
    }

    return result;
}

std::vector<std::filesystem::path> include_paths(std::istream& ifs) {
    std::vector<std::filesystem::path> result;

    while (!is_at_end(ifs)) {
        skip_whitespace(ifs);

        if (is_at_end(ifs)) {
            break;
        }

        switch (peek(ifs)) {
            case '/': {
                skip_comment(ifs);
            } continue;
            case '"': {
                skip_string(ifs);
            } continue;
        }

        if (!eat_if(ifs, '#')) {
            eat(ifs);
            continue;
        }

        skip_whitespace(ifs);

        if (read_identifier(ifs) != "include") {
            continue;
        }

        skip_whitespace(ifs);

        if (eat_if(ifs, '<')) {
            const auto path = read_path(ifs, '>');
            if (!path.empty()) {
                result.push_back(path);
            }
        } else if (eat_if(ifs, '"')) {
            const auto path = read_path(ifs, '"');
            if (!path.empty()) {
                result.push_back(path);
            }
        }
    }

    return result;
}

} // namespace m03gn7qllwpi68ovctow4jrccj_lexer
