#ifndef M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
# define M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>
# include <string_view>

namespace create_module {

/**
 * Generated module name and directory.
 */
struct created_module_t {
    std::string name;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t directory;
};

/**
 * Returns a module name with a generated prefix and validated slug.
 */
std::string make_module_name(std::string_view name);

/**
 * Creates a boilerplate module under workspace and returns its name and path.
 */
created_module_t create(std::string_view workspace, std::string_view name);

} // namespace create_module

#endif // M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
