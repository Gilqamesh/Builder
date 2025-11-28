#ifndef FUNCTION_IR_ASSEMBLY_H
# define FUNCTION_IR_ASSEMBLY_H

#include <string>
#include <function_ir.h>

class function_ir_assembly_t {
public:
    function_ir_assembly_t(const function_ir_t& ir);

    const std::string& assembly() const;
    function_ir_t function_ir() const;

private:
    std::string m_assembly;
};

#endif // FUNCTION_IR_ASSEMBLY_H
