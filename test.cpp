#include <iostream>
#include <functional>

struct b_t {
    template <typename F>
    b_t(F&& f): f(f) {}

    void (*f)();
};

int main() {
    int a = 3;

    auto f = [a] mutable {
        std::cout << a << std::endl;
        ++a;
    };

    b_t b(std::move(f));
    
    b.f();
    b.f();
    b.f();
    b.f();

    std::cout << "a: " << a << std::endl;

    return 0;
}
