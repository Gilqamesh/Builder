#include <exception>
#include <iostream>

#include "api.h"
#include "context.h"

int main(int argc, char **argv) {
    try {
        module_builder::Context ctx = module_builder::load_context(argc, argv);
        return module_builder::build(ctx);
    } catch (const std::exception &ex) {
        std::cerr << "module_builder error: " << ex.what() << "\n";
    }
    return 1;
}
