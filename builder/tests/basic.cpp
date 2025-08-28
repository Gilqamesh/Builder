#include <gtest/gtest.h>

#include "builder.h"

TEST(Basic, Happy) {
    process_t p1("add",
        [](process_t* self) {
            self->send<int>(0, 0);
        },
        [](process_t* self) {
            int arg1 = self->receive<int>(0);
            int arg2 = self->receive<int>(1);
            self->send<int>(0, arg1 + arg2);
            return true;
        }
    );
    p1.define_in<int>(0, "in1");
    p1.define_in<int>(1, "in2");
    p1.define_out<int>(0, "out");

    process_t p2("1",
        [](process_t* self) {
            self->send<int>(0, 1);
        },
        [](process_t* self) {
            self->send<int>(0, 1);
            return true;
        }
    );
    p2.define_out<int>(0, "out");

    process_t p3("2",
        [](process_t* self) {
            self->send<int>(0, 2);
        },
        [](process_t* self) {
            self->send<int>(0, 2);
            return true;
        }
    );
    p3.define_out<int>(0, "out");

    process_t p4("display",
        [](process_t* self) {
            (void) self;
        },
        [](process_t* self) {
            std::cout << self->receive<int>(0) << std::endl;
            return true;
        }
    );
    p4.define_in<int>(0, "in1");

    connection_t connection1;
    connection_t connection2;
    connection_t connection3;

    p1.connect_in(0, &connection1);
    p1.connect_in(1, &connection2);
    p1.connect_out(0, &connection3);
    p2.connect_out(0, &connection1);
    p3.connect_out(0, &connection2);
    p4.connect_in(0, &connection3);

    p4.initialize();

    p2.call();
    p3.call();
    p3.call();
}

TEST(Boolean, Happy) {
    process_t cond(
        "cond",
        [](process_t* self) {
            self->send<bool>(0, true);
        },
        [](process_t* self) {
            self->send<bool>(0, true);
            return true;
        }
    );
    cond.define_out<bool>(0, "cond");

    process_t cons(
        "cons",
        [](process_t* self) {
            (void) self;
        },
        [](process_t* self) {
            (void) self;
            std::cout << "cons" << std::endl;
            return true;
        }
    );

    process_t alt(
        "alt",
        [](process_t* self) {
            (void) self;
        },
        [](process_t* self) {
            (void) self;
            std::cout << "alt" << std::endl;
            return true;
        }
    );
    process_t* branch = boolean_t::process_if(&cond, &cons, &alt);

    branch->initialize();
    branch->call();
}
