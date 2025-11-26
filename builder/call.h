#ifndef CALL_H
# define CALL_H

struct call_t {
public:
    call_t(void* function_symbol);

    void call(void** args, const int* arg_types, int n_args, void* ret, int ret_type);

private:
    void* m_function_symbol;
};

#endif // CALL_H
