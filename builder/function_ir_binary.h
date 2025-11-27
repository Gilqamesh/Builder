#ifndef FUNCTION_IR_BINARY_H
# define FUNCTION_IR_BINARY_H

# include "function_ir.h"

struct function_ir_binary_t {
public:
    function_ir_binary_t(const function_ir_t& ir);

    const std::vector<uint8_t>& bytes() const;

private:
    void serialize_function_id(const function_id_t& function_id);

private:
    std::vector<uint8_t> m_bytes;
};

#endif // FUNCTION_IR_BINARY_H
