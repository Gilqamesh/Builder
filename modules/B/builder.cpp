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

#include <iostream>
MODULES_EXTERN void c_module__build_module_artifacts(const c_module_t* c_module, const char* static_libs) {
    // bin
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);

    const auto main_obj = (
        compiler_t::update_object_file(
            cpp_module.module_dir / "main.cpp",
            {},
            { cpp_module.root_dir },
            {},
            cpp_module.artifact_dir / "main.o",
            false
        )
    );

    std::cout << "Installing to: " << cpp_module.artifact_dir / "b_bin" << std::endl;
    compiler_t::update_binary({ main_obj, static_libs }, cpp_module.artifact_dir / "b_bin");
}
