#include "program.h"

#include <signal.h>
#include <iostream>
#include <stdexcept>
#include <thread>

static struct {
    abs_ptr_t<program::context_t> context;
    offset_ptr_t<program::base_t> program_cur;
} _;

static void init(const std::string& process_name);
static void deinit();
static void update();

static void init(const std::string& process_name) {
    using namespace program;

    program::init(process_name, "driver", "shared_namespace_name", 8 * 1024);
    _.context = g_context;

    offset_ptr_t<pulse_t> updater = pulse();
    offset_ptr_t<oscillator_t> oscillator_200ms = oscillator({ updater }, 0.2);
    offset_ptr_t<process_t> proc = process(
        {
            lambda(
                { file_modified({ oscillator_200ms }, "driver.cpp") },
                [](abs_ptr_t<context_t> context, offset_ptr_t<lambda_t> lambda) {
                    time_type_t time_cur = context->get_time();
                    lambda->set_start(time_cur);
                    
                    std::cout << "Hello from lambda" << std::endl;
                    
                    lambda->set_success(time_cur);
                    lambda->set_finish(time_cur);
                }
            )
        },
        { oscillator_200ms },
        0
    );
    (void) proc;

    _.program_cur = updater;
}

static void deinit() {
    program::deinit();
}

static void update() {
    try {
        _.context->run(_.program_cur, 0);
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
