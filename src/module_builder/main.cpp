#include "api.h"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: module_builder <module_name>" << std::endl;
        return 1;
    }

    return build_module(argv[1]);
}
