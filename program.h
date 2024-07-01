#ifndef PROGRAM_H
# define PROGRAM_H

# include "ipc_cpp/mem.h"

# include <string>
# include <chrono>
# include <initializer_list>
# include <vector>
# include <boost/process.hpp>
# include <boost/process/async_pipe.hpp>
# include <boost/thread/thread.hpp>
# include <boost/asio/io_service.hpp>

namespace program {

using clock_type_t = std::chrono::high_resolution_clock;
using time_type_t  = std::chrono::time_point<clock_type_t>;

struct process_t;
struct context_t;

enum program_type_t {
    PROGRAM_TYPE_PROCESS,
    PROGRAM_TYPE_LAMBDA,
    PROGRAM_TYPE_FILE_MODIFIED,
    PROGRAM_TYPE_PULSE,
    PROGRAM_TYPE_OSCILLATOR,

    _PROGRAM_TYPE_SIZE
};

struct base_t {
    // todo: consider taking inputs here
    base_t(abs_ptr_t<context_t> context, program_type_t program_type);

    program_type_t m_program_type;

    bool m_is_running; // necessary as t_start == t_finish could mean either
    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;

    size_t m_n_runs;
    size_t m_n_finished_runs;
    size_t m_n_success;
    size_t m_n_failure;

    shared_vector_t<offset_ptr_t<base_t>> m_inputs;
    shared_vector_t<offset_ptr_t<base_t>> m_outputs;

    // for asynchronous programs
    offset_ptr_t<process_t> m_process;

    shared_set_t<offset_ptr_t<base_t>> m_input_locks; // todo: implement as bitfield

    void add_input(offset_ptr_t<base_t> input);

    void set_start(time_type_t t);
    void set_success(time_type_t t);
    void set_failure(time_type_t t);
    void set_finish(time_type_t t);
};

struct process_t : public base_t {
    process_t(
        abs_ptr_t<context_t> context,
        std::initializer_list<offset_ptr_t<base_t>> programs_to_run,
        std::initializer_list<offset_ptr_t<base_t>> inputs,
        int expected_status_code
    );

    bool m_force_restart;
    int  m_expected_status_code;
    shared_string_t<char> m_shared_unique_process_name;

    struct child_process_t {
        child_process_t(abs_ptr_t<context_t> context, const std::string& process_name);
        ~child_process_t();
        boost::process::async_pipe m_child_cout;
        boost::process::async_pipe m_child_cerr;
        abs_ptr_t<boost::process::child> m_child;

        void read_handler(
            const boost::system::error_code& e,
            std::size_t size,
            boost::process::async_pipe& pipe,
            std::vector<char>& buf,
            abs_ptr_t<context_t> context
        );
    };
    abs_ptr_t<child_process_t> m_child_process;

    void run_impl_override(abs_ptr_t<context_t> context);
};
// all new processes run through this function, call from main
int process_driver(int argc, char** argv);
offset_ptr_t<process_t> process(
    std::initializer_list<offset_ptr_t<base_t>> programs_to_run,
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    int expected_status_code
);

struct lambda_t : public base_t {
    lambda_t(
        abs_ptr_t<context_t> context,
        std::initializer_list<offset_ptr_t<base_t>> inputs,
        void (*fn)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)
    );

    offset_ptr_t<void(*)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)> m_fn;
};
offset_ptr_t<lambda_t> lambda(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    void (*fn)(abs_ptr_t<context_t>, offset_ptr_t<lambda_t>)
);

struct file_modified_t : public base_t {
    file_modified_t(
        abs_ptr_t<context_t> context,
        std::initializer_list<offset_ptr_t<base_t>> inputs,
        const std::string& path
    );

    shared_string_t<char> m_path;
};
offset_ptr_t<file_modified_t> file_modified(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    const std::string& path
);

struct pulse_t : public base_t {
    pulse_t(abs_ptr_t<context_t> context);
};
offset_ptr_t<pulse_t> pulse();

struct oscillator_t : public base_t {
    oscillator_t(
        abs_ptr_t<context_t> context,
        std::initializer_list<offset_ptr_t<base_t>> inputs,
        double periodicity_s
    );

    double m_periodicity_s;
};
offset_ptr_t<oscillator_t> oscillator(
    std::initializer_list<offset_ptr_t<base_t>> inputs,
    double periodicity_s
);

struct context_t {
    // call from main process, which runs the program loop
    context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size);

    // call from any other process than the main process to access shared programs
    context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name);

    // destroys shared programs if this was the main process
    ~context_t();

    struct shared_namespace_t {
        shared_namespace_t(abs_ptr_t<context_t> context);

        int m_unique_counter;

        time_type_t m_t_init;

        offset_ptr_t<void (*)(abs_ptr_t<context_t>, offset_ptr_t<base_t>)> m_run_program[_PROGRAM_TYPE_SIZE];
        offset_ptr_t<std::string (*)(offset_ptr_t<base_t>)> m_describe_program[_PROGRAM_TYPE_SIZE];

        shared_vector_t<offset_ptr_t<base_t>> m_programs;
    };
    
    std::string m_process_name;
    std::string m_shared_namespace_name;
    abs_ptr_t<shared_memory_t> m_shared_memory;
    offset_ptr_t<shared_namespace_t> m_shared_namespace;
    boost::asio::io_context m_io_context;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::asio::io_service::work m_fake_work;
    boost::thread m_io_thread;
    std::vector<char> m_io_cout_buffer;
    std::vector<char> m_io_cerr_buffer;

    time_type_t get_time_init();
    time_type_t get_time();

    template <typename program_t, typename... Args>
    offset_ptr_t<program_t> malloc_program_named(const std::string& program_name, Args&&... args);

    template <typename program_t, typename... Args>
    offset_ptr_t<program_t> malloc_program(Args&&... args);

    template <typename program_t>
    offset_ptr_t<program_t> find_named_program(const std::string& object_name);

    void run(offset_ptr_t<base_t> program, offset_ptr_t<base_t> caller);
    int  run_preamble(offset_ptr_t<base_t> program, offset_ptr_t<base_t> caller);
    void run_impl(offset_ptr_t<base_t> program);
    int  run_postamble(offset_ptr_t<base_t> program);

    // should be short and descriptive one liner, without a terminating newline
    std::string describe(offset_ptr_t<base_t> program);
};

extern abs_ptr_t<context_t> g_context;

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size);
void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name);
void deinit();

} // namespace program

# include "program_impl.h"

#endif // PROGRAM_H
