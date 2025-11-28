#ifndef FUNCTION_REPOSITORY_H
# define FUNCTION_REPOSITORY_H

#include <unordered_map>
#include <function.h>

struct function_repository_t {
public:
    struct entry_t {
        function_t::function_call_t call;
        function_ir_t ir;
    };

public:
    void save(function_ir_t ir, function_t::function_call_t call);

    entry_t load(const function_id_t& id);

private:
    std::unordered_map<function_id_t, entry_t> m_functions;
};

#endif // FUNCTION_REPOSITORY_H
