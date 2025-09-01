#include "builder.h"

#include "x86intrin.h"

int fact_native(int a) {
    if (a == 0) {
        return 1;
    }

    return a * fact_native(a - 1);
}

int main() {
    // wire_t a;
    // wire_t b;
    // wire_t mul_result;
    // // hand craft custom component: (a * b) + c -> display
    // mul_component_t mul;
    // mul.in(0, &a);
    // mul.in(1, &b);
    // mul.out(0, &mul_result);

    // wire_t c;
    // add_component_t add;
    // add.in(0, &mul_result);
    // add.in(1, &c);
    // wire_t add_result;
    // add.out(0, &add_result);

    // display_component_t display;
    // display.in(0, add.out(0));

    // a.write<int>(3);
    // b.write<int>(4);
    // c.write<int>(5);

    COMPILER.add<fact_component_t>("fact");

    wire_t a;
    wire_t b;
    fact_component_t fact;
    fact.connect(0, &a);
    fact.connect(1, &b);

    int result;

    a.write<int>(5);
    if (b.read<int>(result)) {
        std::cout << "result: " << result << std::endl;
    }

    // for (int i = 0; i < 1000; ++i) {
    //     size_t t_start = __rdtsc();
    //     in.write<int>(10);
    //     size_t t_end = __rdtsc();
    //     if (out.read<int>(result)) {
    //         std::cout << "result: " << result << std::endl;
    //     }
    //     std::cout << "i: " << i << ", " << (t_end - t_start) / 1000.0 << "kCy" << std::endl;
    // }

    // size_t t_start = __rdtsc();
    // int res = fact_native(10);
    // size_t t_end = __rdtsc();
    // std::cout << "res: " << res << ", fact_native: " << (t_end - t_start) / 1000.0 << "kCy" << std::endl;

    // in.write<int>(5);
    // if (out.read<int>(result)) {
    //     std::cout << "result: " << result << std::endl;
    // }

    // in.write<int>(10);
    // if (out.read<int>(result)) {
    //     std::cout << "result: " << result << std::endl;
    // }

    // t_start = __rdtsc();
    // in.write<int>(20);
    // if (out.read<int>(result)) {
    //     std::cout << "result: " << result << std::endl;
    // }
    // t_end = __rdtsc();
    // std::cout << (t_end - t_start) / 1000.0 << "kCy" << std::endl;

    return 0;
}
