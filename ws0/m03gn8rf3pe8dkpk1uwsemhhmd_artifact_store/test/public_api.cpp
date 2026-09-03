#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store/artifact_store.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

namespace api = m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace graph_api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-artifact-store-public-api-{}-{}",
            std::chrono::steady_clock::now().time_since_epoch().count(),
            reinterpret_cast<std::uintptr_t>(this)
        );
        std::filesystem::create_directory(m_path);
    }

    ~temporary_directory_t() {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

void touch_native(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to create artifact-store fixture");
    }
}

filesystem_api::path_t resolved_symlink_target(const filesystem_api::path_t& path) {
    auto target = std::filesystem::read_symlink(path.to_native_path());
    if (target.is_relative()) {
        target = path.parent().to_native_path() / target;
    }
    return filesystem_api::path_t(target);
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const filesystem_api::path_t root(temporary_directory.path());

        const auto missing = root / filesystem_api::relative_path_t("missing");
        test::expect(std::identity(), !api::exists_or_symlink(missing));
        test::expect_no_throw([&] { api::remove_existing_path(missing); });

        const auto regular_file = root / filesystem_api::relative_path_t("regular-file");
        touch_native(regular_file.to_native_path());
        test::expect(std::identity(), api::exists_or_symlink(regular_file));
        api::remove_existing_path(regular_file);
        test::expect(std::identity(), !api::exists_or_symlink(regular_file));

        const auto directory = root / filesystem_api::relative_path_t("directory");
        std::filesystem::create_directories(directory.to_native_path() / "nested");
        touch_native(directory.to_native_path() / "nested" / "file");
        test::expect(std::identity(), api::exists_or_symlink(directory));
        api::remove_existing_path(directory);
        test::expect(std::identity(), !api::exists_or_symlink(directory));

        const auto symlink_target = root / filesystem_api::relative_path_t("symlink-target");
        std::filesystem::create_directory(symlink_target.to_native_path());
        const auto symlink = root / filesystem_api::relative_path_t("symlink");
        std::filesystem::create_directory_symlink(
            symlink_target.to_native_path(),
            symlink.to_native_path()
        );
        test::expect(std::identity(), api::exists_or_symlink(symlink));
        api::remove_existing_path(symlink);
        test::expect(std::identity(), !api::exists_or_symlink(symlink));
        test::expect(std::identity(), std::filesystem::exists(symlink_target.to_native_path()));

        const auto dangling = root / filesystem_api::relative_path_t("dangling");
        std::filesystem::create_symlink(
            temporary_directory.path() / "does-not-exist",
            dangling.to_native_path()
        );
        test::expect(std::identity(), api::exists_or_symlink(dangling));
        api::remove_existing_path(dangling);
        test::expect(std::identity(), !api::exists_or_symlink(dangling));

        const auto artifact = root / filesystem_api::relative_path_t("artifact");
        test::expect(std::equal_to<>(), api::started_marker(artifact),
            artifact / filesystem_api::relative_path_t(".started")
        );
        test::expect(std::equal_to<>(), api::completed_marker(artifact),
            artifact / filesystem_api::relative_path_t(".complete")
        );

        const auto lookup_root = root / filesystem_api::relative_path_t("lookup");
        test::expect(std::identity(), !api::completed_artifact_dir_by_hash(lookup_root, "abc"));

        const auto older = lookup_root / filesystem_api::relative_path_t("20240101T000000.000000000Z-abc");
        const auto newer = lookup_root / filesystem_api::relative_path_t("20240201T000000.000000000Z-abc");
        const auto newest_incomplete = lookup_root / filesystem_api::relative_path_t("20240301T000000.000000000Z-abc");
        const auto wrong_hash = lookup_root / filesystem_api::relative_path_t("20240401T000000.000000000Z-other");
        const auto too_short_name = lookup_root / filesystem_api::relative_path_t("-abc");
        std::filesystem::create_directories(older.to_native_path());
        std::filesystem::create_directories(newer.to_native_path());
        std::filesystem::create_directories(newest_incomplete.to_native_path());
        std::filesystem::create_directories(wrong_hash.to_native_path());
        std::filesystem::create_directories(too_short_name.to_native_path());
        touch_native(api::completed_marker(older).to_native_path());
        touch_native(api::completed_marker(newer).to_native_path());
        touch_native(api::completed_marker(wrong_hash).to_native_path());
        touch_native(lookup_root.to_native_path() / "20240501T000000.000000000Z-abc");

        const auto found = api::completed_artifact_dir_by_hash(lookup_root, "abc");
        test::expect(std::identity(), found.has_value());
        test::expect(std::equal_to<>(), *found, newer);
        test::expect(std::identity(), !api::completed_artifact_dir_by_hash(lookup_root, "missing"));
        test::expect(std::equal_to<>(), api::versioned_artifact_dir(lookup_root, "abc"), newer);

        const auto generated = api::versioned_artifact_dir(lookup_root, "newhash");
        test::expect(std::equal_to<>(), generated.parent(), lookup_root);
        test::expect(std::identity(), std::regex_match(
            generated.filename(),
            std::regex(R"([0-9]{8}T[0-9]{6}\.[0-9]{9}Z-newhash)")
        ));
        test::expect(std::identity(), !api::exists_or_symlink(generated));

        const auto workspace_root_native = temporary_directory.path() / "workspace";
        std::filesystem::create_directory(workspace_root_native);
        const filesystem_api::path_t workspace_root(workspace_root_native);
        const filesystem_api::path_t artifact_root(temporary_directory.path() / "module-artifacts");
        graph_api::workspace_graph_t graph(workspace_root, artifact_root);
        graph_api::workspace_t workspace(graph, graph_api::workspace_name_t("ws0"));
        const graph_api::module_name_t module_name(
            "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store"
        );
        graph_api::module_t module(&workspace, module_name, graph_api::version_t(1));
        const filesystem_api::relative_path_t kind("library");

        const auto expected_kind_root = module.artifact_base_dir() / kind;
        const auto module_artifact = api::artifact_dir(module, kind, "modulehash");
        test::expect(std::equal_to<>(), module_artifact.parent(), expected_kind_root);
        test::expect(std::identity(), module_artifact.filename().ends_with("-modulehash"));
        test::expect(std::equal_to<>(), api::latest_dir(module, kind),
            module.artifact_latest_dir() / kind
        );

        const auto completed_module_artifact = expected_kind_root
            / filesystem_api::relative_path_t("20250101T000000.000000000Z-modulehash");
        std::filesystem::create_directories(completed_module_artifact.to_native_path());
        touch_native(api::completed_marker(completed_module_artifact).to_native_path());
        test::expect(std::equal_to<>(), api::artifact_dir(module, kind, "modulehash"),
            completed_module_artifact
        );

        const auto target_a = root / filesystem_api::relative_path_t("targets/a");
        const auto target_b = root / filesystem_api::relative_path_t("targets/b");
        std::filesystem::create_directories(target_a.to_native_path());
        std::filesystem::create_directories(target_b.to_native_path());

        const auto latest_path = root / filesystem_api::relative_path_t("latest/tree/value");
        api::update_latest_symlink(latest_path, target_a);
        test::expect(std::identity(), std::filesystem::is_symlink(latest_path.to_native_path()));
        test::expect(std::equal_to<>(), resolved_symlink_target(latest_path), target_a);
        test::expect(std::identity(), !api::exists_or_symlink(latest_path + "_tmp"));

        api::update_latest_symlink(latest_path, target_a);
        test::expect(std::equal_to<>(), resolved_symlink_target(latest_path), target_a);
        api::update_latest_symlink(latest_path, target_b);
        test::expect(std::equal_to<>(), resolved_symlink_target(latest_path), target_b);

        const auto preexisting_tmp_latest = root / filesystem_api::relative_path_t("tmp-case/latest");
        const auto preexisting_tmp = preexisting_tmp_latest + "_tmp";
        std::filesystem::create_directories(preexisting_tmp.to_native_path() / "nested");
        touch_native(preexisting_tmp.to_native_path() / "nested" / "file");
        api::update_latest_symlink(preexisting_tmp_latest, target_a);
        test::expect(std::equal_to<>(), resolved_symlink_target(preexisting_tmp_latest), target_a);
        test::expect(std::identity(), !api::exists_or_symlink(preexisting_tmp));

        const auto parent_symlink_target = root / filesystem_api::relative_path_t("old-parent-target");
        std::filesystem::create_directory(parent_symlink_target.to_native_path());
        const auto parent_symlink = root / filesystem_api::relative_path_t("parent-symlink");
        std::filesystem::create_directory_symlink(
            parent_symlink_target.to_native_path(),
            parent_symlink.to_native_path()
        );
        const auto through_parent_symlink = parent_symlink / filesystem_api::relative_path_t("latest");
        api::update_latest_symlink(through_parent_symlink, target_b);
        test::expect(std::identity(), std::filesystem::is_directory(parent_symlink.to_native_path()));
        test::expect(std::identity(), !std::filesystem::is_symlink(parent_symlink.to_native_path()));
        test::expect(std::equal_to<>(), resolved_symlink_target(through_parent_symlink), target_b);

        const auto parent_file = root / filesystem_api::relative_path_t("parent-file");
        touch_native(parent_file.to_native_path());
        test::expect_throws<std::runtime_error>([&] {
            api::update_latest_symlink(
                parent_file / filesystem_api::relative_path_t("latest"),
                target_a
            );
        });

        const auto module_target = root / filesystem_api::relative_path_t("module-target");
        std::filesystem::create_directory(module_target.to_native_path());
        api::update_latest(module, kind, module_target);
        const auto module_latest = api::latest_dir(module, kind);
        test::expect(std::identity(), std::filesystem::is_symlink(module_latest.to_native_path()));
        test::expect(std::equal_to<>(), resolved_symlink_target(module_latest), module_target);
        api::update_latest(module, kind, target_a);
        test::expect(std::equal_to<>(), resolved_symlink_target(module_latest), target_a);
    });
}
