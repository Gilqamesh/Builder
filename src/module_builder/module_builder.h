#ifndef MODULE_CONTAINER_H
# define MODULE_CONTAINER_H

# include "module.h"
# include "module_repository.h"
# include "artifact_repository.h"

# include <string>
# include <unordered_map>
# include <stack>

class module_builder_t {
public:
    static void populate_dependencies(const module_repository_t& module_repository, module_t* module);

    static void compile_module(const artifact_repository_t& artifact_repository, module_t* module);

private:
    enum class dependency_population_state_t {
        NOT_POPULATED,
        PROCESSING,
        POPULATED,
    };

private:
    static void populate_dependencies(
        const module_repository_t& module_repository,
        module_t* module,
        std::unordered_map<module_t*, dependency_population_state_t>& dependency_population_state,
        std::stack<module_t*>& processed_stack
    );
};

#endif // MODULE_CONTAINER_H
