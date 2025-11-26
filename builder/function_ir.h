#ifndef FUNCTION_IR_H
# define FUNCTION_IR_H

# include "libc.h"

struct function_ir_t {
    ulid::ULID ulid;

    struct dependency_info_t {
        function_ir_t* function_dependency;
        int left;
        int right;
        int top;
        int bottom;
    };
    std::vector<dependency_info_t> dependency_infos;

    struct connection_info_t {
        uint16_t from_function_index;
        uint8_t from_argument_index;
        uint16_t to_function_index;
        uint8_t to_argument_index;
    };
    std::vector<connection_info_t> connections;
};

#endif // FUNCTION_IR_H
