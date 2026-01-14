#ifndef BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H
# define BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H

# include "../filesystem/filesystem.h"

# include <variant>
# include <vector>

using process_arg_t = std::variant<std::string, path_t>;

class process_t {
public:

public:
    /**
     * Creates a new process with the given arguments and waits for it to complete.
     * Returns a non-negative exit code on success, or the negated value of the signal that caused the process to terminate.
     */
    static int create_and_wait(const std::vector<process_arg_t>& args);

    /**
     * Replaces the current process with a new process with the given arguments.
     */
    [[noreturn]] static void exec(const std::vector<process_arg_t>& args);
};

#endif // BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H
