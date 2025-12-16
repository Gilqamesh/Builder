#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

#include <format>

MODULES_EXTERN void c_module__build_builder_artifacts(const c_module_t* c_module) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);

    // export so
    compiler_t::update_shared_libary(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "b.cpp",
                {},
                { cpp_module.root_dir },
                {},
                cpp_module.artifact_dir / "b.o",
                true
            )
        },
        cpp_module.artifact_dir / API_SO_NAME
    );

    // export lib
    compiler_t::update_static_library(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "b.cpp",
                {},
                { cpp_module.root_dir },
                {},
                cpp_module.artifact_dir / "b_static.o",
                false
            )
        },
        cpp_module.artifact_dir / API_LIB_NAME
    );
}

MODULES_EXTERN void c_module__build_module_artifacts(const c_module_t* c_module) {
    // bin
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);

    auto module_dependency_paths = [&]() {
        std::vector<std::filesystem::path> paths;
        for (const auto& dependency : cpp_module.module_dependencies) {
            const auto dependency_lib = dependency->artifact_dir / API_LIB_NAME;
            if (!std::filesystem::exists(dependency_lib)) {
                throw std::runtime_error(std::format("dependency lib does not exist '{}'", dependency_lib.string()));
            }
            paths.push_back(dependency_lib);
        }
        return paths;
    }();
    module_dependency_paths.push_back(
        compiler_t::update_object_file(
            cpp_module.module_dir / "main.cpp",
            {},
            { cpp_module.root_dir },
            {},
            cpp_module.artifact_dir / "main.o",
            false
        )
    );

    compiler_t::update_binary(module_dependency_paths, cpp_module.artifact_dir / "b_bin");
}
