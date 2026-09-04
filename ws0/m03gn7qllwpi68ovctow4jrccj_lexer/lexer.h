#ifndef M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H
# define M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H

# include <filesystem>
# include <istream>
# include <vector>

namespace m03gn7qllwpi68ovctow4jrccj_lexer {

/**
 * @brief Returns the paths named by quoted and angle-bracket include directives in ifs.
 */
std::vector<std::filesystem::path> include_paths(std::istream& ifs);

} // namespace m03gn7qllwpi68ovctow4jrccj_lexer

#endif // M03GN7QLLWPI68OVCTOW4JRCCJ_LEXER_LEXER_H
