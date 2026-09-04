#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gn7qllwpi68ovctow4jrccj_lexer/lexer.h>

#include <functional>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace lexer = m03gn7qllwpi68ovctow4jrccj_lexer;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


static std::vector<std::string> generic_strings(const std::vector<std::filesystem::path>& paths) {
    std::vector<std::string> result;
    result.reserve(paths.size());
    for (const auto& path : paths) {
        result.push_back(path.generic_string());
    }
    return result;
}

int main() {
    return test::run([] {
        std::istringstream empty_input;
        test::expect(std::identity(), lexer::include_paths(empty_input).empty());

        std::istringstream input(
            "#include \"local/header.h\"\n"
            "# include\t<system/header.hpp>\n"
            "  #  include   \"path with spaces/file.h\"\n"
            "// #include \"ignored/line-comment.h\"\n"
            "/* #include <ignored/block-comment.h> */\n"
            "const char* text = \"#include \\\"ignored/string.h\\\"\";\n"
            "const char* escaped = \"\\\\\" #include <ignored/escaped-string.h>\";\n"
            "#define include_alias \"ignored/macro.h\"\n"
            "#included \"ignored/prefix.h\"\n"
            "#include HEADER_MACRO\n"
            "#include \"\"\n"
            "#include <>\n"
            "#include \"unterminated.h\n"
            "#include <also-unterminated.h\n"
            "#include \"after/malformed.h\"\n"
        );

        const auto paths = generic_strings(lexer::include_paths(input));
        test::expect(std::equal_to<>(), paths.size(), std::size_t(5));
        test::expect(std::equal_to<>(), paths[0], std::string("local/header.h"));
        test::expect(std::equal_to<>(), paths[1], std::string("system/header.hpp"));
        test::expect(std::equal_to<>(), paths[2], std::string("path with spaces/file.h"));
        test::expect(std::equal_to<>(), paths[3], std::string("ignored/escaped-string.h"));
        test::expect(std::equal_to<>(), paths[4], std::string("after/malformed.h"));

        std::istringstream adjacent(
            "#include\"first.h\"\n"
            "#include<second.hpp>\n"
            "#\n"
            "include \"not-a-directive.h\"\n"
        );
        const auto adjacent_paths = generic_strings(lexer::include_paths(adjacent));
        test::expect(std::equal_to<>(), adjacent_paths.size(), std::size_t(2));
        test::expect(std::equal_to<>(), adjacent_paths[0], std::string("first.h"));
        test::expect(std::equal_to<>(), adjacent_paths[1], std::string("second.hpp"));

        std::istringstream unterminated_comment(
            "#include \"before.h\"\n"
            "/* #include \"ignored.h\""
        );
        const auto before_comment = generic_strings(lexer::include_paths(unterminated_comment));
        test::expect(std::equal_to<>(), before_comment.size(), std::size_t(1));
        test::expect(std::equal_to<>(), before_comment[0], std::string("before.h"));
    });
}
