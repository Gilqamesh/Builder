#include "int.h"

namespace int_t {

process_t* process_constant(int constant) {
    process_t* result = new process_t(
        "constant",
        [constant](process_t* self) {
            self->send<int>(0, constant);
        },
        [constant](process_t* self) {
            self->send<int>(0, constant);
            return true;
        }
    );

    result->define_out<int>(0, "out");

    return result;
}

process_t* process_add() {
    process_t* result = new process_t(
        "add",
        [](process_t* self) {
            self->send<int>(0, 0);
        },
        [](process_t* self) {
            const size_t size_in = self->size_in();
            int result = 0;
            for (size_t i = 0; i < size_in; ++i) {
                result += self->receive<int>(i);
            }
            self->send<int>(0, result);
            return true;
        }
    );

    result->define_in<int>(0, "a");
    result->define_in<int>(1, "b");
    result->define_out<int>(0, "sum");

    return result;
}

}