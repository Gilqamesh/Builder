#include "b.h"
#include <modules/C/c.h>

#include <iostream>

int main() {
    std::cout << "Hello from B main!" << std::endl;
    c();
    b();
    std::cout << "Bye from B main!" << std::endl;

    return 0;
}
