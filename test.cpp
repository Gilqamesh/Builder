#include <iostream>
#include <cstdlib>

struct idk {

    static void* operator new(std::size_t size) {
        return malloc(size);
    }

};

int main() {
    void* r = idk::operator new(120);

    return 0;
}
