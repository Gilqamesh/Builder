#include "artifact_store.h"

#include <chrono>
#include <ctime>
#include <format>
#include <stdexcept>
#include <string>

namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store {

namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;

static std::string utc_version_prefix() {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(seconds);

    std::tm tm {};
    gmtime_r(&time, &tm);

    return std::format(
        "{:04}{:02}{:02}T{:02}{:02}{:02}.{:09}Z",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        nanoseconds
    );
}

static bool is_symlink(const filesystem::path_t& path) {
    std::error_code ec;
    return std::filesystem::is_symlink(path.to_native_path(), ec);
}

bool exists_or_symlink(const filesystem::path_t& path) {
    return filesystem::exists(path) || is_symlink(path);
}

void remove_existing_path(const filesystem::path_t& path) {
    if (!exists_or_symlink(path)) {
        return ;
    }

    if (is_symlink(path) || !filesystem::is_directory(path)) {
        filesystem::remove(path);
        return ;
    }

    filesystem::remove_all(path);
}

filesystem::path_t completed_marker(const filesystem::path_t& artifact_dir) {
    return artifact_dir / filesystem::relative_path_t(".complete");
}

filesystem::path_t started_marker(const filesystem::path_t& artifact_dir) {
    return artifact_dir / filesystem::relative_path_t(".started");
}

static bool artifact_dir_name_matches_hash(std::string_view name, std::string_view hash) {
    const auto suffix = std::format("-{}", hash);
    return name.size() > suffix.size() && name.ends_with(suffix);
}

std::optional<filesystem::path_t> completed_artifact_dir_by_hash(
    const filesystem::path_t& root,
    std::string_view hash
) {
    if (!filesystem::exists(root)) {
        return std::nullopt;
    }

    std::optional<filesystem::path_t> result;
    std::string result_name;

    for (const auto& entry : filesystem::find(
        root,
        filesystem::find_include_predicate_t::is_dir,
        filesystem::find_descend_predicate_t::descend_none
    )) {
        const auto name = entry.path().filename();
        if (!artifact_dir_name_matches_hash(name, hash)) {
            continue;
        }
        if (!filesystem::exists(completed_marker(entry.path()))) {
            continue;
        }
        if (!result || result_name < name) {
            result = entry.path();
            result_name = name;
        }
    }

    return result;
}

filesystem::path_t versioned_artifact_dir(
    const filesystem::path_t& root,
    std::string_view hash
) {
    if (const auto existing = completed_artifact_dir_by_hash(root, hash)) {
        return *existing;
    }

    return root / filesystem::relative_path_t(std::format("{}-{}", utc_version_prefix(), hash));
}

filesystem::path_t artifact_dir(
    const workspace_graph::module_t& module,
    const filesystem::relative_path_t& kind,
    std::string_view hash
) {
    return versioned_artifact_dir(module.artifact_base_dir() / kind, hash);
}

filesystem::path_t latest_dir(
    const workspace_graph::module_t& module,
    const filesystem::relative_path_t& kind
) {
    return module.artifact_latest_dir() / kind;
}

static void create_latest_parent(const filesystem::path_t& path) {
    const auto parent = path.parent();
    if (is_symlink(parent)) {
        filesystem::remove(parent);
    }
    if (filesystem::exists(parent)) {
        if (!filesystem::is_directory(parent)) {
            throw std::runtime_error(std::format("m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store::create_latest_parent: latest parent '{}' is not a directory", parent));
        }
        return ;
    }

    filesystem::create_directories(parent);
}

static bool symlink_points_to(
    const filesystem::path_t& symlink_path,
    const filesystem::path_t& target_path
) {
    if (!is_symlink(symlink_path)) {
        return false;
    }

    std::error_code ec;
    auto symlink_target = std::filesystem::read_symlink(symlink_path.to_native_path(), ec);
    if (ec) {
        return false;
    }
    if (symlink_target.is_relative()) {
        symlink_target = symlink_path.parent().to_native_path() / symlink_target;
    }

    return filesystem::path_t(symlink_target) == target_path;
}

void update_latest_symlink(
    const filesystem::path_t& latest_path,
    const filesystem::path_t& artifact_dir
) {
    if (symlink_points_to(latest_path, artifact_dir)) {
        return ;
    }

    const auto latest_tmp_path = latest_path + "_tmp";

    remove_existing_path(latest_tmp_path);
    create_latest_parent(latest_path);

    filesystem::create_directory_symlink(artifact_dir, latest_tmp_path);
    filesystem::rename_replace(latest_tmp_path, latest_path);
}

void update_latest(
    const workspace_graph::module_t& module,
    const filesystem::relative_path_t& kind,
    const filesystem::path_t& artifact_dir
) {
    update_latest_symlink(latest_dir(module, kind), artifact_dir);
}

} // namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store
