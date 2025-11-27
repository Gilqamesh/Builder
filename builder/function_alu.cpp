#include "function_alu.h"

#include "function_primitive_lang.h"

function_t function_alu_t::add(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "add", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a;
        if (!function.read(0, a)) {
            return ;
        }

        int b;
        if (!function.read(1, b)) {
            return ;
        }

        function.write(2, a + b);
    });
}

function_t function_alu_t::sub(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "sub", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a;
        if (!function.read(0, a)) {
            return ;
        }

        int b;
        if (!function.read(1, b)) {
            return ;
        }

        function.write(2, a - b);
    });
}

function_t function_alu_t::mul(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "mul", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a;
        if (!function.read(0, a)) {
            return ;
        }

        int b;
        if (!function.read(1, b)) {
            return ;
        }

        function.write(2, a * b);
    });
}

function_t function_alu_t::div(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "div", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a;
        if (!function.read(0, a)) {
            return ;
        }

        int b;
        if (!function.read(1, b)) {
            return ;
        }

        if (b == 0) {
            // TODO: better error type
            throw std::runtime_error("division by zero");
        }

        function.write(2, a / b);
    });
}

function_t function_alu_t::cond(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "cond", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        bool condition;
        if (!function.read(0, condition)) {
            return ;
        }

        if (condition) {
            function.copy(1, 3);
        } else {
            function.copy(2, 3);
        }
    });
}

function_t function_alu_t::is_zero(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "is_zero", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int in;
        if (!function.read(0, in)) {
            return ;
        }

        function.write(1, in == 0);
    });
}

function_t function_alu_t::integer(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "integer", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        function.write(0, 0);
    });
}

function_t function_alu_t::logger(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "logger", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int in;
        if (function.read(0, in)) {
            std::ostream* os;
            if (function.read(1, os)) {
                *os << in << "\n";
            } else {
                std::cout << in << "\n";
            }
        }
    });
}

function_t function_alu_t::pin(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "pin", [](function_t& function, uint8_t argument_index) {
        for (int i = 0; i < function.arguments().size(); ++i) {
            if (i != argument_index && function.is_connected(i)) {
                function.copy(argument_index, i);
            }
        }
    });
}
