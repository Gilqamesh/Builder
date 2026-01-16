#ifndef FILESYSTEM_FILESYSTEM_H
# define FILESYSTEM_FILESYSTEM_H

# include <filesystem>
# include <functional>
# include <format>

/**
 * filesystem
 *
 * Controlled interface to the native filesystem.
 *
 * Design goals:
 * - Centralize all std::filesystem interaction
 * - Enforce path invariants via path_t
 * - Provide explicit, pruneable traversal
 *
 * All functions throw std::runtime_error on failure.
 */
namespace filesystem {

// TODO: predicate API for filesystem::find is too heavy

/**
 * relative_path_t
 * 
 * Invariants:
 * - Always relative
 *
 * Semantics:
 * - Represents a relative filesystem location
 */
class relative_path_t {
public:
    friend class path_t;

public:
    relative_path_t(const std::filesystem::path& relative_path);

    /**
     * Returns the native string representation.
    */
    const char* c_str() const;

    /**
     * Returns the string representation.
    */
    std::string string() const;

    /**
     * Returns the stem of the filename.
    */
    std::string stem() const;

    /**
     * Returns the file extension, including the leading dot.
    */
    std::string extension() const;

    /**
     * Sets a new file extension.
    */
    void extension(std::string_view new_extension);

    /**
     * Appends a postfix to the filename.
     * The resulting path must be a sibling of the base path.
    */
    relative_path_t operator+(std::string_view postfix) const;

    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_relative_path;
};

/**
 * path_t
 * 
 * Invariants:
 * - Always absolute
 * - Always lexically normalized
 *
 * Semantics:
 * - Represents a concrete filesystem location
 * - All path composition enforces containment
 */
class path_t {
public:
    friend class filesystem_t;

public:
    /**
     * Constructs a normalized absolute path.
     */
    path_t(const std::filesystem::path& path);

    /**
     * Returns the parent directory.
     *
     * Throws if the path has no parent, i.e., path is root.
     */
    path_t parent() const;

    /**
     * Checks whether `other` is a strict lexical descendant of this path.
    */
    bool is_child(const path_t& other) const;

    /**
     * Returns the relative path from this path to `other`.
    */
    relative_path_t relative(const path_t& other) const;

    /**
     * Checks whether `sibling` shares the same parent directory as this path.
    */
    bool is_sibling(const path_t& sibling) const;

    /**
     * Returns the final path component.
     */
    std::string filename() const;

    /**
     * Returns the native string representation.
    */
    const char* c_str() const;

    /**
     * Returns the string representation.
    */
    std::string string() const;

    /**
     * Returns the stem of the filename.
    */
    std::string stem() const;

    /**
     * Returns the file extension, including the leading dot.
     */
    std::string extension() const;

    void extension(std::string_view new_extension);

    /**
     * Lexical equality comparison.
     */
    bool operator==(const path_t& other) const;

    /**
     * Joins a relative path component.
     *
     * Invariant:
     * - The resulting path must be a strict lexical child of the base path
     *
     * Throws if:
     * - The component is absolute
     * - The result escapes the base path
     * - The result is identical to the base path
     */
    path_t operator/(const relative_path_t& relative_path) const;

    /**
     * Appends a postfix to the filename.
     * The resulting path must be a sibling of the base path.
    */
    path_t operator+(std::string_view postfix) const;

    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_path;
};

/**
 * pretty_path_t
 *
 * Semantics:
 * - Non-owning formatting adapter for path_t
 *
 * Notes:
 * - Holds a reference to a path_t
 * - Intended for ephemeral formatting (e.g. logging, std::format)
 */
class pretty_path_t {
public:
    explicit pretty_path_t(const path_t& path);

    const std::string& string() const;
    const char* c_str() const;

private:
    std::string m_string;
};

struct find_include_predicate_t {
    find_include_predicate_t(std::function<bool(const path_t& path)>&& predicate);

    static find_include_predicate_t include_all;
    static find_include_predicate_t is_dir;
    static find_include_predicate_t is_regular;
    static find_include_predicate_t cpp_file;
    static find_include_predicate_t c_file;
    static find_include_predicate_t hpp_file;
    static find_include_predicate_t h_file;

    /**
     * Matches entries by basename.
     */
    static find_include_predicate_t filename(const std::string& name);

    /**
     * Matches a single path.
     */
    static find_include_predicate_t path(const path_t& target);

    std::function<bool(const path_t& path)> predicate;
    bool operator()(const path_t& path) const;

    find_include_predicate_t operator&&(find_include_predicate_t b) const;
    find_include_predicate_t operator||(find_include_predicate_t b) const;
    find_include_predicate_t operator!() const;
};

struct find_descend_predicate_t {
    find_descend_predicate_t(std::function<bool(const path_t& dir, size_t depth)>&& predicate);

    static find_descend_predicate_t descend_all;
    static find_descend_predicate_t descend_none;

    std::function<bool(const path_t& dir, size_t depth)> predicate;
    bool operator()(const path_t& dir, size_t depth) const;

    find_descend_predicate_t operator&&(find_descend_predicate_t b) const;
    find_descend_predicate_t operator||(find_descend_predicate_t b) const;
    find_descend_predicate_t operator!() const;
};

/**
 * For each encountered entry `e` in `dir`:
 *   - if `e` is a directory and `descend_predicate(e, depth)` is true, recurse into `e`
 *   - if `include_predicate(e)` is true, include `e` in the result
 * 
 * The filesystem structure must not be modified during traversal.
 */
std::vector<path_t> find(const path_t& dir, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate);

/**
 * Returns the canonical path, resolving all symbolic links.
*/
path_t canonical(const path_t& path);

/**
 * Copies a file or directory tree.
*/
void copy(const path_t& src, const path_t& dst);

/**
 * Updates the last modification timestamp or creates an empty file if it does not exist.
 * Throws if !exists(parent(path))
*/
void touch(const path_t& path);

/**
 * Creates all missing parent directories.
 */
void create_directories(const path_t& path);

/**
 * Creates a symbolic link.
 */
void create_symlink(const path_t& src, const path_t& dst);

/**
 * Creates a directory symbolic link.
 */
void create_directory_symlink(const path_t& src, const path_t& dst);

/**
 * Returns the current working directory.
*/
path_t current_path();

/**
 * Sets the current working directory.
*/
void current_path(const path_t& path);

/**
 * Checks whether a path exists.
 */
bool exists(const path_t& path);

/**
 * Returns the size of a regular file in bytes.
 */
std::uintmax_t file_size(const path_t& path);

/**
 * Returns the last modification timestamp.
 */
std::filesystem::file_time_type last_write_time(const path_t& path);

/**
 * Removes a single file or empty directory.
 */
bool remove(const path_t& path);

/**
 * Recursively removes a directory tree.
 *
 * Returns the number of removed filesystem objects.
 */
std::uintmax_t remove_all(const path_t& path);

/**
 * Renames a file or directory.
 * Throws if `from` does not exist or `to` already exists.
 * Does not overwrite.
 */
void rename_strict(const path_t& from, const path_t& to);

/**
 * Atomically renames `from` to `to`, replacing `to` if it exists.
 * Throws if `from` does not exist.
 */
void rename_replace(const path_t& from, const path_t& to);

/**
 * Checks whether the path refers to a regular file.
 */
bool is_regular_file(const path_t& path);

/**
 * Checks whether the path refers to a directory.
 */
bool is_directory(const path_t& path);

} // namespace filesystem

namespace std {

template <>
struct formatter<::filesystem::path_t> : formatter<std::string> {
    auto format(const ::filesystem::path_t& path, auto& ctx) const {
        return formatter<std::string>::format(path.string(), ctx);
    }
};

template <>
struct formatter<::filesystem::relative_path_t> : formatter<std::string> {
    auto format(const ::filesystem::relative_path_t& relative_path, auto& ctx) const {
        return formatter<std::string>::format(relative_path.string(), ctx);
    }
};

template <>
struct formatter<::filesystem::pretty_path_t> : formatter<std::string> {
    auto format(const ::filesystem::pretty_path_t& pretty_path, auto& ctx) const {
        return formatter<std::string>::format(pretty_path.string(), ctx);
    }
};

} // namespace std

#endif // FILESYSTEM_FILESYSTEM_H
