#ifndef M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
# define M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>
# include <string_view>

namespace m03gagbht5685jfnokvj7crv2c_create_module {

/**
 * Creates a boilerplate module under workspace directory with the given friendly name.
 *
 * @return The path to the created module directory.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t create(std::string_view workspace, std::string_view friendly_name);

} // namespace m03gagbht5685jfnokvj7crv2c_create_module

#endif // M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
