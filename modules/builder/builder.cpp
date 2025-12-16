#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

#include <string>
#include <filesystem>
#include <format>

MODULES_EXTERN void c_module__build_builder_artifacts(const c_module_t* c_module) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);

    const auto module_dir = cpp_module.root_dir / MODULES_DIR / BUILDER;
    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    const auto builder_obj = compiler_t::update_object_file(
        module_dir / BUILDER_CPP,
        {},
        { cpp_module.root_dir },
        {},
        cpp_module.artifact_dir / (BUILDER + std::string(".o")),
        true
    );

    const auto so = compiler_t::update_shared_libary(
        [&]() {
            std::vector<std::filesystem::path> objs;
            for (const auto& entry : std::filesystem::directory_iterator(module_dir)) {
                const auto& path = entry.path();
                const auto filename = path.filename().string();
                if (path.extension() != ".cpp" || filename == ORCHESTRATOR_CPP || filename == BUILDER_CPP) {
                    continue ;
                }

                const auto stem = path.stem().string();
                objs.push_back(
                    compiler_t::update_object_file(
                        path,
                        {},
                        { cpp_module.root_dir },
                        {},
                        cpp_module.artifact_dir / (stem + ".o"),
                        true
                    )
                );
            }
            return objs;
        }(),
        cpp_module.artifact_dir / API_SO_NAME
    );

    compiler_t::update_shared_libary({ builder_obj, so }, cpp_module.artifact_dir / BUILDER_SO);

    const auto orchestrator_bin = compiler_t::update_binary(
        {
            compiler_t::update_object_file(
                module_dir / ORCHESTRATOR_CPP,
                { },
                { cpp_module.root_dir },
                { { "VERSION", std::to_string(VERSION) } },
                cpp_module.artifact_dir / (ORCHESTRATOR_BIN + std::string(".o")),
                false
            ),
            builder_obj,
            so
        },
        cpp_module.artifact_dir / ORCHESTRATOR_BIN
    );
}

MODULES_EXTERN void c_module__build_module_artifacts(const c_module_t* c_module) {
}
