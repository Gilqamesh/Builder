#include <builder/find/find.h>

#include <builder/internal.h>

#include <format>

std::vector<std::filesystem::path> find_t::find(builder_ctx_t* ctx, const builder_api_t* api, const std::function<bool(const std::filesystem::directory_entry& entry)>& predicate, bool recursive) {
    const auto builder_plugin_cpp = api->source_dir(ctx) / BUILDER_PLUGIN_CPP;
    return find(
        api->source_dir(ctx),
        [&](const std::filesystem::directory_entry& entry) {
            if (entry.path() == builder_plugin_cpp) {
                return false;
            } else {
                return predicate(entry);
            }
        },
        recursive
    );
}


std::vector<std::filesystem::path> find_t::find(const std::filesystem::path& root, const std::function<bool(const std::filesystem::directory_entry& entry)>& predicate, bool recursive) {
    std::vector<std::filesystem::path> result;

    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;

    for (auto& entry : std::filesystem::directory_iterator(root, ec)) {
        entries.push_back(entry);
    }

    if (ec) {
        throw std::runtime_error(std::format("find: failed to iterate directory '{}': {}", root.string(), ec.message()));
    }

    std::sort(entries.begin(), entries.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
        return a.path() < b.path();
    });

    for (const auto& entry : entries) {
        if (!predicate(entry)) {
            continue ;
        }

        if (entry.is_directory()) {
            // NOTE: could be pulled out from the for loop, but this is clearer
            if (recursive) {
                auto subresult = find(entry.path(), predicate, recursive);
                result.insert(result.end(), std::make_move_iterator(subresult.begin()), std::make_move_iterator(subresult.end()));
            }
        } else {
            result.push_back(entry.path());
        }
    }

    return result;
}
