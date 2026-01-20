#include <kernel/cpp_builder/module_builder.h>
#include <kernel/cpp_builder/module.h>
#include <kernel/cpp_builder/cpp_compiler.h>

BUILDER_EXTERN void module_builder__export_interface(const builder::module_builder_t* module_builder, builder::library_type_t library_type) {
    for (const auto& interface : filesystem::find(module_builder->source_dir(), filesystem::find_include_predicate_t::h_file, filesystem::find_descend_predicate_t::descend_none)) {
        module_builder->install_interface(
            interface,
            module_builder->source_dir().relative(interface),
            library_type
        );
    }
}

BUILDER_EXTERN void module_builder__export_libraries(const builder::module_builder_t* module_builder, builder::library_type_t library_type) {
    const auto sources = filesystem::find(
        module_builder->source_dir(),
        filesystem::find_include_predicate_t::cpp_file &&
        !filesystem::find_include_predicate_t::filename(builder::module_t::BUILDER_CPP) &&
        !filesystem::find_include_predicate_t::filename(builder::module_t::DEPS_JSON),
        filesystem::find_descend_predicate_t::descend_none
    );
    switch (library_type) {
        case builder::library_type_t::STATIC: {
            const auto relative_builder_lib = filesystem::relative_path_t("builder.lib");
            const auto builder_lib = cpp_compiler::create_static_library(
                module_builder->libraries_build_dir(library_type),
                module_builder->source_dir(),
                module_builder->export_interfaces(library_type),
                sources,
                {},
                module_builder->libraries_install_dir(library_type) / relative_builder_lib
            );
            module_builder->install_library(builder_lib, relative_builder_lib, library_type);
        } break ;
        case builder::library_type_t::SHARED: {
            const auto relative_builder_so = filesystem::relative_path_t("builder.so");
            const auto builder_so = cpp_compiler::create_shared_library(
                module_builder->libraries_build_dir(library_type),
                module_builder->source_dir(),
                module_builder->export_interfaces(library_type),
                sources,
                {},
                {},
                module_builder->libraries_install_dir(library_type) / relative_builder_so
            );
            module_builder->install_library(builder_so, relative_builder_so, library_type);
        } break ;
        default: throw std::runtime_error(std::format("module_builder__export_libraries: unknown library_type_t {}", static_cast<int>(library_type)));
    }
}

BUILDER_EXTERN void module_builder__import_libraries(const builder::module_builder_t* module_builder) {
}
