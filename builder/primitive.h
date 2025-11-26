#ifndef PRIMITIVE_H
# define PRIMITIVE_H

# include "function_ir.h"

struct primitive_locator_t {
    std::string ns;
    std::string name;
};

class primitive_t {
public:
    static void save(const primitive_locator_t& locator, call_t primitive);
    static function_ir_t* load(const primitive_locator_t& locator);

private:
    static std::unordered_map<std::string, function_ir_t*> primitives;
};

#endif // PRIMITIVE_H
