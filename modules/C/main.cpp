#include <modules/B/b.h>
#include "c.h"

#include <iostream>

int main() {
    std::cout << "Hello from C main!" << std::endl;
    c();
    b();
    std::cout << "Bye from C main!" << std::endl;

    return 0;
}
