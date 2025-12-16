#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

MODULES_EXTERN void c_module__build(const c_module_t* c_module) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);
    
    compiler_t::update_shared_libary(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "example1.cpp",
                { },
                { cpp_module.root_dir },
                cpp_module.artifact_dir / "example1.o",
                true
            )
        },
        cpp_module.artifact_dir / API_SO_NAME
    );
}
