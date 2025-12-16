#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

MODULES_EXTERN void c_module__build_builder_artifacts(const c_module_t* c_module) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);

    compiler_t::update_shared_libary(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "c.cpp",
                {},
                { cpp_module.root_dir },
                {},
                cpp_module.artifact_dir / "c.o",
                true
            )
        },
        cpp_module.artifact_dir / API_SO_NAME
    );

    compiler_t::update_static_library(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "c.cpp",
                {},
                { cpp_module.root_dir },
                {},
                cpp_module.artifact_dir / "c_static.o",
                false
            )
        },
        cpp_module.artifact_dir / API_LIB_NAME
    );
}

MODULES_EXTERN void c_module__build_module_artifacts(const c_module_t* c_module) {
}
