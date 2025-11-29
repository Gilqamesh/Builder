#include <function_alu.h>
#include <function_primitive_lang.h>
#include <iostream>

function_t* function_alu_t::add(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "add", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a + b);
    });
}

function_t* function_alu_t::sub(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "sub", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a - b);
    });
}

function_t* function_alu_t::mul(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "mul", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a * b);
    });
}

function_t* function_alu_t::div(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "div", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a / b);
        if (b == 0) {
            // TODO: better error type
            throw std::runtime_error("division by zero");
        }
    });
}

function_t* function_alu_t::cond(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "cond", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        bool condition = function.read(0);
        if (condition) {
            function.copy(1, 3);
        } else {
            function.copy(2, 3);
        }
    });
}

function_t* function_alu_t::is_zero(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "is_zero", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int in = function.read(0);

        function.write(1, in == 0);
    });
}

function_t* function_alu_t::integer(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "integer", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;

        function.write(0, 0);
    });
}

function_t* function_alu_t::logger(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "logger", [](function_t& function, uint8_t argument_index) {
        (void) argument_index;
        
        int in = function.read(0);
        try {
            std::ostream* os = function.read(1);
            *os << in << "\n";
        } catch (...) {
            std::cout << in << "\n";
        }
    });
}

function_t* function_alu_t::pin(typesystem_t& typesystem) {
    return function_primitive_lang_t::function(typesystem, "function_alu_t", "pin", [](function_t& function, uint8_t argument_index) {
        for (size_t i = 0; i < function.arguments().size(); ++i) {
            if (i != argument_index && function.is_connected(i)) {
                function.copy(argument_index, i);
            }
        }
    });
}

