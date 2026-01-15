#ifndef BUILDER_FILESYSTEM_H
# define BUILDER_FILESYSTEM_H

# include "module_builder.h"
# include "filesystem/filesystem.h"

namespace builder::filesystem {

/**
 * Same semantics as `filesystem::find(source_dir(builder), include_predicate, descend_predicate)`.
 * Also excludes `builder.cpp` and `deps.json` from the results.
 */
std::vector<::filesystem::path_t> find(const module_builder_t* module_builder, const ::filesystem::find_include_predicate_t& include_predicate, const ::filesystem::find_descend_predicate_t& descend_predicate);

} // builder::filesystem

#endif // BUILDER_FILESYSTEM_H
