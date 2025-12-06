#include <iostream>
#include <string>

#include "api.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: module_builder <module_name>" << std::endl;
        return 1;
    }

    std::string module_name = argv[1];
    return module_builder::build_module(module_name);
}

