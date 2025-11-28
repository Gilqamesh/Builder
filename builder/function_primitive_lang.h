#ifndef FUNCTION_PRIMITIVE_LANG_H
# define FUNCTION_PRIMITIVE_LANG_H

#include <string>
#include <function.h>

class function_primitive_lang_t {
public:
    static function_t* function(typesystem_t& typesystem, std::string ns, std::string name, function_t::function_call_t function_call);
};

#endif // FUNCTION_PRIMITIVE_LANG_H
