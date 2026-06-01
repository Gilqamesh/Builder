#ifndef KERNEL_LINKER_H
# define KERNEL_LINKER_H

# include "graph.h"
# include "module_config.h"

# include <vector>

namespace kernel {

namespace linker {

struct link_input_group_t {
    std::vector<filesystem::path_t> libraries;
    bool static_library_group;
};

struct link_inputs_t {
    std::vector<link_input_group_t> groups;
};

link_inputs_t binary_link_inputs(const graph::module_t& module, module_config::module_config_t module_config);

} // namespace linker

} // namespace kernel

#endif // KERNEL_LINKER_H
