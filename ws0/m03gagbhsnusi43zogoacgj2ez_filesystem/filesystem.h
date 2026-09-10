#ifndef M03GAGBHSNUSI43ZOGOACGJ2EZ_FILESYSTEM_FILESYSTEM_H
# define M03GAGBHSNUSI43ZOGOACGJ2EZ_FILESYSTEM_FILESYSTEM_H

# include <filesystem>
# include <functional>
# include <format>

/**
 * @brief Checked filesystem paths and operations.
 *
 * Explicit path and operation checks throw std::runtime_error. Underlying
 * standard-library operations and caller-supplied predicates may also throw.
 */
namespace m03gagbhsnusi43zogoacgj2ez_filesystem {

/**
 * @brief Stores a relative filesystem path normalized lexically at construction.
 */
class relative_path_t {
public:
    /**
     * @brief Normalizes a relative path without accessing the filesystem.
     *
     * Collapses lexical `.` and `..` components and redundant separators, and
     * removes trailing separators. Rejects absolute paths with std::runtime_error.
     * Empty paths, `.`, and leading `..` are allowed here; containment is checked
     * when joining to path_t. Symbolic links are not resolved.
     */
    relative_path_t(const std::filesystem::path& relative_path);

    /**
     * @brief Borrows the native string until this path is modified or destroyed.
     */
    const char* c_str() const;

    /**
     * @brief Returns the string representation.
     */
    std::string string() const;

    /**
     * @brief Returns the stem of the filename.
     */
    std::string stem() const;

    /**
     * @brief Returns the file extension, including the leading dot.
     */
    std::string extension() const;

    /**
     * @brief Replaces the stored extension in place and returns this path.
     *
     * Does not rename a filesystem entry; invalidates previously borrowed strings.
     */
    const relative_path_t& extension(std::string_view new_extension);

    /**
     * @brief Lexical equality comparison.
     */
    bool operator==(const relative_path_t& other) const;

    /**
     * @brief Appends postfix to the filename and returns a sibling path.
     */
    relative_path_t operator+(std::string_view postfix) const;

    /**
     * @brief Joins and lexically normalizes two relative paths without a containment check.
     */
    relative_path_t operator/(const relative_path_t& other) const;

    /**
     * @brief Borrows the stored native path for this object's lifetime.
     *
     * Mutating this object changes the referenced path.
     */
    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_relative_path;
};

/**
 * @brief Stores an absolute filesystem path normalized lexically at construction.
 */
class path_t {
public:
    /**
     * @brief Resolves relative input against the current directory and normalizes it lexically.
     *
     * Does not require the result to exist or resolve symbolic links; use
     * canonical() for an existing path with symbolic links resolved. Relative
     * input uses the process working directory at construction time.
     */
    path_t(const std::filesystem::path& path);

    /**
     * @brief Returns the parent directory.
     *
     * Throws if the path has no parent, i.e., path is root.
     */
    path_t parent() const;

    /**
     * @brief Checks whether `other` is a strict lexical descendant of this path.
     */
    bool is_child(const path_t& other) const;

    /**
     * @brief Returns the lexical relative path to a strict descendant.
     *
     * Throws std::runtime_error when other is this path itself or is outside it.
     * Neither path needs to exist.
     */
    relative_path_t relative(const path_t& other) const;

    /**
     * @brief Checks whether `sibling` shares the same parent directory as this path.
     */
    bool is_sibling(const path_t& sibling) const;

    /**
     * @brief Returns the final path component.
     */
    std::string filename() const;

    /**
     * @brief Borrows the native string until this path is modified or destroyed.
     */
    const char* c_str() const;

    /**
     * @brief Returns the string representation.
     */
    std::string string() const;

    /**
     * @brief Returns the stem of the filename.
     */
    std::string stem() const;

    /**
     * @brief Returns the file extension, including the leading dot.
     */
    std::string extension() const;

    /**
     * @brief Replaces the stored extension in place and returns this path.
     *
     * Does not rename a filesystem entry; invalidates previously borrowed strings.
     */
    const path_t& extension(std::string_view new_extension);

    /**
     * @brief Lexical equality comparison.
     */
    bool operator==(const path_t& other) const;

    /**
     * @brief Joins relative_path and throws unless the result is a strict child.
     *
     * Checks the normalized result, so `a/../b` is accepted, while `.`, an
     * empty path, and `a/..` select the base itself and throw std::runtime_error.
     * A result outside the base also throws. This is lexical containment, not
     * a check that symbolic-link targets stay beneath the base.
     */
    path_t operator/(const relative_path_t& relative_path) const;

    /**
     * @brief Appends postfix to the filename and returns a sibling path.
     */
    path_t operator+(std::string_view postfix) const;

    /**
     * @brief Borrows the stored native path for this object's lifetime.
     *
     * Mutating this object changes the referenced path.
     */
    const std::filesystem::path& to_native_path() const;

private:
    std::filesystem::path m_path;
};

/**
 * @brief Retains a root and relative path whose strict descendant exists at construction.
 *
 * Owns copies of both paths, not the filesystem entry. Existence is checked once
 * and may change afterwards; path() only recomputes the lexical join. Files and
 * directories are both accepted. Symbolic links are followed for the existence
 * check, so this is not a filesystem containment boundary.
 *
 * The caller supplies a source_root below the filesystem root, containing `src/widget.cpp`:
 * @code{.cpp}
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * #include <stdexcept>
 *
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * filesystem::rooted_path_t select_source(const filesystem::path_t& source_root) {
 *     const auto source_path = source_root
 *         / filesystem::relative_path_t("src/../src/widget.cpp"); // Accepted.
 *     const char* rejected[] { ".", "src/..", "..", "../outside.cpp" };
 *     for (const char* spelling : rejected) {
 *         try {
 *             (void)(source_root / filesystem::relative_path_t(spelling));
 *         } catch (const std::runtime_error&) {
 *             // Each join is rejected: it selects the root itself or escapes it.
 *         }
 *     }
 *     return filesystem::rooted_path_t(source_root, source_root.relative(source_path));
 * }
 * @endcode
 */
class rooted_path_t {
public:
    /**
     * @brief Copies the paths and checks that root / relative_path exists.
     *
     * Throws std::runtime_error for a non-descendant, a missing entry, or an
     * existence-query failure. Selecting root itself is invalid even if it exists.
     */
    rooted_path_t(const path_t& root, const relative_path_t& relative_path);

    /**
     * @brief Borrows the stored root for this rooted path's lifetime.
     */
    const path_t& root() const;

    /**
     * @brief Borrows the stored relative path for this rooted path's lifetime.
     */
    const relative_path_t& relative_path() const;

    /**
     * @brief Absolute path: root() / relative_path().
     */
    path_t path() const;

private:
    path_t m_root;
    relative_path_t m_relative_path;
};

/**
 * @brief Stores a display string relative to the construction-time working directory for strict children.
 *
 * Other paths, including the working directory itself, remain absolute. Returned
 * string references and pointers borrow this object's storage; later changes to
 * the working directory do not alter it.
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
 * @brief Predicate that controls which paths find() returns.
 *
 * Owns its callable; composed predicates copy their operands and short-circuit.
 * Any references captured by a callable must outlive its use.
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
     * @brief Matches entries by basename.
     */
    static find_include_predicate_t filename(const std::string& name);

    /**
     * @brief Matches a single path.
     */
    static find_include_predicate_t path(const path_t& target);

    std::function<bool(const path_t& path)> predicate;
    bool operator()(const path_t& path) const;

    find_include_predicate_t operator&&(find_include_predicate_t b) const;
    find_include_predicate_t operator||(find_include_predicate_t b) const;
    find_include_predicate_t operator!() const;
};

/**
 * @brief Predicate that controls which directories find() enters.
 *
 * Owns its callable; composed predicates copy their operands and short-circuit.
 * A direct child directory of the search root is tested with depth 0. Any
 * references captured by a callable must outlive its use.
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
 * @brief Finds entries under root that match include_predicate.
 *
 * The root must be an existing directory and is not itself returned. Inclusion
 * does not control descent: descend_predicate(directory, depth) decides whether
 * to enter each non-symlink directory. Directory symlink entries are not traversed.
 * Results are unsorted owning path values; neither callbacks nor their borrowed
 * path arguments are retained. Callback exceptions propagate.
 *
 * @code{.cpp}
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * #include <vector>
 *
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * std::vector<filesystem::rooted_path_t> headers(const filesystem::path_t& root) {
 *     return filesystem::find(
 *         root,
 *         filesystem::find_include_predicate_t::h_file || filesystem::find_include_predicate_t::hpp_file,
 *         filesystem::find_descend_predicate_t::descend_all
 *     );
 * }
 * @endcode
 */
std::vector<rooted_path_t> find(const path_t& root, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate);

/**
 * @brief Returns the canonical path, resolving all symbolic links.
 */
path_t canonical(const path_t& path);

/**
 * @brief Copies src to dst and creates dst parent directories.
 */
void copy(const path_t& src, const path_t& dst);

/**
 * @brief Updates path's timestamp or creates an empty file.
 *
 * The parent directory must already exist.
 */
void touch(const path_t& path);

/**
 * @brief Creates the named directory and all missing parent directories.
 */
void create_directories(const path_t& path);

/**
 * @brief Creates a symbolic link at dst pointing to src.
 */
void create_symlink(const path_t& src, const path_t& dst);

/**
 * @brief Creates a directory symbolic link at dst pointing to src.
 */
void create_directory_symlink(const path_t& src, const path_t& dst);

/**
 * @brief Returns the current working directory.
 */
path_t current_path();

/**
 * @brief Sets the current working directory.
 *
 * Changes process-wide state, including the base used by subsequent relative
 * path_t construction; coordinate with other threads using relative paths.
 */
void current_path(const path_t& path);

/**
 * @brief Checks whether a path exists.
 */
bool exists(const path_t& path);

/**
 * @brief Returns the size of a regular file in bytes.
 */
std::uintmax_t file_size(const path_t& path);

/**
 * @brief Returns the last modification timestamp.
 */
std::filesystem::file_time_type last_write_time(const path_t& path);

/**
 * @brief Removes a single file or empty directory.
 *
 * Returns true if a filesystem object was removed.
 */
bool remove(const path_t& path);

/**
 * @brief Recursively removes a directory tree.
 *
 * Returns the number of removed filesystem objects.
 */
std::uintmax_t remove_all(const path_t& path);

/**
 * @brief Renames `from` to `to` without overwriting an existing destination.
 */
void rename_strict(const path_t& from, const path_t& to);

/**
 * @brief Renames `from` to `to`, replacing the destination if it exists.
 */
void rename_replace(const path_t& from, const path_t& to);

/**
 * @brief Checks whether the path refers to a regular file.
 */
bool is_regular_file(const path_t& path);

/**
 * @brief Checks whether the path refers to a directory.
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
