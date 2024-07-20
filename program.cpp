#include "program.h"

#include <unordered_set>
#include <stdexcept>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>

#include <cmath>
#include <cassert>
#include <csignal>

#include <boost/asio/placeholders.hpp>
#include <boost/process/spawn.hpp>

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

namespace program {

abs_ptr_t<context_t> g_context = 0;
offset_ptr_t<base_t> g_pulse = 0;
offset_ptr_t<base_t> g_oscillator_200ms = 0;

std::string debug_shared_string_to_std_string(const shared_string_t<char>& str) {
    return std::string(str.begin(), str.end());
}

std::string time_type__serialize(const time_type_t& t) {
    auto duration = t.time_since_epoch();
    auto duration_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    return std::to_string(duration_in_ns);
}

time_type_t time_type__deserialize(const std::string& serialized_time) {
    auto duration_in_ns = std::stoll(serialized_time);
    auto duration = std::chrono::nanoseconds(duration_in_ns);
    return time_type_t(duration);
}

base_t::base_t(const std::string& program_type):
    m_program_type(g_context->m_shared_memory, program_type),
    m_inputs(g_context->m_shared_memory),
    m_outputs(g_context->m_shared_memory) {
    m_is_running = 0;
    m_n_runs = 0;
    m_n_finished_runs = 0;
    m_n_success = 0;
    m_n_failure = 0;

    m_shared_memory_name_hash = std::hash<std::string>()(g_context->m_shared_memory->m_shared_memory_name);
    m_unique_counter = (*g_context->m_shared_memory->m_unique_counter)++;
    std::string unique_file_name("tmp_locks/" + std::to_string(m_shared_memory_name_hash) + "_" + std::to_string(m_unique_counter));
    {
        std::ofstream file_lock(unique_file_name, std::ofstream::trunc | std::ofstream::out);
        if (!file_lock) {
            throw std::runtime_error("Could not create file lock '" + unique_file_name + "'");
        }
    }

    m_file_lock = boost::interprocess::file_lock(unique_file_name.c_str());
}

base_t::~base_t() {
    std::string unique_file_name("tmp_locks/" + std::to_string(m_shared_memory_name_hash) + "_" + std::to_string(m_unique_counter));
    std::remove(unique_file_name.c_str());
}

void base_t::set_start(time_type_t t) {
    auto lock = scoped_lock_write();

    if (m_is_running) {
        set_finish(t);
    }

    m_t_start = t;
    m_is_running = true;
    ++m_n_runs;
}

void base_t::set_success(time_type_t t) {
    auto lock = scoped_lock_write();

    assert(m_is_running);

    m_t_success = t;
    ++m_n_success;
}

void base_t::set_failure(time_type_t t) {
    auto lock = scoped_lock_write();

    assert(m_is_running);

    m_t_failure = t;
    ++m_n_failure;
}

void base_t::set_finish(time_type_t t) {
    auto lock = scoped_lock_write();

    assert(m_is_running);

    m_t_finish = t;
    m_is_running = false;
    ++m_n_finished_runs;
}

boost::interprocess::sharable_lock<boost::interprocess::file_lock> base_t::scoped_lock_read() {
    return boost::interprocess::sharable_lock<boost::interprocess::file_lock>(m_file_lock);
}

boost::interprocess::scoped_lock<boost::interprocess::file_lock> base_t::scoped_lock_write() {
    return boost::interprocess::scoped_lock<boost::interprocess::file_lock>(m_file_lock);
}

process_t::process_t(
    std::initializer_list<offset_ptr_t<base_t>> inputs
):
    base_initializer_t<process_t>(
        inputs,
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            offset_ptr_t<process_t> self = static_cast<offset_ptr_t<process_t>>(base);
            
            if (base->m_is_running) {
                try {
                    if (g_context->m_group) {
                        // if I do this though, all output and their outputs needs to be finished.. this is not great..
                        g_context->m_group.terminate();
                    }
                    g_context->m_group = boost::move(boost::process::group());
                } catch (std::exception& e) {
                    std::cout << "kekw: " << e.what() << std::endl;
                }

                for (offset_ptr_t<base_t> output : base->m_outputs) {
                    if (output->m_is_running) {
                        output->set_finish(get_time());
                    }
                }
                base->set_finish(get_time());
            }
            base->set_start(get_time());

            // // note: set startup times on the outputs to prevent restarting before the process can start up
            // time_type_t time_cur = get_time();
            // for (offset_ptr_t<base_t> output : base->m_outputs) {
            //     output->set_start(time_cur);
            // }

            // read(context);

            boost::process::spawn(
                g_context->m_process_name + " " + g_context->m_shared_memory->m_shared_memory_name + " " + g_context->m_shared_namespace_name + " " + self->m_shared_unique_process_name + " " + time_type__serialize(time_propagate),
                g_context->m_group
                // std_out > m_child_cout,
                // std_err > m_child_cerr
            );

            return 1;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            offset_ptr_t<process_t> self = static_cast<offset_ptr_t<process_t>>(base);

            std::string result("process");
            if (self->m_child_id) {
                result += " [" + std::to_string(self->m_child_id) + "]";
            }

            return result;
        }
    ),
    m_shared_unique_process_name(g_context->m_shared_memory),
    m_child_cout(g_context->m_io_context),
    m_child_cerr(g_context->m_io_context) {
    m_child_id = 0;
}

offset_ptr_t<process_t> process(std::initializer_list<offset_ptr_t<base_t>> inputs) {
    std::string unique_name("process_" + std::to_string(g_context->m_shared_namespace->m_unique_counter++));
    offset_ptr_t<process_t> result = malloc_program_named<process_t>(unique_name, inputs);
    result->m_shared_unique_process_name.assign(unique_name.begin(), unique_name.end());
    return result;
}


void process_t::read() {
    using namespace boost::process;
    using namespace boost::asio;

    m_child_cout.async_read_some(
        buffer(g_context->m_io_cout_buffer),
        bind_executor(
            g_context->m_strand,
            boost::bind(
                &process_t::read_handler,
                this,
                placeholders::error,
                placeholders::bytes_transferred,
                boost::ref(m_child_cout),
                boost::ref(g_context->m_io_cout_buffer)
            )
        )
    );
    m_child_cerr.async_read_some(
        buffer(g_context->m_io_cerr_buffer),
        bind_executor(
            g_context->m_strand,
            boost::bind(
                &process_t::read_handler,
                this,
                placeholders::error,
                placeholders::bytes_transferred,
                boost::ref(m_child_cerr),
                boost::ref(g_context->m_io_cerr_buffer)
            )
        )
    );
}

void process_t::read_handler(
    const boost::system::error_code& e,
    std::size_t size,
    boost::process::async_pipe& pipe,
    std::vector<char>& buf
) {
    using namespace boost::process;
    using namespace boost::asio;

    if (!e) {
        // lock
        std::cout.write(buf.data(), size);
        pipe.async_read_some(
            buffer(buf),
            bind_executor(
                g_context->m_strand,
                boost::bind(
                    &process_t::read_handler,
                    this,
                    placeholders::error,
                    placeholders::bytes_transferred,
                    boost::ref(pipe),
                    boost::ref(buf)
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
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    lambda_fn_t fn
):
    base_initializer_t<lambda_t>(
        inputs,
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            (void) time_propagate;

            offset_ptr_t<lambda_t> self = static_cast<offset_ptr_t<lambda_t>>(base);

            // todo: no need to run it async, we can just call it here..
            // if (self->m_context_id == boost::this_process::get_id()) {
            // }

            // note: to avoid restarting
            base->set_start(get_time());
            send(self->m_context_id, custom_message_t::MESSAGE_LAMBDA, base, self->m_fn, time_propagate);

            // self->m_fn(context, base);

            return 1;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            (void) base;
            return "lambda";
        }
    ) {
    m_context_id = boost::this_process::get_id();
    m_fn = fn;
}

offset_ptr_t<lambda_t> lambda(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    lambda_t::lambda_fn_t fn
) {
    return malloc_program<lambda_t>(inputs, fn);
}

file_modified_t::file_modified_t(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    const std::string& path
):
    base_initializer_t<file_modified_t>(
        inputs,
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            (void) time_propagate;
            offset_ptr_t<file_modified_t> self = static_cast<offset_ptr_t<file_modified_t>>(base);

            time_type_t t_cur = get_time();

            base->set_start(t_cur);

            try {
                std::string path(self->m_path.begin(), self->m_path.end());
                time_type_t t_last_write_time = std::chrono::clock_cast<clock_type_t>(std::filesystem::last_write_time(path));
                if (self->m_t_success == time_type_t() || self->m_t_success < t_last_write_time) {
                    base->set_success(t_last_write_time);
                }
            } catch (...) {
                // todo: log?
                base->set_failure(t_cur);
            }

            base->set_finish(t_cur);

            return 0;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            offset_ptr_t<file_modified_t> self = static_cast<offset_ptr_t<file_modified_t>>(base);

            return std::string(self->m_path.begin(), self->m_path.end()) + " " + std::format("{}", base->m_t_success);
        }
    ),
    m_path(g_context->m_shared_memory) {
    m_path.assign(path.begin(), path.end());

    run_impl(this, get_time());
    propagate_past_events(this);
}

offset_ptr_t<file_modified_t> file_modified(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    const std::string& path
) {
    return malloc_program<file_modified_t>(inputs, path);
}

pulse_t::pulse_t():
    base_initializer_t<pulse_t>(
        {},
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            (void) time_propagate;

            time_type_t t_cur = get_time();
            base->set_start(t_cur);
            base->set_success(t_cur);
            base->set_finish(t_cur);

            return 0;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            return std::format("time {}", base->m_t_success);
        }
    ) {
}

offset_ptr_t<pulse_t> pulse() {
    return malloc_program<pulse_t>();
}

oscillator_t::oscillator_t(std::initializer_list<offset_ptr_t<base_t>> inputs, double periodicity_s):
    base_initializer_t<oscillator_t>(
        inputs,
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            (void) time_propagate;
            offset_ptr_t<oscillator_t> self = static_cast<offset_ptr_t<oscillator_t>>(base);

            time_type_t t_cur = get_time();

            base->set_start(t_cur);

            const double min_periodicity = 0.1;
            if (
                base->m_t_success == time_type_t() ||
                self->m_periodicity_s < min_periodicity
            ) {
                base->set_success(t_cur);
            } else {
                const double t_epsilon = min_periodicity / 10.0; // to avoid modding down a whole period
                const double t_since_last_success = (std::chrono::duration_cast<std::chrono::duration<double>>(base->m_t_success - get_time_init())).count() + t_epsilon;
                const double t_since_last_success_modded = t_since_last_success - std::fmod(t_since_last_success, self->m_periodicity_s);
                const double t_since_init = (std::chrono::duration_cast<std::chrono::duration<double>>(t_cur - get_time_init())).count();
                if (t_since_last_success_modded + self->m_periodicity_s < t_since_init) {
                    base->set_success(t_cur);
                }
            }

            base->set_finish(t_cur);

            return 0;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            offset_ptr_t<oscillator_t> self = static_cast<offset_ptr_t<oscillator_t>>(base);
            return "oscillator [" + std::to_string(self->m_periodicity_s) + "s]";
        }
    ),
    m_periodicity_s(periodicity_s) {
}

offset_ptr_t<oscillator_t> oscillator(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    double periodicity_s
) {
    return malloc_program<oscillator_t>(inputs, periodicity_s);
}

void context_t::init(const std::string& process_name, const std::string& shared_namespace_name) {
    m_io_thread = boost::thread(boost::bind(&boost::asio::io_context::run, &m_io_context));
    m_process_name = process_name;
    m_shared_namespace_name = shared_namespace_name;

    auto process_id = boost::this_process::get_id();
    auto message_queue_it = m_shared_namespace->m_message_queues.find(process_id);
    if (message_queue_it != m_shared_namespace->m_message_queues.end()) {
        throw std::runtime_error("message queue for process id already exists");
    }
    m_shared_namespace->m_message_queues.emplace(process_id, m_shared_memory->malloc<decltype(m_shared_namespace->m_message_queues)::mapped_type::value_type>(m_shared_memory));
    // m_shared_namespace->m_message_queues.emplace(process_id, m_shared_memory->malloc<shared_deque_t<offset_ptr_t<message_t<custom_message_t>>>>(m_shared_memory));

    m_queue_should_run = true;
    m_queue_thread = std::thread([this]() {
        auto message_queue_it = m_shared_namespace->m_message_queues.find(boost::this_process::get_id());
        assert(message_queue_it != m_shared_namespace->m_message_queues.end());
        auto message_queue = message_queue_it->second;
        assert(message_queue);

        while (m_queue_should_run) {
            while (!message_queue->empty()) {
                auto message = message_queue->front();
                message_queue->pop_front();
                dispatch(message);
            }
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.1));
        }
    });
}

context_t::context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size):
    m_io_context(),
    m_strand(boost::asio::make_strand(m_io_context)),
    m_fake_work(m_io_context),
    m_io_cout_buffer(4096),
    m_io_cerr_buffer(4096) {
    m_shared_memory = new shared_memory_t(shared_memory_name, shared_memory_size);

    m_shared_namespace = m_shared_memory->malloc_named<shared_namespace_t>(shared_namespace_name, m_shared_memory);
    if (!m_shared_namespace) {
        throw std::runtime_error("could not create shared memory: '" + shared_namespace_name + "'");
    }

    init(process_name, shared_namespace_name);
}

context_t::context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name):
    m_io_context(),
    m_strand(boost::asio::make_strand(m_io_context)),
    m_fake_work(m_io_context),
    m_io_cout_buffer(4096),
    m_io_cerr_buffer(4096) {
    m_shared_memory = new shared_memory_t(shared_memory_name);

    m_shared_namespace = m_shared_memory->find_named<shared_namespace_t>(shared_namespace_name);
    if (!m_shared_namespace) {
        throw std::runtime_error("could not find shared memory: '" + shared_namespace_name + "'");
    }

    init(process_name, shared_namespace_name);
}

context_t::~context_t() {
    m_io_context.stop();
    m_io_thread.join();

    m_queue_should_run = false;
    if (m_queue_thread.joinable()) {
        m_queue_thread.join();
    }

    auto process_id = boost::this_process::get_id();
    auto message_queue_it = m_shared_namespace->m_message_queues.find(process_id);
    assert(message_queue_it != m_shared_namespace->m_message_queues.end());
    auto message_queue = message_queue_it->second;
    m_shared_memory->free(message_queue);
    m_shared_namespace->m_message_queues.erase(message_queue_it);

    delete m_shared_memory;
}

shared_namespace_t::shared_namespace_t(abs_ptr_t<shared_memory_t> shared_memory):
    m_fn_signatures(shared_memory),
    m_programs(shared_memory),
    m_message_queues(shared_memory) {
    m_context_owner_id = boost::this_process::get_id();
    m_t_init = get_time();
    m_unique_counter = 0;
}

int process_driver(int argc, char** argv) {
    std::string process_name(argv[0]);

    if (argc != 5) {
        std::cerr << "usage: <" << process_name << "> <shared_memory_name> <shared_namespace_name> <program_name> <time_propagate>" << std::endl;
        return 1;
    }

    std::string shared_memory_name(argv[1]);
    std::string shared_namespace_name(argv[2]);
    std::string program_name(argv[3]);
    time_type_t time_propagate(time_type__deserialize(argv[4]));

    std::cout << "[" << boost::this_process::get_id() << "] initializing shared memory '" << argv[1] << "'" << std::endl;

    init(process_name, shared_memory_name, shared_namespace_name);

    std::cout << "[" << boost::this_process::get_id() << "] finding process named '" << program_name << "'" << std::endl;

    offset_ptr_t<process_t> process = find_named_program<process_t>(program_name);
    if (!process) {
        std::cerr << "[" << boost::this_process::get_id() << "] could not find process '" << program_name << "'" << std::endl;
        return 1;
    }

    std::cout << "[" << boost::this_process::get_id() << "] running programs..." << std::endl;

    for (offset_ptr_t<base_t> output : process->m_outputs) {
        std::cout << "[" << boost::this_process::get_id() << "] running '" << call_fn<describe_fn_t>(output, "describe", output) << "'" << std::endl;
        if (run_impl(output, time_propagate)) {
            continue ;
        }
        
        run_postamble(output, time_propagate);
    }

    std::cout << "[" << boost::this_process::get_id() << "] joining group!" << std::endl;
    if (g_context->m_group && g_context->m_group.joinable()) {
        g_context->m_group.join();
    }

    time_type_t time_cur = get_time();
    process->set_success(time_cur);
    process->set_finish(time_cur);

    std::cout << "[" << boost::this_process::get_id() << "] finished running programs!" << std::endl;

    deinit();

    // todo: redirect pipes to some other context before we terminate

    return 0;
}

static void cleanup_references(int signal) {
    delete g_context;
    exit(signal);
}

static void _init() {
    using namespace boost::interprocess;
    g_pulse = pulse();
    g_oscillator_200ms = oscillator({ g_pulse }, 0.2);
    std::signal(SIGTERM, &cleanup_references);
    std::signal(SIGINT, &cleanup_references);
}

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size) {
    assert(!g_context && "already initialized");
    g_context = new context_t(process_name, shared_memory_name, shared_namespace_name, shared_memory_size);
    _init();
}

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name) {
    assert(!g_context && "already initialized");
    g_context = new context_t(process_name, shared_memory_name, shared_namespace_name);
    _init();
}

void deinit() {
    delete g_context;
}

void run(offset_ptr_t<base_t> program) {
    run(program, get_time());
}

time_type_t get_time_init() {
    return g_context->m_shared_namespace->m_t_init;
}

time_type_t get_time() {
    return std::chrono::high_resolution_clock::now();
}

int run_preamble(offset_ptr_t<base_t> program, time_type_t time_propagate) {
    if (time_propagate < program->m_t_propagate) {
        return 1;
    }

    std::function<void(offset_ptr_t<base_t>)> propagate_time;
    propagate_time = [&propagate_time, time_propagate](offset_ptr_t<base_t> program) {
        if (time_propagate <= program->m_t_propagate) {
            return ;
        }
        program->m_t_propagate = time_propagate;

        for (offset_ptr_t<base_t> output : program->m_outputs) {
            propagate_time(output);
        }
    };
    propagate_time(program);

    if (program->m_inputs.empty()) {
        return 0;
    }

    for (offset_ptr_t<base_t> input : program->m_inputs) {
        assert(!(time_propagate < input->m_t_propagate) && "how didn't the input propogate it's propogation time to us?");
        if (input->m_t_propagate == time_propagate) {
            if (input->m_t_start < time_propagate) {
                // note: input has not run yet with the new time of propogation, so we cannot run yet as not all inputs are updated yet
                return 1;
            }
        }
    }

    time_type_t most_recent_input;
    for (offset_ptr_t<base_t> input : program-> m_inputs) {
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
    for (offset_ptr_t<base_t> output : program->m_outputs) {
        most_oldest_output = std::min(most_oldest_output, output->m_t_success);
    }

    if (most_recent_input <= most_oldest_output) {
        // all output programs are more recent than the inputs
        return 1;
    }

    return 0;
}

int run_impl(offset_ptr_t<base_t> program, time_type_t time_propagate) {
    // std::cout << "run: " << call_fn<describe_fn_t>(program, "describe", program) << std::endl;
    return call_fn<run_fn_t>(program, "run", program, time_propagate);
}

int run_postamble(offset_ptr_t<base_t> program, time_type_t time_propagate) {
    if (program->m_t_success <= program->m_t_failure) {
        return 1;
    }

    for (auto& output : program->m_outputs) {
        if (output->m_t_start < program->m_t_success) {
            run(output, time_propagate);
        }
    }

    return 0;
}

void run(offset_ptr_t<base_t> program, time_type_t time_propagate) {
    if (run_preamble(program, time_propagate)) {
        return ;
    }

    if (run_impl(program, time_propagate)) {
        return ;
    }

    run_postamble(program, time_propagate);
}

void propagate_past_events(offset_ptr_t<base_t> program) {
    time_type_t time_init = get_time_init();

    if (
        time_init <= program->m_t_success ||
        program->m_is_running
    ) {
        return ;
    }

    for (offset_ptr_t<base_t> input : program->m_inputs) {
        if (
            program->m_t_success <= input->m_t_success ||
            input->m_t_success < input->m_t_failure
        ) {
            continue ;
        } else if (input->m_t_success < time_init) {
            input->m_t_success = program->m_t_success;
            propagate_past_events(input);
        }
    }
}

void add_input(offset_ptr_t<base_t> program, offset_ptr_t<base_t> input) {
    program->m_inputs.push_back(input);
    input->m_outputs.push_back(program);

    validate_no_circle(program);

    propagate_past_events(program);
}

void validate_no_circle(offset_ptr_t<base_t> program) {
    std::unordered_set<abs_ptr_t<base_t>> found_outputs;
    std::function<abs_ptr_t<base_t>(abs_ptr_t<base_t>)> find_circle;
    find_circle = [&find_circle, &found_outputs](abs_ptr_t<base_t> program) -> abs_ptr_t<base_t> {
        auto found_output_it = found_outputs.find(program);
        if (found_output_it != found_outputs.end()) {
            return *found_output_it;
        }
        found_outputs.insert(program);

        for (offset_ptr_t<base_t> output : program->m_outputs) {
            abs_ptr_t<base_t> fund_output = find_circle(output.get());
            if (fund_output) {
                return fund_output;
            }
        }

        return 0;
    };

    if (find_circle(program.get())) {
        // todo: display circle
        throw std::runtime_error("validate_no_circle found circle");
    }
}

void dispatch(offset_ptr_t<message_t<custom_message_t>> message) {
    switch (message->m_message_header.m_id) {
    case custom_message_t::MESSAGE_LAMBDA: {
        time_type_t time_propagated;
        lambda_t::lambda_fn_t fn;
        offset_ptr_t<base_t> base;
        (*message) >> time_propagated >> fn >> base;
        std::cout << "Got the message, executing lambda " << fn << " " << base << std::endl;
        fn(base);
        base->set_finish(get_time());
        run_postamble(base, time_propagated);
    } break ;
    default: {
        std::cerr << "Dispatch is not implemented for message type: " << static_cast<int>(message->m_message_header.m_id) << std::endl;
    }
    }
}

} // namespace program
