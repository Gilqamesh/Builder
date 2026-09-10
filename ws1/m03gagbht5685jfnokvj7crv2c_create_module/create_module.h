#ifndef M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
# define M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string_view>

namespace m03gagbht5685jfnokvj7crv2c_create_module {

/**
 * @brief Creates neutral module boilerplate with a new identity in an existing workspace.
 *
 * workspace is a workspace_name_t spelling such as "ws1": "ws" followed by a
 * non-negative decimal position representable by uint32_t. It is not an arbitrary
 * directory path; absolute paths and nested paths such as "ws1/nested" are rejected.
 * The destination workspace is invocation_context().workspace_root / workspace.
 * BUILDER_WORKSPACE_ROOT selects that root; when unset, it defaults to the current
 * working directory. A relative environment root resolves from that directory.
 * The selected workspace must already exist and be a directory.
 *
 * Root configuration is owned by
 * m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context(), which also reads
 * BUILDER_ARTIFACT_ROOT and exports both selected roots into the process environment.
 * Empty root environment values are errors. Coordinate calls with other process
 * environment/current-directory users and filesystem writers.
 *
 * friendly_name must be nonempty and contain only alphanumeric characters and
 * underscores, as required by module_name_t::from_friendly_name(). Each call
 * generates a fresh UUIDv7 identity; repeating a friendly name creates another module.
 * The new directory contains api.h with the complete module namespace/include guard,
 * cli.cpp with a greeting CLI, and an empty builder.cpp using default phase behavior.
 * No module-specific semantics or agent contract are generated.
 *
 * Existing destinations are rejected rather than overwritten. Invalid names/root
 * configuration, a missing or non-directory workspace, collisions, and file creation
 * or write failures throw. Identity/path validation exceptions propagate; friendly
 * name errors are std::invalid_argument and explicit scaffolding failures are
 * std::runtime_error. Creation is not transactional: a failure after directory
 * creation may leave partial boilerplate for the caller to inspect and remove.
 *
 * @return An owning absolute normalized path to the newly created module directory.
 *
 * @code{.cpp}
 * #include <m03gagbht5685jfnokvj7crv2c_create_module/create_module.h>
 * #include <iostream>
 *
 * int main() {
 *     // Configure BUILDER_WORKSPACE_ROOT with an existing ws1 child before launch,
 *     // or launch from that workspace root with BUILDER_WORKSPACE_ROOT unset.
 *     const auto module_dir = m03gagbht5685jfnokvj7crv2c_create_module::create("ws1", "example");
 *     std::cout << module_dir.string() << '\n';
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t create(std::string_view workspace, std::string_view friendly_name);

} // namespace m03gagbht5685jfnokvj7crv2c_create_module

#endif // M03GAGBHT5685JFNOKVJ7CRV2C_CREATE_MODULE_CREATE_MODULE_H
