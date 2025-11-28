#ifndef FUNCTION_COMPOUND_H
# define FUNCTION_COMPOUND_H

#include <function.h>

class function_compound_t {
public:
    static function_t* function(typesystem_t& typesystem, function_ir_t function_ir);
};

#endif // FUNCTION_COMPOUND_H
