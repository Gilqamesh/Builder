#include <modules/builder/find/find.h>

#include <format>

bool find_predicate_t::operator()(const std::filesystem::directory_entry& e) const {
    return predicate(e);
}

find_predicate_t operator&&(find_predicate_t a, find_predicate_t b) {
    return find_predicate_t {
        [=](const std::filesystem::directory_entry& e) {
            return a(e) && b(e);
        }
    };
}

find_predicate_t operator||(find_predicate_t a, find_predicate_t b) {
    return find_predicate_t {
        [=](const std::filesystem::directory_entry& e) {
            return a(e) || b(e);
        }
    };
}

find_predicate_t operator!(find_predicate_t p) {
    return find_predicate_t {
        [=](const std::filesystem::directory_entry& e) {
            return !p(e);
        }
    };
}

find_predicate_t find_t::all = {
    [](const std::filesystem::directory_entry& entry) {
        return true;
    }
};

find_predicate_t find_t::cpp_only = {
    [](const std::filesystem::directory_entry& entry) {
        return entry.is_regular_file() && entry.path().extension() == ".cpp";
    }
};

find_predicate_t find_t::c_only = {
    [](const std::filesystem::directory_entry& entry) {
        return entry.is_regular_file() && entry.path().extension() == ".c";
    }
};

find_predicate_t find_t::filename(const std::string& name) {
    return {
        [=](const std::filesystem::directory_entry& entry) {
            return entry.is_regular_file() && entry.path().filename() == name;
        }
    };
}

find_predicate_t find_t::path(const std::filesystem::path& file_path) {
    return {
        [=](const std::filesystem::directory_entry& entry) {
            return entry.is_regular_file() && entry.path() == file_path;
        }
    };
}

std::vector<std::filesystem::path> find_t::find(const builder_t* builder, const find_predicate_t& find_predicate, bool recursive) {
    return find(
        builder->src_dir(),
        find_predicate && !path(builder->builder_src_path()) && !path(builder->src_dir() / module_t::DEPS_JSON),
        recursive
    );
}

std::vector<std::filesystem::path> find_t::find(const std::filesystem::path& root, const find_predicate_t& find_predicate, bool recursive) {
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
        if (!find_predicate(entry)) {
            continue ;
        }

        if (entry.is_directory()) {
            // NOTE: could be pulled out from the for loop, but this is clearer
            if (recursive) {
                auto subresult = find(entry.path(), find_predicate, recursive);
                result.insert(result.end(), std::make_move_iterator(subresult.begin()), std::make_move_iterator(subresult.end()));
            }
        } else {
            result.push_back(entry.path());
        }
    }

    return result;
}
