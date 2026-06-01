#ifndef KERNEL_MODULE_CONFIG_H
# define KERNEL_MODULE_CONFIG_H

# include <cstdint>

namespace kernel {

namespace module_config {

enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

struct module_config_t {
    library_type_t library_type;
};

} // namespace module_config

} // namespace kernel

#endif // KERNEL_MODULE_CONFIG_H
