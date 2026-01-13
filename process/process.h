#ifndef BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H
# define BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H

# include <builder/filesystem/filesystem.h>

# include <variant>
# include <vector>

using process_arg_t = std::variant<std::string, path_t>;

class process_t {
public:

public:
    static int create_and_wait(const std::vector<process_arg_t>& args);
    [[noreturn]] static void exec(const std::vector<process_arg_t>& args);
};

#endif // BUILDER_PROJECT_BUILDER_PROCESS_PROCESS_H
