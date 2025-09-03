#include <iostream>
#include <vector>
#include <cstdint>

#include "x86intrin.h"

void f() {
    std::vector<uint8_t> v;
    std::string s;

    s += "asdasdasdasdasd";
    v.push_back(8);
}

int main() {
    size_t t_start = __rdtsc();

    f();

    size_t t_end = __rdtsc();

    std::cout << ((t_end - t_start) / 1000.0) << " kCy" << std::endl;

    return 0;
}
