#ifndef M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H
# define M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H

# include <filesystem>
# include <istream>
# include <vector>

namespace m03gn7qllwpi68ovctow4jrccj_lexer {

/**
 * @brief Extracts literal quoted and angle-bracket include paths from a source stream.
 *
 * Consumes ifs from its current position to end of input without retaining it.
 * Returns owning paths in encounter order, including duplicates, with delimiters
 * removed; paths are neither resolved against a directory nor checked for existence.
 * Spaces and tabs may separate #, include and the opening delimiter.
 *
 * Line/block comments and ordinary double-quoted strings are skipped. Macro include
 * operands are not expanded. Empty paths and paths without a closing delimiter
 * before newline or end of input are omitted, and scanning continues when possible.
 * Conditional preprocessing is not evaluated, so includes inside #if 0 are returned.
 * This is a lexical approximation: it does not validate directive line placement,
 * splice escaped newlines, or fully parse raw strings and character literals.
 *
 * Malformed directives produce no diagnostic. Stream errors follow ifs's exception
 * mask; without stream exceptions, a failed read can yield only the paths read so far.
 * The caller owns the stream and must provide exclusive access during the scan.
 *
 * @code{.cpp}
 * #include <m03gn7qllwpi68ovctow4jrccj_lexer/lexer.h>
 * #include <cassert>
 * #include <filesystem>
 * #include <sstream>
 * #include <vector>
 *
 * int main() {
 *     std::istringstream input(
 *         "#include \"local.h\"\n"
 *         "# include <library/api.h>\n"
 *         "#include HEADER_MACRO\n"
 *         "#include \"unterminated.h\n"
 *         "#if 0\n#include \"disabled.h\"\n#endif\n"
 *     );
 *     const auto paths = m03gn7qllwpi68ovctow4jrccj_lexer::include_paths(input);
 *     const std::vector<std::filesystem::path> expected {
 *         "local.h", "library/api.h", "disabled.h"
 *     };
 *     assert(paths == expected);
 * }
 * @endcode
 */
std::vector<std::filesystem::path> include_paths(std::istream& ifs);

} // namespace m03gn7qllwpi68ovctow4jrccj_lexer

#endif // M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H
