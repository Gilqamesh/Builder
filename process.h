#ifndef KERNEL_CPP_BUILDER_PROCESS_H
# define KERNEL_CPP_BUILDER_PROCESS_H

# include "filesystem.h"

# include <variant>
# include <vector>

namespace kernel {

namespace cpp_builder {

namespace process {

using process_arg_t = std::variant<std::string, filesystem::path_t>;

/**
 * Creates a new process with the given arguments and waits for it to complete.
 * Returns a non-negative exit code on success, or the negated value of the signal that caused the process to terminate.
 */
int create_and_wait(const std::vector<process_arg_t>& args);

/**
 * Replaces the current process with a new process with the given arguments.
 */
[[noreturn]] void exec(const std::vector<process_arg_t>& args);

} // namespace process

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_PROCESS_H
