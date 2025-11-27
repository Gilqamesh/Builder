#ifndef FUNCTION_REPOSITORY_H
# define FUNCTION_REPOSITORY_H

# include "function.h"

struct function_repository_t {
public:
    struct entry_t {
        function_t::function_call_t call;
        function_ir_t ir;
    };

    // Save compound function
    void save(function_id_t id, function_ir_t ir);

    // Save primitive function
    void save(function_id_t id, function_t::function_call_t call);

private:
    std::unordered_map<function_id_t, entry_t> m_functions;
};

#endif // FUNCTION_REPOSITORY_H
