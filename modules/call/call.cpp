#include <call/call.h>

call_t::call_t(void* function_symbol) : m_function_symbol(function_symbol) {}

void call_t::call(void** args, const int* arg_types, int n_args, void* ret, int ret_type) {
    using raw_call_t = void (*)(void**, const int*, int, void*, int);
    if (m_function_symbol == nullptr) {
        return;
    }

    auto callable = reinterpret_cast<raw_call_t>(m_function_symbol);
    callable(args, arg_types, n_args, ret, ret_type);
}
