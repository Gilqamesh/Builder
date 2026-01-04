#ifndef BUILDER_PROJECT_FIND_FIND_H
# define BUILDER_PROJECT_FIND_FIND_H

# include <builder/builder_ctx.h>
# include <builder/builder_api.h>

# include <filesystem>
# include <functional>

struct find_predicate_t {
    std::function<bool(const std::filesystem::directory_entry& e)> predicate;

    bool operator()(const std::filesystem::directory_entry& e) const;
};

find_predicate_t operator&&(find_predicate_t a, find_predicate_t b);
find_predicate_t operator||(find_predicate_t a, find_predicate_t b);
find_predicate_t operator!(find_predicate_t p);

class find_t {
public:
    static find_predicate_t all;
    static find_predicate_t cpp_only;
    static find_predicate_t c_only;

    /**
     * Matches files by basename only.
     *
     * Independent of directory depth and traversal root; when used with
     * recursive traversal, all files with the given name are matched.
     */
    static find_predicate_t filename(const std::string& name);

public:
    /**
     * Traverses the module source directory.
     *
     * - Directory entries are visited in lexicographical order.
     * - `builder_plugin.cpp` is always excluded.
     * - The predicate filters entries and may prune recursion.
     * - Only regular file paths are returned.
     */
    static std::vector<std::filesystem::path> find(builder_ctx_t* ctx, const builder_api_t* api, const find_predicate_t& find_predicate, bool recursive);

public:
    /**
     * Traverses `root` with identical semantics to the framework version of `find`.
     */
    static std::vector<std::filesystem::path> find(const std::filesystem::path& root, const find_predicate_t& find_predicate, bool recursive);
};

#endif // BUILDER_PROJECT_FIND_FIND_H
