#include <iostream>
#include <string>

#include "number_types.h"
#include "graphics_types.h"

void test() {
    if (auto result = instance_rational_t(4, 5) + (instance_rational_t(10, 9))) {
        cout << result->to_string() << endl;
    }

    if (auto result = instance_complex_t(4.0, 5.1) + (instance_rational_t(10, 9))) {
        cout << result->to_string() << endl;
    }

    graphics_engine_t graphics_engine;

    graphics_engine.run();
}

void init_modules() {
    init_number_module();
    init_graphics_module();
}

void deinit_modules() {
    deinit_graphics_module();
    deinit_number_module();
}

#include <x86intrin.h>
int main() {
    size_t time_start = __rdtsc();
    init_modules();
    size_t time_end = __rdtsc();
    cout << "init types took: " << (time_end - time_start) / 1000.0 << "kCy" << endl;

    time_start = __rdtsc();
    test();
    time_end = __rdtsc();
    cout << "test took: " << (time_end - time_start) / 1000.0 << "kCy" << endl;

    time_start = __rdtsc();
    deinit_modules();
    time_end = __rdtsc();
    cout << "deinit types took: " << (time_end - time_start) / 1000.0 << "kCy" << endl;

    return 0;
}
