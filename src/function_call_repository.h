#ifndef FUNCTION_CALL_REPOSITORY_H
# define FUNCTION_CALL_REPOSITORY_H

#include <unordered_map>
#include <function.h>

class function_call_repository_t {
public:
    void save(function_id_t id, function_t::function_call_t call);

    function_t::function_call_t load(const function_id_t& id) const;

private:
    std::unordered_map<function_id_t, function_t::function_call_t> m_function_calls;
};

#endif // FUNCTION_CALL_REPOSITORY_H
