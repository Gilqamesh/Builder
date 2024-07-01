#include "program.h"

#include <unordered_set>
#include <stdexcept>
#include <filesystem>
#include <format>
#include <functional>
#include <cmath>
#include <cassert>
#include <boost/asio/placeholders.hpp>

#include <iostream>

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

namespace program {

abs_ptr_t<context_t> g_context = 0;

base_t::base_t(abs_ptr_t<context_t> context, program_type_t program_type):
    m_program_type(program_type),
    m_inputs(context->m_shared_memory->get_allocator<offset_ptr_t<base_t>>()),
    m_outputs(context->m_shared_memory->get_allocator<offset_ptr_t<base_t>>()),
    m_input_locks(context->m_shared_memory->get_allocator<offset_ptr_t<base_t>>()) {
    m_process = 0;
    m_is_running = 0;
    m_n_runs = 0;
    m_n_finished_runs = 0;
    m_n_success = 0;
    m_n_failure = 0;
}

void base_t::add_input(offset_ptr_t<base_t> input) {
    std::function<offset_ptr_t<base_t>(offset_ptr_t<base_t>, offset_ptr_t<base_t>)> find_input;
    find_input = [&find_input](offset_ptr_t<base_t> program, offset_ptr_t<base_t> input) -> offset_ptr_t<base_t> {
        if (program == input) {
            return program;
        }

        for (offset_ptr_t<base_t> input : program->m_inputs) {
            if (find_input(program, input)) {
                return input;
            }
        }

        return 0;
    };

    if (find_input(this, input)) {
        throw std::runtime_error("input is already contained in program");
    }

    m_inputs.push_back(input);
    input->m_outputs.push_back(this);
}

void base_t::set_start(time_type_t t) {
    m_t_start = t;
    m_is_running = true;
    ++m_n_runs;
}

void base_t::set_success(time_type_t t) {
    m_t_success = t;
    ++m_n_success;
}

void base_t::set_failure(time_type_t t) {
    m_t_failure = t;
    ++m_n_failure;
}

void base_t::set_finish(time_type_t t) {
    m_t_finish = t;
    m_is_running = false;
    ++m_n_finished_runs;
}

process_t::process_t(
    abs_ptr_t<context_t> context,
    std::initializer_list<offset_ptr_t<base_t>> programs_to_run,
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    int expected_status_code
):
    base_t(context, PROGRAM_TYPE_PROCESS),
    m_force_restart(false),
    m_shared_unique_process_name(context->m_shared_memory->get_allocator<char>()) {
    m_child_process = 0;
    if (!context->m_shared_namespace->m_run_program[m_program_type]) {
        context->m_shared_namespace->m_run_program[m_program_type] = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t> context, offset_ptr_t<base_t>)>(
            [](abs_ptr_t<context_t> context, offset_ptr_t<base_t> base) {
                offset_ptr_t<process_t> self = static_cast<offset_ptr_t<process_t>>(base);

                // collect children process
                if (!self->m_child_process) {
                    return ;
                }

                if (!self->m_child_process->m_child->running()) {
                    self->m_child_process->m_child->join();

                    time_type_t time_cur = context->get_time();
                    int exit_code = self->m_child_process->m_child->exit_code();
                    if (exit_code == self->m_expected_status_code) {
                        // propagate unlock to all outputs
                        std::function<void(offset_ptr_t<base_t>)> unlock_outputs;
                        unlock_outputs = [&unlock_outputs](offset_ptr_t<base_t> program) {
                            for (auto& output : program->m_outputs) {
                                output->m_input_locks.erase(program);
                                unlock_outputs(output);
                            }
                        };
                        base->set_success(time_cur);
                        unlock_outputs(base);
                    } else {
                        base->set_failure(time_cur);
                    }
                    base->set_finish(time_cur);

                    delete self->m_child_process;
                    self->m_child_process = 0;
                }
            }
        );
    }
    if (!context->m_shared_namespace->m_describe_program[m_program_type]) {
        context->m_shared_namespace->m_describe_program[m_program_type] = context->m_shared_memory->malloc<std::string(*)(offset_ptr_t<base_t>)>(
            [](offset_ptr_t<base_t> base) -> std::string {
                (void) base;
                return "process";
            }
        );
    }
    
    m_expected_status_code = expected_status_code;
    for (offset_ptr_t<base_t> input : inputs) {
        add_input(input);
    }

    for (offset_ptr_t<base_t> program_to_run : programs_to_run) {
        if (program_to_run->m_process) {
            throw std::runtime_error("program '" + context->describe(program_to_run) + "' already has a process to run under: '" + std::string(program_to_run->m_process->m_shared_unique_process_name.begin(), program_to_run->m_process->m_shared_unique_process_name.end()) + "'");
        }
        program_to_run->m_process = this;
        m_outputs.push_back(program_to_run);
    }
}

offset_ptr_t<process_t> process(
    std::initializer_list<offset_ptr_t<base_t>> programs_to_run,
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    int expected_status_code
) {
    std::string unique_name("process_" + std::to_string(g_context->m_shared_namespace->m_unique_counter++));
    offset_ptr_t<process_t> result = g_context->malloc_program_named<process_t>(unique_name, programs_to_run, inputs, expected_status_code);
    result->m_shared_unique_process_name.assign(unique_name.begin(), unique_name.end());
    return result;
}

void process_t::run_impl_override(abs_ptr_t<context_t> context) {
    time_type_t t_cur = context->get_time();

    // todo: create a group (boost::process::group), as the process might create other processes..

    assert(
        (m_child_process && m_is_running) ||
        (!m_child_process && !m_is_running)
    );

    if (m_is_running) {
        m_child_process->m_child->terminate();
        m_child_process->m_child->wait();
        // todo: reinitialize existing memory
        delete m_child_process;
        m_child_process = 0;

        set_failure(t_cur);
        set_finish(t_cur);
        for (offset_ptr_t<base_t> output : m_outputs) {
            if (output->m_is_running) {
                output->set_failure(t_cur);
                output->set_finish(t_cur);
            }
            // note: set start of outputs so they don't get called again by an older input before the startup time of the process
            output->set_start(t_cur);
        }
    } else {
        // propagate lock to all outputs
        std::function<void(offset_ptr_t<base_t>)> lock_outputs;
        lock_outputs = [&lock_outputs](offset_ptr_t<base_t> program) {
            for (offset_ptr_t<base_t> output : program->m_outputs) {
                output->m_input_locks.insert(program);
                lock_outputs(output);
            }
        };

        // todo: if we are in a failed state, this won't be necessary, as unlocking should happen on success only
        for (offset_ptr_t<base_t> output : m_outputs) {
            lock_outputs(output);
        }
    }
    set_start(t_cur);

    m_child_process = new child_process_t(
        context,
        context->m_process_name + " " + context->m_shared_memory->m_shared_memory_name + " " + context->m_shared_namespace_name + " " + std::string(m_shared_unique_process_name.begin(), m_shared_unique_process_name.end())
    );
}

process_t::child_process_t::child_process_t(abs_ptr_t<context_t> context, const std::string& process_name):
    m_child_cout(context->m_io_context),
    m_child_cerr(context->m_io_context) {
    using namespace boost::process;
    using namespace boost::asio;

    async_read(
        m_child_cout,
        buffer(context->m_io_cout_buffer),
        bind_executor(
            context->m_strand,
            boost::bind(
                &child_process_t::read_handler,
                this,
                placeholders::error,
                placeholders::bytes_transferred,
                boost::ref(m_child_cout),
                boost::ref(context->m_io_cout_buffer),
                context
            )
        )
    );
    async_read(
        m_child_cerr,
        buffer(context->m_io_cerr_buffer),
        bind_executor(
            context->m_strand,
            boost::bind(
                &child_process_t::read_handler,
                this,
                placeholders::error,
                placeholders::bytes_transferred,
                boost::ref(m_child_cerr),
                boost::ref(context->m_io_cerr_buffer),
                context
            )
        )
    );

    m_child = new child(
        process_name,
        std_out > m_child_cout,
        std_err > m_child_cerr
    );
}

process_t::child_process_t::~child_process_t() {
    delete m_child;
}

void process_t::child_process_t::read_handler(
    const boost::system::error_code& e,
    std::size_t size,
    boost::process::async_pipe& pipe,
    std::vector<char>& buf,
    abs_ptr_t<context_t> context
) {
    using namespace boost::process;
    using namespace boost::asio;

    if (!e) {
        // lock
        std::cout.write(buf.data(), size);
        async_read(
            pipe,
            buffer(buf),
            bind_executor(
                context->m_strand,
                boost::bind(
                    &child_process_t::read_handler,
                    this,
                    placeholders::error,
                    placeholders::bytes_transferred,
                    boost::ref(pipe),
                    boost::ref(buf),
                    context
                )
            )
        );
        // unlock
    } else if (e == boost::asio::error::eof) {
        // lock
        std::cout.write(buf.data(), size);
        // unlock
    } else {
        // lock
        std::cerr << "async_read " << e.what() << std::endl;
        // unlock
    }
}

lambda_t::lambda_t(
    abs_ptr_t<context_t> context,
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    void (*fn)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)
):
    base_t(context, PROGRAM_TYPE_LAMBDA) {
    if (!context->m_shared_namespace->m_run_program[m_program_type]) {
        context->m_shared_namespace->m_run_program[m_program_type] = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t> context, offset_ptr_t<base_t>)>(
            [](abs_ptr_t<context_t> context, offset_ptr_t<base_t> base) {
                offset_ptr_t<lambda_t> self = static_cast<offset_ptr_t<lambda_t>>(base);

                (*self->m_fn)(context, self);
            }
        );
    }
    if (!context->m_shared_namespace->m_describe_program[m_program_type]) {
        context->m_shared_namespace->m_describe_program[m_program_type] = context->m_shared_memory->malloc<std::string(*)(offset_ptr_t<base_t>)>(
            [](offset_ptr_t<base_t> base) -> std::string {
                (void) base;
                return "lambda";
            }
        );
    }

    for (offset_ptr_t<base_t> input : inputs) {
        add_input(input);
    }

    m_fn = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)>(fn);
}

offset_ptr_t<lambda_t> lambda(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    void (*fn)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)
) {
    return g_context->malloc_program<lambda_t>(inputs, fn);
}

file_modified_t::file_modified_t(
    abs_ptr_t<context_t> context,
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    const std::string& path
):
    base_t(context, PROGRAM_TYPE_FILE_MODIFIED),
    m_path(context->m_shared_memory->get_allocator<char>()) {
    if (!context->m_shared_namespace->m_run_program[m_program_type]) {
        context->m_shared_namespace->m_run_program[m_program_type] = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)>(
            [](abs_ptr_t<context_t> context, offset_ptr_t<base_t> base) {
                offset_ptr_t<file_modified_t> self = static_cast<offset_ptr_t<file_modified_t>>(base);

                time_type_t t_cur = context->get_time();
                try {
                    std::string path(self->m_path.begin(), self->m_path.end());
                    time_type_t t_last_write_time = std::chrono::clock_cast<clock_type_t>(std::filesystem::last_write_time(path));
                    if (self->m_t_success == time_type_t() || self->m_t_success < t_last_write_time) {
                        base->set_start(t_last_write_time);
                        base->set_success(t_last_write_time);
                        base->set_finish(t_last_write_time);
                    }
                } catch (...) {
                    // todo: log?
                    base->set_start(t_cur);
                    base->set_failure(t_cur);
                    base->set_finish(t_cur);
                }
            }
        );
    }
    if (!context->m_shared_namespace->m_describe_program[m_program_type]) {
        context->m_shared_namespace->m_describe_program[m_program_type] = context->m_shared_memory->malloc<std::string(*)(offset_ptr_t<base_t>)>(
            [](offset_ptr_t<base_t> base) -> std::string {
                offset_ptr_t<file_modified_t> self = static_cast<offset_ptr_t<file_modified_t>>(base);

                return std::string(self->m_path.begin(), self->m_path.end()) + " " + std::format("{}", base->m_t_success);
            }
        );
    }

    m_path.assign(path.begin(), path.end());
    for (offset_ptr_t<base_t> input : inputs) {
        add_input(input);
    }

    context->run_impl(this);
}

offset_ptr_t<file_modified_t> file_modified(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    const std::string& path
) {
    return g_context->malloc_program<file_modified_t>(inputs, path);
}

pulse_t::pulse_t(abs_ptr_t<context_t> context):
    base_t(context, PROGRAM_TYPE_PULSE) {
    if (!context->m_shared_namespace->m_run_program[m_program_type]) {
        context->m_shared_namespace->m_run_program[m_program_type] = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)>(
            [](abs_ptr_t<context_t> context, offset_ptr_t<base_t> base) {
                time_type_t t_cur = context->get_time();
                base->set_start(t_cur);
                base->set_success(t_cur);
                base->set_finish(t_cur);
            }
        );
    }
    if (!context->m_shared_namespace->m_describe_program[m_program_type]) {
        context->m_shared_namespace->m_describe_program[m_program_type] = context->m_shared_memory->malloc<std::string(*)(offset_ptr_t<base_t>)>(
            [](offset_ptr_t<base_t> base) -> std::string {
                return std::format("time {}", base->m_t_success);
            }
        );
    }
}
offset_ptr_t<pulse_t> pulse() {
    return g_context->malloc_program<pulse_t>();
}

oscillator_t::oscillator_t(abs_ptr_t<context_t> context, std::initializer_list<offset_ptr_t<base_t>> inputs, double periodicity_s):
    base_t(context, PROGRAM_TYPE_OSCILLATOR),
    m_periodicity_s(periodicity_s) {
    if (!context->m_shared_namespace->m_run_program[m_program_type]) {
        context->m_shared_namespace->m_run_program[m_program_type] = context->m_shared_memory->malloc<void(*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)>(
            [](abs_ptr_t<context_t> context, offset_ptr_t<base_t> base) {
                offset_ptr_t<oscillator_t> self = static_cast<offset_ptr_t<oscillator_t>>(base);

                time_type_t t_cur = context->get_time();
                const double min_periodicity = 0.1;

                base->set_start(t_cur);

                if (
                    base->m_t_success == time_type_t() ||
                    self->m_periodicity_s < min_periodicity
                ) {
                    base->set_success(t_cur);
                    base->set_finish(t_cur);
                    return ;
                }

                const double t_epsilon = min_periodicity / 10.0; // to avoid modding down a whole period
                const double t_since_last_success = (std::chrono::duration_cast<std::chrono::duration<double>>(base->m_t_success - context->get_time_init())).count() + t_epsilon;
                const double t_since_last_success_modded = t_since_last_success - std::fmod(t_since_last_success, self->m_periodicity_s);
                const double t_since_init = (std::chrono::duration_cast<std::chrono::duration<double>>(t_cur - context->get_time_init())).count();
                if (t_since_last_success_modded + self->m_periodicity_s < t_since_init) {
                    base->set_success(t_cur);
                    base->set_finish(t_cur);
                    return ;
                }
            }
        );
    }
    if (!context->m_shared_namespace->m_describe_program[m_program_type]) {
        context->m_shared_namespace->m_describe_program[m_program_type] = context->m_shared_memory->malloc<std::string(*)(offset_ptr_t<base_t>)>(
            [](offset_ptr_t<base_t> base) -> std::string {
                offset_ptr_t<oscillator_t> self = static_cast<offset_ptr_t<oscillator_t>>(base);
                return "oscillator [" + std::to_string(self->m_periodicity_s) + "s]";
            }
        );
    }

    for (offset_ptr_t<base_t> input : inputs) {
        add_input(input);
    }
}

offset_ptr_t<oscillator_t> oscillator(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    double periodicity_s
) {
    return g_context->malloc_program<oscillator_t>(inputs, periodicity_s);
}

context_t::context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size):
    m_strand(boost::asio::make_strand(m_io_context)),
    m_fake_work(m_io_context),
    m_io_cout_buffer(1024),
    m_io_cerr_buffer(1024) {
    m_shared_memory = new shared_memory_t(shared_memory_name, shared_memory_size);

    m_io_thread = boost::thread(boost::bind(&boost::asio::io_context::run, &m_io_context));

    m_process_name = process_name;
    m_shared_namespace_name = shared_namespace_name;
    m_shared_namespace = m_shared_memory->malloc_named<shared_namespace_t>(shared_namespace_name, this);
    if (!m_shared_namespace) {
        throw std::runtime_error("could not create shared memory: '" + shared_namespace_name + "'");
    }

    for (size_t i = 0; i < ARRAY_SIZE(m_shared_namespace->m_run_program); ++i) {
        m_shared_namespace->m_run_program[i] = 0;
    }
    for (size_t i = 0; i < ARRAY_SIZE(m_shared_namespace->m_describe_program); ++i) {
        m_shared_namespace->m_describe_program[i] = 0;
    }
}

context_t::context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name):
    m_strand(boost::asio::make_strand(m_io_context)),
    m_fake_work(m_io_context),
    m_io_cout_buffer(1024),
    m_io_cerr_buffer(1024) {
    m_shared_memory = new shared_memory_t(shared_memory_name);

    m_io_thread = boost::thread(boost::bind(&boost::asio::io_context::run, &m_io_context));

    m_process_name = process_name;
    m_shared_namespace_name = shared_namespace_name;
    m_shared_namespace = m_shared_memory->find_named<shared_namespace_t>(shared_namespace_name);
    if (!m_shared_namespace) {
        throw std::runtime_error("could not find shared memory: '" + shared_namespace_name + "'");
    }
}

context_t::shared_namespace_t::shared_namespace_t(abs_ptr_t<context_t> context):
    m_programs(context->m_shared_memory->get_allocator<offset_ptr_t<base_t>>()) {
    m_t_init = context->get_time();
    m_unique_counter = 0;
}

context_t::~context_t() {
    m_io_context.stop();
    m_io_thread.join();
    delete m_shared_memory;
}

time_type_t context_t::get_time_init() {
    return m_shared_namespace->m_t_init;
}

time_type_t context_t::get_time() {
    return std::chrono::high_resolution_clock::now();
}

int context_t::run_preamble(offset_ptr_t<base_t> program, offset_ptr_t<base_t> caller) {
    if (program->m_inputs.empty()) {
        return 0;
    }

    if (!program->m_input_locks.empty()) {
        return 1;
    }

    if (program->m_process && caller == program->m_process) {
        run_postamble(program);
        return 1;
    }

    time_type_t most_recent_input;
    for (const auto& input :program-> m_inputs) {
        if (input->m_t_success < input->m_t_failure) {
            // input program in failed status
            return 1;
        }

        most_recent_input = std::max(most_recent_input, input->m_t_success);
    }

    if (most_recent_input <= program->m_t_success) {
        // program is more recent than our input programs
        return 1;
    }

    time_type_t most_oldest_output;
    for (const auto& output : program->m_outputs) {
        most_oldest_output = std::min(most_oldest_output, output->m_t_success);
    }

    if (most_recent_input <= most_oldest_output) {
        // all output programs are more recent than the inputs
        return 1;
    }

    return 0;
}

void context_t::run_impl(offset_ptr_t<base_t> program) {
    offset_ptr_t<void(*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)> run_program_fn = m_shared_namespace->m_run_program[program->m_program_type];
    if (!run_program_fn) {
        throw std::runtime_error("program did not register a run function");
    }

    (*run_program_fn)(this, program);
}

void context_t::run(offset_ptr_t<base_t> program, offset_ptr_t<base_t> caller) {
    offset_ptr_t<void(*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)> run_program_fn = m_shared_namespace->m_run_program[program->m_program_type];
    if (!run_program_fn) {
        throw std::runtime_error("program did not register a run function");
    }

    if (run_preamble(program, caller)) {
        return ;
    }

    if (program->m_process) {
        program->m_process->run_impl_override(this);
        return ;
    }

    run_impl(program);

    run_postamble(program);
}

int context_t::run_postamble(offset_ptr_t<base_t> program) {
    if (program->m_is_running || program->m_t_success <= program->m_t_failure) {
        return 1;
    }

    for (auto& output : program->m_outputs) {
        if (output->m_t_start < program->m_t_success) {
            run(output, program);
        }
    }

    return 0;
}

std::string context_t::describe(offset_ptr_t<base_t> program) {
    offset_ptr_t<std::string(*)(offset_ptr_t<base_t>)> describe_program_fn = m_shared_namespace->m_describe_program[program->m_program_type];
    if (!describe_program_fn) {
        throw std::runtime_error("program did not register a describe function");
    }

    return (*describe_program_fn)(program);
}

int process_driver(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: <bin> <shared_memory_name> <shared_namespace_name> <program_name>" << std::endl;
        return 1;
    }

    std::string process_name(argv[0]);
    std::string shared_memory_name(argv[1]);
    std::string shared_namespace_name(argv[2]);
    std::string program_name(argv[3]);

    // std::cout << "process_driver initializing shared memory '" << argv[1] << "'" << std::endl;

    context_t context(process_name, shared_memory_name, shared_namespace_name);

    // sleep(12);

    // std::cout << "process_driver finding process named '" << program_name << "'" << std::endl;

    offset_ptr_t<process_t> process = context.find_named_program<process_t>(program_name);
    if (!process) {
        std::cerr << "process_driver could not find process '" << program_name << "'" << std::endl;
        return 1;
    }

    // std::cout << "process_driver running programs..." << std::endl;

    for (offset_ptr_t<base_t> output : process->m_outputs) {
        // std::cout << "process_driver running '" << context.describe(output) << "'" << std::endl;
        context.run_impl(output);
    }

    // std::cout << "process_driver finished running programs!" << std::endl;

    return 0;
}

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size) {
    g_context = new context_t(process_name, shared_memory_name, shared_namespace_name, shared_memory_size);
}

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name) {
    g_context = new context_t(process_name, shared_memory_name, shared_namespace_name);
}

void deinit() {
    delete g_context;
}

} // namespace program
