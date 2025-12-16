#include <modules/builder/cpp_module.h>
#include <modules/builder/compiler.h>

MODULES_EXTERN void c_module__build(const c_module_t* c_module) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*c_module);
    
    compiler_t::update_binary(
        {
            compiler_t::update_object_file(
                cpp_module.module_dir / "main.cpp",
                { },
                { cpp_module.root_dir },
                cpp_module.artifact_dir / "main.o",
                true
            )
        },
        cpp_module.artifact_dir / "example2"
    );
}
