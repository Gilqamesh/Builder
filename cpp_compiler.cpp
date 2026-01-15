#include "cpp_compiler.h"

namespace builder::cpp_compiler {

filesystem::path_t create_static_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_static_library_path
) {
    const auto exported_interfaces = module_builder->export_interfaces(library_type_t::STATIC);
    return ::cpp_compiler::create_static_library(
        module_builder->libraries_build_dir(library_type_t::STATIC),
        module_builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        module_builder->libraries_build_dir(library_type_t::STATIC) / relative_static_library_path
    );
}

filesystem::path_t create_shared_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_shared_library_path
) {
    const auto exported_interfaces = module_builder->export_interfaces(library_type_t::SHARED);
    return ::cpp_compiler::create_shared_library(
        module_builder->libraries_build_dir(library_type_t::SHARED),
        module_builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        {},
        module_builder->libraries_build_dir(library_type_t::SHARED) / relative_shared_library_path
    );
}

filesystem::path_t create_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_libary_stem,
    library_type_t library_type
) {
    switch (library_type) {
        case library_type_t::STATIC: return create_static_library(module_builder, source_files, define_key_values, relative_libary_stem + ".a");
        case library_type_t::SHARED: return create_shared_library(module_builder, source_files, define_key_values, relative_libary_stem + ".so");
        default: throw std::runtime_error(std::format("builder::create_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

filesystem::path_t create_binary(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type,
    const filesystem::relative_path_t& relative_binary_path
) {
    const auto exported_interfaces = module_builder->export_interfaces(library_type);
    const auto exported_libraries = module_builder->export_libraries(library_type);

    return ::cpp_compiler::create_binary(
        module_builder->import_build_dir(),
        module_builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        exported_libraries,
        library_type == library_type_t::SHARED,
        module_builder->import_build_dir() / relative_binary_path
    );
}

} // namespace builder::cpp_compiler
