#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H
# define M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>
# include <vector>

namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain {

/**
 * Linker inputs passed in order.
 */
struct link_inputs_t {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> libraries;
};

/**
 * Preprocessor define passed to the compiler as -Dkey="value".
 *
 * The key must be a valid C/C++ preprocessor identifier.
 */
class define_t {
public:
    define_t(std::string key, std::string value);

    const std::string& key() const;
    const std::string& value() const;

private:
    std::string m_key;
    std::string m_value;
};

/**
 * Compiles source_files into a shared library at output_path.
 *
 * The returned path is output_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
);

/**
 * Compiles source_files into an executable at output_path.
 *
 * The returned path is output_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_binary(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
);

} // namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain

#endif // M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H
