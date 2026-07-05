#ifndef M03GAGBHSNUSI43ZOGOACGJ2EZ_FILESYSTEM_FILESYSTEM_H
# define M03GAGBHSNUSI43ZOGOACGJ2EZ_FILESYSTEM_FILESYSTEM_H

# include <filesystem>
# include <functional>
# include <format>

/**
 * Checked filesystem paths and operations.
 *
 * All functions throw std::runtime_error on failure.
 */
namespace m03gagbhsnusi43zogoacgj2ez_filesystem {

/**
 * Relative filesystem path.
 */
class relative_path_t {
public:
    /**
     * Constructs a normalized relative path.
     */
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
    const relative_path_t& extension(std::string_view new_extension);

    /**
     * Lexical equality comparison.
     */
    bool operator==(const relative_path_t& other) const;

    /**
     * Appends postfix to the filename and returns a sibling path.
     */
    relative_path_t operator+(std::string_view postfix) const;

    /**
     * Returns the wrapped std::filesystem path.
     */
    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_relative_path;
};

/**
 * Absolute normalized filesystem path.
 */
class path_t {
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

    /**
     * Sets a new file extension.
     */
    const path_t& extension(std::string_view new_extension);

    /**
     * Lexical equality comparison.
     */
    bool operator==(const path_t& other) const;

    /**
     * Joins relative_path and throws unless the result is a strict child.
     */
    path_t operator/(const relative_path_t& relative_path) const;

    /**
     * Appends postfix to the filename and returns a sibling path.
     */
    path_t operator+(std::string_view postfix) const;

    /**
     * Returns the wrapped std::filesystem path.
     */
    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_path;
};

/**
 * Existing path selected under a concrete root.
 */
class rooted_path_t {
public:
    rooted_path_t(const path_t& root, const relative_path_t& relative_path);

    /**
     * Root used to derive path().
     */
    const path_t& root() const;

    /**
     * Path relative to root().
     */
    const relative_path_t& relative_path() const;

    /**
     * Absolute path: root() / relative_path().
     */
    path_t path() const;

private:
    path_t m_root;
    relative_path_t m_relative_path;
};

/**
 * Display path relative to current_path() when possible.
 */
class pretty_path_t {
public:
    explicit pretty_path_t(const path_t& path);

    const std::string& string() const;
    const char* c_str() const;

private:
    std::string m_string;
};

/**
 * Predicate that controls which paths find() returns.
 */
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

/**
 * Predicate that controls which directories find() enters.
 */
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
 * Finds entries under root that match include_predicate.
 *
 * Directory traversal descends only when descend_predicate(directory, depth) returns true.
 *
 * Example:
 * auto headers = find(
 *     root,
 *     find_include_predicate_t::h_file || find_include_predicate_t::hpp_file,
 *     find_descend_predicate_t::descend_all
 * );
 */
std::vector<rooted_path_t> find(const path_t& root, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate);

/**
 * Returns the canonical path, resolving all symbolic links.
 */
path_t canonical(const path_t& path);

/**
 * Copies src to dst and creates dst parent directories.
 */
void copy(const path_t& src, const path_t& dst);

/**
 * Updates path's timestamp or creates an empty file.
 *
 * The parent directory must already exist.
 */
void touch(const path_t& path);

/**
 * Creates all missing parent directories.
 */
void create_directories(const path_t& path);

/**
 * Creates a symbolic link at dst pointing to src.
 */
void create_symlink(const path_t& src, const path_t& dst);

/**
 * Creates a directory symbolic link at dst pointing to src.
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
 *
 * Returns true if a filesystem object was removed.
 */
bool remove(const path_t& path);

/**
 * Recursively removes a directory tree.
 *
 * Returns the number of removed filesystem objects.
 */
std::uintmax_t remove_all(const path_t& path);

/**
 * Renames `from` to `to` without overwriting an existing destination.
 */
void rename_strict(const path_t& from, const path_t& to);

/**
 * Renames `from` to `to`, replacing the destination if it exists.
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

} // namespace m03gagbhsnusi43zogoacgj2ez_filesystem

template <>
struct std::formatter<m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid relative_path_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", relative_path.string());

        return out;
    }
};

template <>
struct std::formatter<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid path_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", path.string());

        return out;
    }
};

template <>
struct std::formatter<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid rooted_path_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", rooted_path.path());

        return out;
    }
};

template <>
struct std::formatter<m03gagbhsnusi43zogoacgj2ez_filesystem::pretty_path_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid pretty_path_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsnusi43zogoacgj2ez_filesystem::pretty_path_t& pretty_path, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", pretty_path.string());

        return out;
    }
};

template <>
struct std::hash<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> {
    std::size_t operator()(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const noexcept {
        return std::hash<std::string>()(path.string());
    }
};

template <>
struct std::hash<m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t> {
    std::size_t operator()(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path) const noexcept {
        return std::hash<std::string>()(relative_path.string());
    }
};

#endif // M03GAGBHSNUSI43ZOGOACGJ2EZ_FILESYSTEM_FILESYSTEM_H
