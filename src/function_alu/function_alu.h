#ifndef FUNCTION_ALU_H
# define FUNCTION_ALU_H

#include <function.h>

class function_alu_t {
public:
    static function_t* add(typesystem_t& typesystem);
    static function_t* sub(typesystem_t& typesystem);
    static function_t* mul(typesystem_t& typesystem);
    static function_t* div(typesystem_t& typesystem);
    
    static function_t* cond(typesystem_t& typesystem);
    static function_t* is_zero(typesystem_t& typesystem);

    // todo: move these somewhere
    static function_t* integer(typesystem_t& typesystem);
    static function_t* logger(typesystem_t& typesystem);
    static function_t* pin(typesystem_t& typesystem);
};

#endif // FUNCTION_ALU_H
