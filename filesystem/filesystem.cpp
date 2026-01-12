#include <modules/builder/filesystem/filesystem.h>
#include <modules/builder/module/module_graph.h>

#include <format>
#include <algorithm>
#include <iostream>

static std::filesystem::path append_postfix(const std::filesystem::path& path, std::string_view postfix) {
    if (postfix.find_first_of("/\\") != std::string_view::npos) {
        throw std::runtime_error(std::format("append_postfix: postfix '{}' contains path separator", postfix));
    }

    std::filesystem::path new_path = path;
    new_path.replace_filename(new_path.filename().string() + std::string(postfix));
    return new_path;
}

relative_path_t::relative_path_t(const std::filesystem::path& relative_path):
    m_relative_path(relative_path)
{
    if (m_relative_path.is_absolute()) {
        throw std::runtime_error(std::format("relative_path_t: path '{}' is absolute", m_relative_path.string()));
    }
}

const char* relative_path_t::c_str() const {
    return m_relative_path.c_str();
}

std::string relative_path_t::string() const {
    return m_relative_path.string();
}

std::string relative_path_t::stem() const {
    return m_relative_path.stem().string();
}

std::string relative_path_t::extension() const {
    return m_relative_path.extension().string();
}

void relative_path_t::extension(std::string_view new_extension) {
    m_relative_path.replace_extension(new_extension);
}

relative_path_t relative_path_t::operator+(std::string_view postfix) const {
    relative_path_t result(append_postfix(m_relative_path, postfix));

    return result;
}

const std::filesystem::path& relative_path_t::to_native_path() const {
    return m_relative_path;
}

path_t::path_t(const std::filesystem::path& path):
    m_path(std::filesystem::absolute(path).lexically_normal())
{
}

path_t path_t::parent() const {
    const auto parent_path = m_path.parent_path();
    if (parent_path.empty()) {
        throw std::runtime_error(std::format("parent: path '{}' has no parent", m_path.string()));
    }

    return path_t(parent_path);
}

bool path_t::is_child(const path_t& other) const {
    const auto rel = other.m_path.lexically_relative(m_path);
    return !rel.empty() && rel != "." && !rel.native().starts_with("..");
}

relative_path_t path_t::relative(const path_t& other) const {
    if (!is_child(other)) {
        throw std::runtime_error(std::format("relative: path '{}' is not a child of base path '{}'", other.m_path.string(), m_path.string()));
    }

    return relative_path_t(other.m_path.lexically_relative(m_path));
}

bool path_t::is_sibling(const path_t& sibling) const {
    return m_path.parent_path() == sibling.m_path.parent_path();
}

std::string path_t::filename() const {
    return m_path.filename().string();
}

const char* path_t::c_str() const {
    return m_path.c_str();
}

std::string path_t::string() const {
    return m_path.string();
}

std::string path_t::stem() const {
    return m_path.stem().string();
}

std::string path_t::extension() const {
    return m_path.extension().string();
}

void path_t::extension(std::string_view new_extension) {
    m_path.replace_extension(new_extension);
}

bool path_t::operator==(const path_t& other) const {
    return m_path == other.m_path;
}

path_t path_t::operator/(const relative_path_t& relative_path) const {
    path_t result(m_path / relative_path.to_native_path());

    const auto rel = result.m_path.lexically_relative(m_path);
    if (rel.empty() || rel == "." || rel.native().starts_with("..")) {
        throw std::runtime_error(std::format("operator/: path '{}' must not escape the base path '{}'", result.m_path.string(), m_path.string()));
    }

    return result;
}

path_t path_t::operator+(std::string_view postfix) const {
    path_t result(append_postfix(m_path, postfix));

    if (result.is_sibling(*this) || result == *this) {
        throw std::runtime_error(std::format("operator+: path '{}' must be a strict sibling of base path '{}'", result.m_path.string(), m_path.string()));
    }

    return result;
}

const std::filesystem::path& path_t::to_native_path() const {
    return m_path;
}

find_include_predicate_t::find_include_predicate_t(std::function<bool(const path_t& path)>&& predicate):
    predicate(std::move(predicate))
{
}

bool find_include_predicate_t::operator()(const path_t& path) const {
    return predicate(path);
}

find_include_predicate_t find_include_predicate_t::operator&&(find_include_predicate_t b) const {
    return find_include_predicate_t {
        [=, this](const path_t& path) {
            return operator()(path) && b(path);
        }
    };
}

find_include_predicate_t find_include_predicate_t::operator||(find_include_predicate_t b) const {
    return find_include_predicate_t {
        [=, this](const path_t& path) {
            return operator()(path) || b(path);
        }
    };
}

find_include_predicate_t find_include_predicate_t::operator!() const {
    return find_include_predicate_t {
        [=, this](const path_t& path) {
            return !operator()(path);
        }
    };
}

find_descend_predicate_t::find_descend_predicate_t(std::function<bool(const path_t& dir, size_t depth)>&& predicate):
    predicate(std::move(predicate))
{
}

bool find_descend_predicate_t::operator()(const path_t& dir, size_t depth) const {
    return predicate(dir, depth);
}

find_descend_predicate_t find_descend_predicate_t::operator&&(find_descend_predicate_t b) const {
    return find_descend_predicate_t {
        [=, this](const path_t& dir, size_t depth) {
            return operator()(dir, depth) && b(dir, depth);
        }
    };
}

find_descend_predicate_t find_descend_predicate_t::operator||(find_descend_predicate_t b) const {
    return find_descend_predicate_t {
        [=, this](const path_t& dir, size_t depth) {
            return operator()(dir, depth) || b(dir, depth);
        }
    };
}

find_descend_predicate_t find_descend_predicate_t::operator!() const {
    return find_descend_predicate_t {
        [=, this](const path_t& dir, size_t depth) {
            return !operator()(dir, depth);
        }
    };
}

find_include_predicate_t filesystem_t::include_all = {
    [](const path_t& path) {
        return true;
    }
};

find_include_predicate_t filesystem_t::is_dir = {
    [](const path_t& path) {
        return filesystem_t::is_directory(path);
    }
};

find_include_predicate_t filesystem_t::is_regular = {
    [](const path_t& path) {
        return filesystem_t::is_regular_file(path);
    }
};

find_include_predicate_t filesystem_t::cpp_file = {
    [](const path_t& path) {
        return filesystem_t::is_regular_file(path) && path.extension() == ".cpp";
    }
};

find_include_predicate_t filesystem_t::c_file = {
    [](const path_t& path) {
        return filesystem_t::is_regular_file(path) && path.extension() == ".c";
    }
};

find_descend_predicate_t filesystem_t::descend_all = {
    [](const path_t& dir, size_t depth) {
        return true;
    }
};

find_descend_predicate_t filesystem_t::descend_none = {
    [](const path_t& dir, size_t depth) {
        return false;
    }
};

find_include_predicate_t filesystem_t::filename(const std::string& name) {
    return {
        [=](const path_t& path) {
            return path.filename() == name;
        }
    };
}

find_include_predicate_t filesystem_t::path(const path_t& target) {
    return {
        [=](const path_t& path) {
            return path == target;
        }
    };
}

std::vector<path_t> filesystem_t::find(const builder_t* builder, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate) {
    return find(
        builder->src_dir(),
        include_predicate && !path(builder->builder_src_path()) && !path(builder->src_dir() / relative_path_t(module_t::DEPS_JSON)),
        descend_predicate,
        0
    );
}

std::vector<path_t> filesystem_t::find(const path_t& dir, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate) {
    return find(dir, include_predicate, descend_predicate, 0);
}

void filesystem_t::copy(const path_t& src, const path_t& dst) {
    std::error_code ec;
    // std::cout << std::format("cp -r {} {}", src.to_native_path().string(), dst.to_native_path().string()) << std::endl;
    std::filesystem::copy(src.to_native_path(), dst.to_native_path(), std::filesystem::copy_options::recursive, ec);
    if (ec) {
        throw std::runtime_error(std::format("copy: failed to copy from '{}' to '{}': {}", src.to_native_path().string(), dst.to_native_path().string(), ec.message()));
    }
}

void filesystem_t::create_directories(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("mkdir -p {}", path.to_native_path().string()) << std::endl;
    std::filesystem::create_directories(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("create_directories: failed to create directories for path '{}': {}", path.to_native_path().string(), ec.message()));
    }
}

void filesystem_t::create_symlink(const path_t& src, const path_t& dst) {
    std::error_code ec;
    // std::cout << std::format("ln -s {} {}", src.to_native_path().string(), dst.to_native_path().string()) << std::endl;
    std::filesystem::create_symlink(src.to_native_path(), dst.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("create_symlink: failed to create symlink from '{}' to '{}': {}", dst.to_native_path().string(), src.to_native_path().string(), ec.message()));
    }
}

void filesystem_t::create_directory_symlink(const path_t& src, const path_t& dst) {
    std::error_code ec;
    // std::cout << std::format("ln -s {} {}", src.to_native_path().string(), dst.to_native_path().string()) << std::endl;
    std::filesystem::create_directory_symlink(src.to_native_path(), dst.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("create_directory_symlink: failed to create directory symlink from '{}' to '{}': {}", dst.to_native_path().string(), src.to_native_path().string(), ec.message()));
    }
}

bool filesystem_t::exists(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("test -e {}", path.to_native_path().string()) << std::endl;
    const bool result = std::filesystem::exists(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("exists: failed to check existence of path '{}': {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

std::uintmax_t filesystem_t::file_size(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("stat -c%s {}", path.to_native_path().string()) << std::endl;
    const std::uintmax_t result = std::filesystem::file_size(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("file_size: failed to get file size of path '{}': {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

std::filesystem::file_time_type filesystem_t::last_write_time(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("stat -c%Y {}", path.to_native_path().string()) << std::endl;
    std::filesystem::file_time_type result = std::filesystem::last_write_time(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("last_write_time: failed to get last write time of path '{}': {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

bool filesystem_t::remove(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("rm {}", path.to_native_path().string()) << std::endl;
    const bool result = std::filesystem::remove(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("remove: failed to remove path '{}': {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

std::uintmax_t filesystem_t::remove_all(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("rm -rf {}", path.to_native_path().string()) << std::endl;
    const std::uintmax_t result = std::filesystem::remove_all(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("remove_all: failed to remove all at path '{}': {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

bool filesystem_t::is_regular_file(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("test -f {}", path.to_native_path().string()) << std::endl;
    const bool result = std::filesystem::is_regular_file(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("is_regular_file: failed to check if path '{}' is a regular file: {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

bool filesystem_t::is_directory(const path_t& path) {
    std::error_code ec;
    // std::cout << std::format("test -d {}", path.to_native_path().string()) << std::endl;
    const bool result = std::filesystem::is_directory(path.to_native_path(), ec);
    if (ec) {
        throw std::runtime_error(std::format("is_directory: failed to check if path '{}' is a directory: {}", path.to_native_path().string(), ec.message()));
    }
    return result;
}

std::vector<path_t> filesystem_t::find(const path_t& dir, const find_include_predicate_t& include_predicate, const find_descend_predicate_t& descend_predicate, size_t depth) {
    std::vector<path_t> result;

    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(dir.to_native_path(), ec)) {
        const auto path = path_t(dir.to_native_path() / entry.path().filename());

        if (include_predicate(path)) {
            result.push_back(path);
        }

        if (entry.is_directory(ec)) {
            if (ec) {
                break ;
            }

            if (descend_predicate(path, depth)) {
                auto subresult = find(path, include_predicate, descend_predicate, depth + 1);
                result.insert(result.end(), std::make_move_iterator(subresult.begin()), std::make_move_iterator(subresult.end()));
            }
        }
    }
    if (ec) {
        throw std::runtime_error(std::format("find: failed to iterate directory '{}': {}", dir.to_native_path().string(), ec.message()));
    }

    return result;
}
