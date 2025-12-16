#include "b.h"

#include <modules/C/c.h>
#include <modules/builder/compiler.h>

#include <iostream>

void b() {
    std::cout << "Hello from function b!" << std::endl;

    compiler_t::update_object_file(
        "modules/B/b.cpp",
        {},
        { "." },
        {},
        "modules/B/b.o",
        false
    );

    c();

    std::cout << "Bye from function b!" << std::endl;
}
