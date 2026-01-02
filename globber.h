#ifndef GLOBBER_H
# define GLOBBER_H

# include "builder_ctx.h"
# include "builder_api.h"

# include <filesystem>
# include <unordered_set>

class globber_t {
public:
    // returns relative source files to the module
    static std::vector<std::string> cpp_files_recursive(builder_ctx_t* ctx, const builder_api_t* api);

public:
    static std::vector<std::string> cpp_files_recursive(const std::filesystem::path& dir, const std::unordered_set<std::string>& relative_exclude_files);
};

#endif // GLOBBER_H
