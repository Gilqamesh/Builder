# include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
# include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>

# include <chrono>
# include <cstdint>
# include <cstdlib>
# include <dlfcn.h>
# include <filesystem>
# include <format>
# include <fstream>
# include <functional>
# include <stdexcept>
# include <string>
# include <type_traits>
# include <utility>

namespace api = m03gagbhsx4j5z28bqkac3dhhh_shared_library;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-shared-library-public-api-{}-{}",
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

filesystem_api::path_t build_library(
    const std::filesystem::path& directory,
    std::string_view name,
    std::string_view symbol,
    int value
) {
    const auto source = directory / (std::string(name) + ".c");
    const auto library = directory / ("lib" + std::string(name) + ".so");

    std::ofstream output(source);
    if (!output) {
        throw std::runtime_error("failed to create shared-library fixture source");
    }
    output << std::format("int {}(void) {{ return {}; }}\n", symbol, value);
    output.close();

    const auto command = std::format(
        "cc -shared -fPIC -o '{}' '{}'",
        library.string(),
        source.string()
    );
    if (std::system(command.c_str()) != 0) {
        throw std::runtime_error("failed to compile shared-library fixture");
    }

    return filesystem_api::path_t(library);
}

bool is_loaded(const filesystem_api::path_t& path) {
    dlerror();
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_NOLOAD);
    if (handle == nullptr) {
        return false;
    }
    dlclose(handle);
    return true;
}

} // namespace

int main() {
    return test::run([] {
        static_assert(!std::is_copy_constructible_v<api::loader_t>);
        static_assert(!std::is_copy_assignable_v<api::loader_t>);
        static_assert(std::is_move_constructible_v<api::loader_t>);
        static_assert(std::is_move_assignable_v<api::loader_t>);
        static_assert(api::lifetime_t::PROCESS != api::lifetime_t::DTOR);
        static_assert(api::symbol_resolution_t::NOW != api::symbol_resolution_t::LAZY);
        static_assert(api::symbol_visibility_t::LOCAL != api::symbol_visibility_t::GLOBAL);

        test::expect_no_throw([] {
            [[maybe_unused]] const api::symbol_t null_symbol(nullptr);
        });

        temporary_directory_t temporary_directory;
        const auto local_library = build_library(
            temporary_directory.path(),
            "local",
            "local_answer",
            11
        );
        const auto global_library = build_library(
            temporary_directory.path(),
            "global",
            "global_answer",
            22
        );
        const auto dtor_library = build_library(
            temporary_directory.path(),
            "dtor",
            "dtor_answer",
            33
        );
        const auto process_library = build_library(
            temporary_directory.path(),
            "process",
            "process_answer",
            44
        );
        const auto move_library = build_library(
            temporary_directory.path(),
            "move",
            "add_values",
            0
        );

        {
            std::ofstream output(temporary_directory.path() / "move.c");
            output << "int add_values(int a, int b) { return a + b; }\n";
            output.close();
            const auto command = std::format(
                "cc -shared -fPIC -o '{}' '{}'",
                move_library.string(),
                (temporary_directory.path() / "move.c").string()
            );
            test::expect(std::equal_to<>(), std::system(command.c_str()), 0);
        }

        using nullary_fn_t = int (*)();
        using binary_fn_t = int (*)(int, int);

        {
            api::loader_t loader(
                local_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
            nullary_fn_t answer = loader.resolve("local_answer");
            test::expect(std::equal_to<>(), answer(), 11);

            const auto optional_answer = loader.resolve_optional("local_answer");
            test::expect(std::identity(), optional_answer.has_value());
            nullary_fn_t optional_fn = *optional_answer;
            test::expect(std::equal_to<>(), optional_fn(), 11);

            test::expect(std::identity(), !loader.resolve_optional("missing_symbol"));
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto missing = loader.resolve("missing_symbol");
            });

            dlerror();
            test::expect(std::equal_to<>(), dlsym(RTLD_DEFAULT, "local_answer"), nullptr);
        }

        {
            api::loader_t loader(
                global_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::LAZY,
                api::symbol_visibility_t::GLOBAL
            );
            nullary_fn_t answer = loader.resolve("global_answer");
            test::expect(std::equal_to<>(), answer(), 22);

            dlerror();
            auto* global_symbol = dlsym(RTLD_DEFAULT, "global_answer");
            test::expect(std::identity(), global_symbol != nullptr);
            auto global_fn = reinterpret_cast<nullary_fn_t>(global_symbol);
            test::expect(std::equal_to<>(), global_fn(), 22);
        }

        test::expect(std::identity(), !is_loaded(dtor_library));
        {
            api::loader_t loader(
                dtor_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
            test::expect(std::identity(), is_loaded(dtor_library));
            nullary_fn_t answer = loader.resolve("dtor_answer");
            test::expect(std::equal_to<>(), answer(), 33);
        }
        test::expect(std::identity(), !is_loaded(dtor_library));

        test::expect(std::identity(), !is_loaded(process_library));
        {
            api::loader_t loader(
                process_library,
                api::lifetime_t::PROCESS,
                api::symbol_resolution_t::LAZY,
                api::symbol_visibility_t::LOCAL
            );
            nullary_fn_t answer = loader.resolve("process_answer");
            test::expect(std::equal_to<>(), answer(), 44);
        }
        test::expect(std::identity(), is_loaded(process_library));

        {
            api::loader_t original(
                move_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
            api::loader_t moved(std::move(original));
            test::expect_throws<std::logic_error>([&] {
                [[maybe_unused]] const auto invalid = original.resolve("add_values");
            });
            test::expect_throws<std::logic_error>([&] {
                [[maybe_unused]] const auto invalid = original.resolve_optional("add_values");
            });
            binary_fn_t add = moved.resolve("add_values");
            test::expect(std::equal_to<>(), add(19, 23), 42);

            api::loader_t destination(
                local_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
            test::expect(std::identity(), &(destination = std::move(moved)) == &destination);
            test::expect_throws<std::logic_error>([&] {
                [[maybe_unused]] const auto invalid = moved.resolve("add_values");
            });
            add = destination.resolve("add_values");
            test::expect(std::equal_to<>(), add(-3, 8), 5);
            test::expect(std::identity(), &(destination = std::move(destination)) == &destination);
            add = destination.resolve("add_values");
            test::expect(std::equal_to<>(), add(1, 2), 3);
            test::expect_throws<std::invalid_argument>([&] {
                [[maybe_unused]] const auto invalid = destination.resolve(nullptr);
            });
            test::expect_throws<std::invalid_argument>([&] {
                [[maybe_unused]] const auto invalid = destination.resolve_optional(nullptr);
            });
        }

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const api::loader_t missing(
                filesystem_api::path_t(temporary_directory.path() / "missing.so"),
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const api::loader_t invalid(
                local_library,
                static_cast<api::lifetime_t>(99),
                api::symbol_resolution_t::NOW,
                api::symbol_visibility_t::LOCAL
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const api::loader_t invalid(
                local_library,
                api::lifetime_t::DTOR,
                static_cast<api::symbol_resolution_t>(99),
                api::symbol_visibility_t::LOCAL
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const api::loader_t invalid(
                local_library,
                api::lifetime_t::DTOR,
                api::symbol_resolution_t::NOW,
                static_cast<api::symbol_visibility_t>(99)
            );
        });
    });
}
