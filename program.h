#ifndef PROGRAM_H
# define PROGRAM_H

# include "ipc_cpp/mem.h"
# include "ipc_cpp/msg.h"

# include <string>
# include <chrono>
# include <initializer_list>
# include <vector>
# include <unordered_map>
# include <boost/process.hpp>
# include <boost/process/async_pipe.hpp>
# include <boost/thread/thread.hpp>
# include <boost/asio/io_service.hpp>
# include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
# include <boost/interprocess/sync/sharable_lock.hpp>
# include <boost/interprocess/sync/scoped_lock.hpp>

namespace program {

struct process_t;
struct base_t;

using clock_type_t = std::chrono::high_resolution_clock;
using time_type_t  = std::chrono::time_point<clock_type_t>;

using run_fn_t      = int(*)(ipc_mem::offset_ptr_t<base_t>, time_type_t);
using stop_fn_t     = void(*)(ipc_mem::offset_ptr_t<base_t>);
using describe_fn_t = std::string(*)(ipc_mem::offset_ptr_t<base_t>);

std::string debug_shared_string_to_std_string(const ipc_mem::shared_string_t<char>& str);

std::string time_type__serialize(const time_type_t& t);
time_type_t time_type__deserialize(const std::string& serialized_time);

struct base_t {
    // todo: consider taking inputs here
    base_t(const std::string& program_type);

    ipc_mem::shared_string_t<char> m_program_type;
    
    bool m_is_running; // necessary as t_start == t_finish could mean either
    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;
    time_type_t m_t_propagate;

    size_t m_n_runs;
    size_t m_n_finished_runs;
    size_t m_n_success;
    size_t m_n_failure;

    ipc_mem::shared_vector_t<ipc_mem::offset_ptr_t<base_t>> m_inputs;
    ipc_mem::shared_vector_t<ipc_mem::offset_ptr_t<base_t>> m_outputs;

    void set_start(time_type_t t);
    void set_success(time_type_t t);
    void set_failure(time_type_t t);
    void set_finish(time_type_t t);
};

struct fn_signature_base_t { };

template <typename signature_t>
struct fn_signature_derived_t : public fn_signature_base_t {
    using name_to_fn_ptr_t = ipc_mem::shared_map_t<ipc_mem::shared_string_t<char>, ipc_mem::offset_ptr_t<signature_t>>;
    using fn_signature_t   = signature_t;

    ipc_mem::shared_map_t<
        ipc_mem::shared_string_t<char>,
        ipc_mem::offset_ptr_t<name_to_fn_ptr_t>
    > m_program_type_to_name_to_fn_ptr;

    signature_t add(ipc_mem::offset_ptr_t<base_t> program, const std::string& name, const signature_t& fn);
    signature_t find(ipc_mem::offset_ptr_t<base_t> program, const std::string& name);
};

template <typename program_t>
struct base_initializer_t : public base_t {
    base_initializer_t(
        std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
        run_fn_t run_fn,
        describe_fn_t describe_fn
    );
};

struct process_t : public base_initializer_t<process_t> {
    process_t(std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs);

    ipc_mem::shared_string_t<char> m_shared_unique_process_name;

    boost::process::async_pipe m_child_cout;
    boost::process::async_pipe m_child_cerr;

    void read();

    void read_handler(
        const boost::system::error_code& e,
        std::size_t size,
        boost::process::async_pipe& pipe,
        std::vector<char>& buf
    );
    int m_child_id;
};
// todo: provide a main method for the main process to write its implementation in,
// so this internal logic would not have to be exposed
// all new processes run through this function, call from main
int process_driver(int argc, char** argv);
ipc_mem::offset_ptr_t<process_t> process(std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs);

// todo(david): shell_t program

struct lambda_t : public base_initializer_t<lambda_t> {
    using lambda_fn_t = void(*)(ipc_mem::offset_ptr_t<base_t>);

    lambda_t(
        std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
        lambda_fn_t fn
    );

    int m_context_id;
    lambda_fn_t m_fn;
};
ipc_mem::offset_ptr_t<lambda_t> lambda(
    std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
    lambda_t::lambda_fn_t fn
);

struct file_modified_t : public base_initializer_t<file_modified_t> {
    file_modified_t(
        std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
        const std::string& path
    );

    ipc_mem::shared_string_t<char> m_path;
};
ipc_mem::offset_ptr_t<file_modified_t> file_modified(
    std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
    const std::string& path
);

struct pulse_t : public base_initializer_t<pulse_t> {
    pulse_t();
};
ipc_mem::offset_ptr_t<pulse_t> pulse();

struct oscillator_t : public base_initializer_t<oscillator_t> {
    oscillator_t(
        std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
        double periodicity_s
    );

    double m_periodicity_s;
};
ipc_mem::offset_ptr_t<oscillator_t> oscillator(
    std::initializer_list<ipc_mem::offset_ptr_t<base_t>> inputs,
    double periodicity_s
);

enum class custom_message_t : uint16_t {
    MESSAGE_LAMBDA
};

struct shared_namespace_t {
    int m_context_owner_id;

    int m_unique_counter;

    time_type_t m_t_init;

    ipc_mem::shared_map_t<
        ipc_mem::shared_string_t<char>,
        ipc_mem::offset_ptr_t<fn_signature_base_t>
    > m_fn_signatures;

    ipc_mem::shared_vector_t<ipc_mem::offset_ptr_t<base_t>> m_programs;

    ipc_mem::shared_map_t<
        int,
        ipc_mem::offset_ptr_t<ipc_mem::shared_deque_t<ipc_mem::offset_ptr_t<message_t<custom_message_t>>>>
    > m_message_queues;
};

struct context_t {
    std::string m_process_name;
    std::string m_shared_namespace_name; // todo(david): move into shared namespace
    ipc_mem::offset_ptr_t<shared_namespace_t> m_shared_namespace;

    boost::asio::io_context m_io_context;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::asio::io_service::work m_fake_work;
    boost::thread m_io_thread;
    std::vector<char> m_io_cout_buffer;
    std::vector<char> m_io_cerr_buffer;

    bool m_queue_should_run;
    std::thread m_queue_thread;
    boost::process::group m_group;

    // call from main process, which runs the program loop
    context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size);

    // call from any other process than the main process to access shared programs
    context_t(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name);

    // destroys shared programs if this was the main process
    ~context_t();
private:
    void init(const std::string& process_name, const std::string& shared_namespace_name);
};

extern ipc_mem::abs_ptr_t<context_t> g_context;
extern ipc_mem::offset_ptr_t<base_t> g_pulse;
extern ipc_mem::offset_ptr_t<base_t> g_oscillator_200ms; // already connected with pulse

void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name, size_t shared_memory_size);
void init(const std::string& process_name, const std::string& shared_memory_name, const std::string& shared_namespace_name);
void deinit();

void run(ipc_mem::offset_ptr_t<base_t> program = g_pulse);

template <typename signature_t>
signature_t add_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& name, const signature_t& fn);

template <typename signature_t>
signature_t find_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& name);

template <typename signature_t, typename... Args>
auto call_fn(ipc_mem::offset_ptr_t<base_t> program, const std::string& fn_name, Args&&... args) -> decltype(std::declval<signature_t>()(std::forward<Args>(args)...));

time_type_t get_time_init();
time_type_t get_time();

template <typename program_t, typename... Args>
ipc_mem::offset_ptr_t<program_t> malloc_program_named(const std::string& program_name, Args&&... args);

template <typename program_t, typename... Args>
ipc_mem::offset_ptr_t<program_t> malloc_program(Args&&... args);

template <typename program_t>
ipc_mem::offset_ptr_t<program_t> find_named_program(const std::string& object_name);

int  run_preamble(ipc_mem::offset_ptr_t<base_t> program, time_type_t time_propagate);
int  run_impl(ipc_mem::offset_ptr_t<base_t> program, time_type_t time_propagate);
int  run_postamble(ipc_mem::offset_ptr_t<base_t> program, time_type_t time_propagate);
void run(ipc_mem::offset_ptr_t<base_t> program, time_type_t time_propagate);

void propagate_past_events(ipc_mem::offset_ptr_t<base_t> program);
void add_input(ipc_mem::offset_ptr_t<base_t> program, ipc_mem::offset_ptr_t<base_t> input);
void validate_no_circle(ipc_mem::offset_ptr_t<base_t> program);

template <typename... Args>
int send(int address, const custom_message_t& type, Args&&... data);
void dispatch(ipc_mem::offset_ptr_t<message_t<custom_message_t>> message);

} // namespace program

# include "program_impl.h"

#endif // PROGRAM_H
