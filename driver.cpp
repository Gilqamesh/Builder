#include "program.h"

#include <signal.h>
#include <iostream>
#include <stdexcept>
#include <thread>

static void init(const std::string& process_name);
static void deinit();
static void update();

static void init(const std::string& process_name) {
    using namespace program;

    program::init(process_name, "driver", "shared_namespace_name", 8 * 1024);

    lambda(
        {
            file_modified(
                { g_oscillator_200ms },
                "driver.cpp"
            )
        },
        [](offset_ptr_t<base_t> base) {
            base->set_start(get_time());

            std::cout << "Hello from outer lambda" << std::endl;

            base->set_success(get_time());
            base->set_finish(get_time());
        }
    );
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
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.1));
    }

    deinit();

    return 0;
}
