#include <modules/builder/find/find.h>

#include <format>

bool find_predicate_t::operator()(const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& e) const {
    return predicate(absolute_path, e);
}

find_predicate_t operator&&(find_predicate_t a, find_predicate_t b) {
    return find_predicate_t {
        [=](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& e) {
            return a(absolute_path, e) && b(absolute_path, e);
        }
    };
}

find_predicate_t operator||(find_predicate_t a, find_predicate_t b) {
    return find_predicate_t {
        [=](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& e) {
            return a(absolute_path, e) || b(absolute_path, e);
        }
    };
}

find_predicate_t operator!(find_predicate_t p) {
    return find_predicate_t {
        [=](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& e) {
            return !p(absolute_path, e);
        }
    };
}

find_predicate_t find_t::all = {
    [](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& entry) {
        return true;
    }
};

find_predicate_t find_t::cpp_only = {
    [](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& entry) {
        return entry.is_regular_file() && entry.path().extension() == ".cpp";
    }
};

find_predicate_t find_t::c_only = {
    [](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& entry) {
        return entry.is_regular_file() && entry.path().extension() == ".c";
    }
};

find_predicate_t find_t::filename(const std::string& name) {
    return {
        [=](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& entry) {
            return entry.is_regular_file() && entry.path().filename() == name;
        }
    };
}

find_predicate_t find_t::abs(const std::filesystem::path& absolute_file_path) {
    if (!absolute_file_path.is_absolute()) {
        throw std::runtime_error(std::format("find::abs: file path '{}' is not absolute", absolute_file_path.string()));
    }

    const auto target = absolute_file_path.lexically_normal();
    return {
        [=](const std::filesystem::path& absolute_path, const std::filesystem::directory_entry& entry) {
            return absolute_path == target;
        }
    };
}

std::vector<std::filesystem::path> find_t::find(const builder_t* builder, const find_predicate_t& find_predicate, bool recursive) {
    return find(
        builder->src_dir(),
        find_predicate && !abs(builder->builder_src_path()) && !abs(builder->src_dir() / module_t::DEPS_JSON),
        recursive
    );
}

std::vector<std::filesystem::path> find_t::find(const std::filesystem::path& root, const find_predicate_t& find_predicate, bool recursive) {
    const auto abs_root = std::filesystem::absolute(root).lexically_normal();
    return find_impl(abs_root, find_predicate, recursive);
}

std::vector<std::filesystem::path> find_t::find_impl(const std::filesystem::path& absolute_dir, const find_predicate_t& find_predicate, bool recursive) {
    std::vector<std::filesystem::path> result;

    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;

    for (auto& entry : std::filesystem::directory_iterator(absolute_dir, ec)) {
        entries.push_back(entry);
    }

    if (ec) {
        throw std::runtime_error(std::format("find_impl: failed to iterate directory '{}': {}", absolute_dir.string(), ec.message()));
    }

    std::sort(entries.begin(), entries.end(), [&](const auto& a, const auto& b) {
        return (absolute_dir / a.path().filename()).lexically_normal() < (absolute_dir / b.path().filename()).lexically_normal();
    });

    for (const auto& entry : entries) {
        const auto absolute_path = (absolute_dir / entry.path().filename()).lexically_normal();

        if (!find_predicate(absolute_path, entry)) {
            continue ;
        }

        if (entry.is_directory()) {
            // NOTE: could be pulled out from the for loop, but this is clearer
            if (recursive) {
                auto subresult = find_impl(absolute_path, find_predicate, recursive);
                result.insert(result.end(), std::make_move_iterator(subresult.begin()), std::make_move_iterator(subresult.end()));
            }
        } else {
            result.push_back(absolute_path);
        }
    }

    return result;
}
