#include "boolean.h"

namespace boolean_t {

process_t* process_if(process_t* condition, process_t* consequence, process_t* alternative) {
    process_t* result = new process_t(
        "if",
        [](process_t* self) {
            (void) self;
        },
        [](process_t* self) {
            if (self->receive<bool>(0)) {
                if (process_t* consequence = self->receive<process_t*>(1)) {
                    return consequence->call();
                }
            } else {
                if (process_t* alternative = self->receive<process_t*>(2)) {
                    return alternative->call();
                }
            }

            return false;
        }
    );

    result->define_in<bool>(0, "condition");
    result->define_in<process_t*>(1, "consequence");
    result->define_in<process_t*>(2, "alternative");

    if (!condition) {
        throw std::runtime_error("condition cannot be null");
    }
    connection_t* connection_cond = new connection_t;
    result->connect_in(0, connection_cond);
    condition->connect_out(condition->size_out(), connection_cond);

    if (consequence) {
        connection_t* connection_cons = new connection_t;
        result->connect_in(1, connection_cons);
        consequence->define_out<process_t*>(consequence->size_out(), "self");
        consequence->connect_out(consequence->size_out() - 1, connection_cons);

        connection_cons->is_initialized = true;
        connection_cons->message = (void*) consequence;
    }

    if (alternative) {
        connection_t* connection_alt = new connection_t;
        result->connect_in(2, connection_alt);
        alternative->define_out<process_t*>(alternative->size_out(), "self");
        alternative->connect_out(alternative->size_out() - 1, connection_alt);

        connection_alt->is_initialized = true;
        connection_alt->message = (void*) alternative;
    }

    return result;
}

}
