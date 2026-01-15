#include "filesystem.h"

namespace builder::filesystem {

std::vector<::filesystem::path_t> find(const module_builder_t* module_builder, const ::filesystem::find_include_predicate_t& include_predicate, const ::filesystem::find_descend_predicate_t& descend_predicate) {
    return ::filesystem::find(
        module_builder->source_dir(),
        include_predicate &&
        !::filesystem::find_include_predicate_t::path(module_builder->builder_source_path()) &&
        !::filesystem::find_include_predicate_t::path(module_builder->source_dir() / ::filesystem::relative_path_t(module_t::DEPS_JSON)),
        descend_predicate
    );
}

} // builder::filesystem
