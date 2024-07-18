#include "program.h"
#include "program_gfx/program_gfx.h"

#include <signal.h>
#include <iostream>
#include <stdexcept>
#include <thread>

static void init(const std::string& process_name);
static void deinit();
static void update();

static void init(const std::string& process_name) {
    using namespace program;

    program::init(process_name, "driver_gfx", "shared_namespace_name", 16 * 1024);

    program_visualizer();
}

static void deinit() {
    program::deinit();
}

static void update() {
    try {
        program::run();
    } catch (std::exception& e) {
        std::cerr << "exception caught: '" << e.what() << "'" << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 1) {
        return program::process_driver(argc, argv);
    }

    init(argv[0]);

    while (1) {
        update();
        // std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.1));
    }

    deinit();

    return 0;
}
