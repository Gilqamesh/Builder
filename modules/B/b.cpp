#include "b.h"

#include <modules/C/c.h>

#include <iostream>

void b() {
    std::cout << "Hello from function b!" << std::endl;

    c();

    std::cout << "Bye from function b!" << std::endl;
}
