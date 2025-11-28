#ifndef FUNCTION_IR_H
# define FUNCTION_IR_H

#include <vector>
#include <function_id.h>

struct function_ir_t {
    function_id_t function_id;
    int left;
    int right;
    int top;
    int bottom;

    struct child_t {
        function_id_t function_id;
        int left;
        int right;
        int top;
        int bottom;
    };
    std::vector<child_t> children;

    struct connection_info_t {
        uint16_t from_function_index;
        uint8_t from_argument_index;
        uint16_t to_function_index;
        uint8_t to_argument_index;
    };
    std::vector<connection_info_t> connections;
};

#endif // FUNCTION_IR_H
