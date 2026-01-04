#ifndef BUILDER_PROJECT_FIND_FIND_H
# define BUILDER_PROJECT_FIND_FIND_H

# include <builder/builder_ctx.h>
# include <builder/builder_api.h>

# include <filesystem>
# include <unordered_set>
# include <regex>

class find_t {
public:
    /**
     * Traverses the module source directory.
     *
     * - Entries are processed in lexicographical order per directory.
     * - `builder_plugin.cpp` is implicitly excluded.
     * - `predicate(entry) == false` prunes directories when `recursive == true`.
     * - Only non-directory entries are returned.
     */
    static std::vector<std::filesystem::path> find(builder_ctx_t* ctx, const builder_api_t* api, const std::function<bool(const std::filesystem::directory_entry& entry)>& predicate, bool recursive);

public:
    /**
     * Traverses `root` with identical semantics to the framework version of `find`.
     */
    static std::vector<std::filesystem::path> find(const std::filesystem::path& root, const std::function<bool(const std::filesystem::directory_entry& entry)>& predicate, bool recursive);
};

#endif // BUILDER_PROJECT_FIND_FIND_H
