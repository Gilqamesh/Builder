#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_H
# define M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <cstdint>
# include <string>
# include <vector>

namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain {

/**
 * Library artifact kind to produce.
 */
enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

/**
 * Libraries passed to the linker as one group.
 *
 * Set static_library_group for mutually dependent static archives.
 */
struct link_input_group_t {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> libraries;
    bool static_library_group;
};

/**
 * Linker inputs passed in group order.
 */
struct link_inputs_t {
    std::vector<link_input_group_t> groups;
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
 * Compiles source_files into a static or shared library at output_path.
 *
 * The returned path is output_path. For static libraries, pass empty link_inputs.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    library_type_t library_type,
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

#endif // M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_H
