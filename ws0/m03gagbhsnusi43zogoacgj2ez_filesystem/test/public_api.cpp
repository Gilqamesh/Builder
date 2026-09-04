#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        const auto suffix = std::format(
            "{}-{}",
            std::chrono::steady_clock::now().time_since_epoch().count(),
            reinterpret_cast<std::uintptr_t>(this)
        );
        m_path = std::filesystem::temp_directory_path() / ("builder-filesystem-public-api-" + suffix);

        std::error_code error;
        const bool created = std::filesystem::create_directory(m_path, error);
        if (error || !created) {
            throw std::runtime_error("failed to create temporary test directory");
        }
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

class current_path_guard_t {
public:
    current_path_guard_t():
        m_path(std::filesystem::current_path())
    {
    }

    ~current_path_guard_t() {
        std::error_code error;
        std::filesystem::current_path(m_path, error);
    }

private:
    std::filesystem::path m_path;
};

void write_file(const api::path_t& path, std::string_view contents) {
    std::ofstream output(path.to_native_path(), std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to open test file");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::vector<std::string> relative_strings(const std::vector<api::rooted_path_t>& paths) {
    std::vector<std::string> result;
    result.reserve(paths.size());
    for (const auto& path : paths) {
        result.push_back(path.relative_path().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool contains(const std::vector<std::string>& values, std::string_view value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}


} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        current_path_guard_t current_path_guard;

        const api::path_t root(temporary_directory.path());

        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const api::relative_path_t invalid(std::filesystem::path("/absolute"));
        });

        api::relative_path_t relative("alpha/./beta/../file.txt");
        test::expect(std::equal_to<>(), relative.string(), std::string("alpha/file.txt"));
        test::expect(std::equal_to<>(), std::string(relative.c_str()), relative.string());
        test::expect(std::equal_to<>(), relative.stem(), std::string("file"));
        test::expect(std::equal_to<>(), relative.extension(), std::string(".txt"));
        test::expect(std::identity(), &(relative.extension("hpp")) == &relative);
        test::expect(std::equal_to<>(), relative.string(), std::string("alpha/file.hpp"));
        test::expect(std::equal_to<>(), relative.extension(), std::string(".hpp"));
        test::expect(std::equal_to<>(), relative.to_native_path().string(), std::string("alpha/file.hpp"));
        test::expect(std::equal_to<>(), relative, api::relative_path_t("alpha/file.hpp"));
        test::expect(std::equal_to<>(), (relative + ".bak").string(),
            std::string("alpha/file.hpp.bak")
        );
        test::expect(std::equal_to<>(), (api::relative_path_t("alpha") / api::relative_path_t("beta/file.cpp")).string(),
            std::string("alpha/beta/file.cpp")
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = relative + "/suffix";
        });
        test::expect(std::equal_to<>(), std::format("{}", relative), relative.string());
        test::expect(std::equal_to<>(), std::hash<api::relative_path_t>()(relative),
            std::hash<api::relative_path_t>()(api::relative_path_t("alpha/file.hpp"))
        );

        const auto normalized_relative = api::relative_path_t("a/./b/..");
        test::expect(std::equal_to<>(), normalized_relative, api::relative_path_t("a"));
        test::expect(std::equal_to<>(), normalized_relative.string(), std::string("a"));
        test::expect(std::equal_to<>(), normalized_relative.to_native_path().string(), normalized_relative.string());
        test::expect(std::equal_to<>(), normalized_relative.stem(), std::string("a"));
        test::expect(std::equal_to<>(), normalized_relative.extension(), std::string());
        test::expect(std::equal_to<>(), std::format("{}", normalized_relative), normalized_relative.string());
        test::expect(std::equal_to<>(), std::hash<api::relative_path_t>()(normalized_relative),
            std::hash<api::relative_path_t>()(api::relative_path_t("a"))
        );

        const auto normalized = api::path_t(root.to_native_path() / "a" / "." / "b" / "..");
        test::expect(std::equal_to<>(), normalized, root / api::relative_path_t("a"));
        test::expect(std::equal_to<>(), normalized.parent(), root);
        test::expect(std::identity(), root.is_child(normalized));
        test::expect(std::identity(), !normalized.is_child(root));
        test::expect(std::identity(), !root.is_child(root));
        const auto dot_prefixed_child = root / api::relative_path_t("..cache/file");
        test::expect(std::identity(), root.is_child(dot_prefixed_child));
        test::expect(std::equal_to<>(), root.relative(dot_prefixed_child), api::relative_path_t("..cache/file"));
        test::expect(std::equal_to<>(), root.relative(normalized), api::relative_path_t("a"));
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = normalized.relative(root);
        });
        test::expect(std::identity(), normalized.is_sibling(root / api::relative_path_t("other")));
        test::expect(std::identity(), !normalized.is_sibling(root / api::relative_path_t("nested/other")));
        test::expect(std::equal_to<>(), normalized.filename(), std::string("a"));
        test::expect(std::equal_to<>(), std::string(normalized.c_str()), normalized.string());
        test::expect(std::equal_to<>(), normalized.stem(), std::string("a"));
        test::expect(std::equal_to<>(), normalized.extension(), std::string());
        test::expect(std::equal_to<>(), normalized.to_native_path().string(), normalized.string());
        test::expect(std::equal_to<>(), std::format("{}", normalized), normalized.string());
        test::expect(std::equal_to<>(), std::hash<api::path_t>()(normalized),
            std::hash<api::path_t>()(api::path_t(normalized.string()))
        );
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto invalid = api::path_t("/").parent();
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = root / api::relative_path_t("..");
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = root / api::relative_path_t(".");
        });

        auto extension_path = root / api::relative_path_t("name.old");
        test::expect(std::equal_to<>(), extension_path.stem(), std::string("name"));
        test::expect(std::equal_to<>(), extension_path.extension(), std::string(".old"));
        test::expect(std::identity(), &(extension_path.extension("new")) == &extension_path);
        test::expect(std::equal_to<>(), extension_path.filename(), std::string("name.new"));
        test::expect(std::equal_to<>(), (extension_path + ".backup").filename(),
            std::string("name.new.backup")
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = extension_path + "sub/path";
        });

        const auto source_dir = root / api::relative_path_t("source");
        const auto nested_dir = source_dir / api::relative_path_t("nested");
        const auto deep_dir = nested_dir / api::relative_path_t("deep");
        api::create_directories(deep_dir);
        api::create_directories(deep_dir);
        test::expect(std::identity(), api::exists(source_dir));
        test::expect(std::identity(), api::is_directory(source_dir));
        test::expect(std::identity(), !api::is_regular_file(source_dir));

        const auto cpp_file = source_dir / api::relative_path_t("one.cpp");
        const auto c_file = source_dir / api::relative_path_t("two.c");
        const auto h_file = source_dir / api::relative_path_t("three.h");
        const auto hpp_file = source_dir / api::relative_path_t("four.hpp");
        const auto text_file = source_dir / api::relative_path_t("five.txt");
        const auto nested_cpp = nested_dir / api::relative_path_t("nested.cpp");
        const auto deep_header = deep_dir / api::relative_path_t("deep.h");

        api::touch(cpp_file);
        api::touch(c_file);
        api::touch(h_file);
        api::touch(hpp_file);
        write_file(text_file, "hello");
        api::touch(nested_cpp);
        api::touch(deep_header);

        test::expect(std::identity(), api::exists(cpp_file));
        test::expect(std::identity(), api::is_regular_file(cpp_file));
        test::expect(std::identity(), !api::is_directory(cpp_file));
        test::expect(std::equal_to<>(), api::file_size(text_file), std::uintmax_t(5));
        test::expect_no_throw([&] { [[maybe_unused]] const auto value = api::last_write_time(text_file); });
        test::expect_no_throw([&] { api::touch(text_file); });
        test::expect(std::identity(), !api::exists(source_dir / api::relative_path_t("missing")));
        test::expect_throws<std::runtime_error>([&] {
            api::touch(root / api::relative_path_t("missing-parent/file"));
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto size = api::file_size(root / api::relative_path_t("missing"));
        });

        const api::rooted_path_t rooted_file(source_dir, api::relative_path_t("one.cpp"));
        test::expect(std::equal_to<>(), rooted_file.root(), source_dir);
        test::expect(std::equal_to<>(), rooted_file.relative_path(), api::relative_path_t("one.cpp"));
        test::expect(std::equal_to<>(), rooted_file.path(), cpp_file);
        test::expect(std::equal_to<>(), std::format("{}", rooted_file), cpp_file.string());
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const api::rooted_path_t invalid(
                source_dir,
                api::relative_path_t("missing")
            );
        });

        test::expect(std::identity(), api::find_include_predicate_t::include_all(cpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::is_dir(source_dir));
        test::expect(std::identity(), !api::find_include_predicate_t::is_dir(cpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::is_regular(cpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::cpp_file(cpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::c_file(c_file));
        test::expect(std::identity(), api::find_include_predicate_t::h_file(h_file));
        test::expect(std::identity(), api::find_include_predicate_t::hpp_file(hpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::filename("one.cpp")(cpp_file));
        test::expect(std::identity(), api::find_include_predicate_t::path(cpp_file)(cpp_file));
        test::expect(std::identity(), !api::find_include_predicate_t::path(cpp_file)(c_file));

        const auto source_predicate =
            api::find_include_predicate_t::is_regular
            && (api::find_include_predicate_t::cpp_file || api::find_include_predicate_t::h_file);
        test::expect(std::identity(), source_predicate(cpp_file));
        test::expect(std::identity(), source_predicate(h_file));
        test::expect(std::identity(), !source_predicate(c_file));
        test::expect(std::identity(), (!api::find_include_predicate_t::is_dir)(cpp_file));

        const auto make_include_predicate = [] {
            const auto regular = api::find_include_predicate_t::is_regular;
            const auto named = api::find_include_predicate_t::filename("one.cpp");
            return regular && named;
        };
        const auto owned_include_predicate = make_include_predicate();
        test::expect(std::identity(), owned_include_predicate(cpp_file));
        test::expect(std::identity(), !owned_include_predicate(c_file));

        const auto direct_entries = relative_strings(api::find(
            source_dir,
            api::find_include_predicate_t::include_all,
            api::find_descend_predicate_t::descend_none
        ));
        test::expect(std::equal_to<>(), direct_entries.size(), std::size_t(6));
        test::expect(std::identity(), contains(direct_entries, "one.cpp"));
        test::expect(std::identity(), contains(direct_entries, "nested"));
        test::expect(std::identity(), !contains(direct_entries, "nested/nested.cpp"));

        const auto all_regular = relative_strings(api::find(
            source_dir,
            api::find_include_predicate_t::is_regular,
            api::find_descend_predicate_t::descend_all
        ));
        test::expect(std::equal_to<>(), all_regular.size(), std::size_t(7));
        test::expect(std::identity(), contains(all_regular, "nested/nested.cpp"));
        test::expect(std::identity(), contains(all_regular, "nested/deep/deep.h"));

        const auto cpp_files = relative_strings(api::find(
            source_dir,
            api::find_include_predicate_t::cpp_file,
            api::find_descend_predicate_t::descend_all
        ));
        test::expect(std::equal_to<>(), cpp_files.size(), std::size_t(2));
        test::expect(std::equal_to<>(), cpp_files[0], std::string("nested/nested.cpp"));
        test::expect(std::equal_to<>(), cpp_files[1], std::string("one.cpp"));

        api::find_descend_predicate_t first_level_only(
            [](const api::path_t&, std::size_t depth) { return depth == 0; }
        );
        test::expect(std::identity(), first_level_only(nested_dir, 0));
        test::expect(std::identity(), !first_level_only(deep_dir, 1));
        test::expect(std::identity(), api::find_descend_predicate_t::descend_all(nested_dir, 100));
        test::expect(std::identity(), !api::find_descend_predicate_t::descend_none(nested_dir, 0));
        test::expect(std::identity(), (first_level_only || api::find_descend_predicate_t::descend_none)(nested_dir, 0));
        test::expect(std::identity(), !(first_level_only && api::find_descend_predicate_t::descend_none)(nested_dir, 0));
        test::expect(std::identity(), (!api::find_descend_predicate_t::descend_none)(nested_dir, 0));

        const auto make_descend_predicate = [] {
            const api::find_descend_predicate_t shallow(
                [](const api::path_t&, std::size_t depth) { return depth < 2; }
            );
            return shallow && api::find_descend_predicate_t::descend_all;
        };
        const auto owned_descend_predicate = make_descend_predicate();
        test::expect(std::identity(), owned_descend_predicate(nested_dir, 1));
        test::expect(std::identity(), !owned_descend_predicate(deep_dir, 2));

        const auto shallow = relative_strings(api::find(
            source_dir,
            api::find_include_predicate_t::is_regular,
            first_level_only
        ));
        test::expect(std::identity(), contains(shallow, "nested/nested.cpp"));
        test::expect(std::identity(), !contains(shallow, "nested/deep/deep.h"));

        const auto copied_file = root / api::relative_path_t("copy/parents/copied.txt");
        api::copy(text_file, copied_file);
        test::expect(std::identity(), api::is_regular_file(copied_file));
        test::expect(std::equal_to<>(), api::file_size(copied_file), std::uintmax_t(5));

        const auto copied_directory = root / api::relative_path_t("directory-copy");
        api::copy(nested_dir, copied_directory);
        test::expect(std::identity(), api::is_regular_file(copied_directory / api::relative_path_t("nested.cpp")));
        test::expect(std::identity(), api::is_regular_file(copied_directory / api::relative_path_t("deep/deep.h")));

        const auto file_link = root / api::relative_path_t("file-link");
        api::create_symlink(text_file, file_link);
        test::expect(std::identity(), api::exists(file_link));
        test::expect(std::identity(), api::is_regular_file(file_link));
        test::expect(std::equal_to<>(), api::canonical(file_link), api::canonical(text_file));

        const auto directory_link = root / api::relative_path_t("directory-link");
        api::create_directory_symlink(nested_dir, directory_link);
        test::expect(std::identity(), api::exists(directory_link));
        test::expect(std::identity(), api::is_directory(directory_link));
        test::expect(std::equal_to<>(), api::canonical(directory_link), api::canonical(nested_dir));
        test::expect_throws<std::runtime_error>([&] {
            api::create_directory_symlink(nested_dir, directory_link);
        });

        const auto found_through_root = relative_strings(api::find(
            root,
            api::find_include_predicate_t::filename("nested.cpp"),
            api::find_descend_predicate_t::descend_all
        ));
        test::expect(std::identity(), contains(found_through_root, "source/nested/nested.cpp"));
        test::expect(std::identity(), !contains(found_through_root, "directory-link/nested.cpp"));

        const auto strict_from = root / api::relative_path_t("strict-from");
        const auto strict_to = root / api::relative_path_t("strict-to");
        api::touch(strict_from);
        api::rename_strict(strict_from, strict_to);
        test::expect(std::identity(), !api::exists(strict_from));
        test::expect(std::identity(), api::exists(strict_to));

        const auto strict_existing = root / api::relative_path_t("strict-existing");
        api::touch(strict_existing);
        test::expect_throws<std::runtime_error>([&] {
            api::rename_strict(strict_to, strict_existing);
        });
        test::expect(std::identity(), api::exists(strict_to));
        test::expect(std::identity(), api::exists(strict_existing));

        const auto replace_from = root / api::relative_path_t("replace-from");
        const auto replace_to = root / api::relative_path_t("replace-to");
        write_file(replace_from, "new");
        write_file(replace_to, "old-value");
        api::rename_replace(replace_from, replace_to);
        test::expect(std::identity(), !api::exists(replace_from));
        test::expect(std::equal_to<>(), api::file_size(replace_to), std::uintmax_t(3));

        const auto removable_file = root / api::relative_path_t("removable-file");
        api::touch(removable_file);
        test::expect(std::identity(), api::remove(removable_file));
        test::expect(std::identity(), !api::remove(removable_file));

        const auto removable_tree = root / api::relative_path_t("removable-tree/a/b");
        api::create_directories(removable_tree);
        api::touch(removable_tree / api::relative_path_t("file"));
        test::expect(std::identity(), 0 < api::remove_all(root / api::relative_path_t("removable-tree")));
        test::expect(std::equal_to<>(), api::remove_all(root / api::relative_path_t("removable-tree")),
            std::uintmax_t(0)
        );

        const auto original_current_path = api::current_path();
        api::current_path(root);
        test::expect(std::equal_to<>(), api::current_path(), root);
        const api::pretty_path_t pretty_child(source_dir);
        test::expect(std::equal_to<>(), pretty_child.string(), std::string("source"));
        test::expect(std::equal_to<>(), std::string(pretty_child.c_str()), pretty_child.string());
        test::expect(std::equal_to<>(), std::format("{}", pretty_child), std::string("source"));

        const api::pretty_path_t pretty_parent(root.parent());
        test::expect(std::equal_to<>(), pretty_parent.string(), root.parent().string());
        api::current_path(original_current_path);
    });
}
