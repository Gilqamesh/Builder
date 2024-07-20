#include "mem.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <x86intrin.h>
#include <random>
#include <cassert>
#include <cstdio>
#include <system_error>
#include <boost/process.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

using namespace ipc_mem;

#define LOG(x) do { \
    std::cout << x << std::endl; \
} while (0)

struct test_result_t {
    bool   succeeded;
    size_t expected;
    size_t got;

    double time_taken;
    double time_read;
    double time_write;
};

static int rand_inclusive(int start, int end) {
    assert(start < end);
    return start + std::rand() % (end - start + 1);
}

static double rand_inclusive(double start, double end) {
    assert(start < end);
    return start + (rand() / (RAND_MAX / (end - start)));
}

struct data_t {
    size_t size = 0;
};

test_result_t test(size_t n_of_threads) {
    size_t time_start = __rdtsc();

    thread_safe_data_t<data_t> data;
    std::mutex mutex_thread_result;
    size_t n_total_runs_total = 0;
    double time_read_total = 0.0;
    double time_write_total = 0.0;
    const size_t n_of_operations = 50;

    std::vector<std::thread> threads;
    for (size_t thread_index = 0; thread_index < n_of_threads; ++thread_index) {
        threads.emplace_back([&data, &n_total_runs_total, &time_read_total, &time_write_total, &mutex_thread_result]() {
            for (size_t operation_index = 0; operation_index < n_of_operations; ++operation_index) {
                size_t operation_type = rand_inclusive(0, 5);
                if (operation_type == 0) {
                    size_t n_runs = rand_inclusive(100000, 100000 * 100);
                    mutex_thread_result.lock();
                    n_total_runs_total += n_runs;
                    mutex_thread_result.unlock();
                    size_t time_write_start = __rdtsc();
                    data.write([n_runs](data_t& data) {
                        for (size_t i = 0; i < n_runs; ++i) {
                            ++data.size;
                        }
                    });
                    size_t time_write_end = __rdtsc();
                    mutex_thread_result.lock();
                    time_write_total += time_write_end - time_write_start;
                    mutex_thread_result.unlock();
                } else {
                    size_t time_read_start = __rdtsc();
                    data.read([](const data_t& data) {
                        (void) data;
                        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(rand_inclusive(0.01, 0.1)));
                    });
                    size_t time_read_end = __rdtsc();
                    mutex_thread_result.lock();
                    time_read_total += time_read_end - time_read_start;
                    mutex_thread_result.unlock();
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    size_t time_end = __rdtsc();

    test_result_t result;

    data.read([&result, n_total_runs_total](const data_t& data) {
        result.succeeded = (n_total_runs_total == data.size);
        result.got = data.size;
    });

    result.expected = n_total_runs_total;
    result.time_taken = static_cast<double>(time_end - time_start);
    result.time_read = time_read_total;
    result.time_write = time_write_total;

    return result;
}

struct shared_t {
    shared_t() {
        m_n_total_runs_total = 0;
        m_time_read_total = 0.0;
        m_time_write_total = 0.0;
        m_time_taken_total = 0.0;
    }
    process_safe_data_t<data_t> m_data;

    boost::interprocess::interprocess_mutex m_mutex;
    size_t m_n_total_runs_total;
    double m_time_read_total;
    double m_time_write_total;
    double m_time_taken_total;

    const size_t n_of_operations = 50;
};

static int process_routine(int argc, char** argv) {
    assert(argc == 3);

    init(argv[1]);
    offset_ptr_t<shared_t> shared = find_named<shared_t>(argv[2]);

    for (size_t operation_index = 0; operation_index < shared->n_of_operations; ++operation_index) {
        size_t operation_type = rand_inclusive(0, 5);
        if (operation_type == 0) {
            size_t n_runs = rand_inclusive(100000, 100000 * 100);
            shared->m_mutex.lock();
            shared->m_n_total_runs_total += n_runs;
            shared->m_mutex.unlock();
            size_t time_write_start = __rdtsc();
            shared->m_data.write([n_runs](data_t& data) {
                for (size_t i = 0; i < n_runs; ++i) {
                    ++data.size;
                }
            });
            size_t time_write_end = __rdtsc();
            shared->m_mutex.lock();
            shared->m_time_write_total += time_write_end - time_write_start;
            shared->m_mutex.unlock();
        } else {
            size_t time_read_start = __rdtsc();
            shared->m_data.read([](const data_t& data) {
                (void) data;
                std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(rand_inclusive(0.01, 0.1)));
            });
            size_t time_read_end = __rdtsc();
            shared->m_mutex.lock();
            shared->m_time_read_total += time_read_end - time_read_start;
            shared->m_mutex.unlock();
        }
    }

    deinit();

    return 0;
}

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>

static test_result_t test(const std::string& process_name, size_t n_of_processes) {
    size_t time_start = __rdtsc();

    std::string shared_memory_name("asd");
    init(shared_memory_name, 16 * 1024);
    std::string shared_name("sdf");
    offset_ptr_t<shared_t> shared = malloc_named<shared_t>(shared_name);

    boost::process::group g;

    boost::asio::io_context io_context;
    boost::asio::io_service::work m_fake_work(io_context);

    LOG("Launching processes..");
    for (size_t process_index = 0; process_index < n_of_processes; ++process_index) {
        auto c = boost::process::child(
            process_name + " " + shared_memory_name + " " + shared_name,
            g,
            io_context,
            boost::process::on_exit = [](int exit, const std::error_code &ec) {
                LOG("Process exited with status code: " << exit << ", error code: '" << ec.message() << "'");
            }
        );
        c.detach();
        // boost::process::spawn(
        //     process_name + " " + shared_memory_name + " " + shared_name,
        //     g,
        //     boost::process::on_exit = [](int exit, const std::error_code &ec) {
        //         LOG("Process exited with status code: " << exit << ", error code: '" << ec.message() << "'");
        //     }
        // );
    }
    LOG("Launched processes!");

    std::thread context_thread([&io_context]() {
        LOG("Running context..");
        io_context.run();
        LOG("Ran context!");
    });

    LOG("Waiting on group..");
    std::error_code ec;
    g.wait(ec);
    if (ec.value()) {
        LOG("Error while waiting for group: " << ec.message());
    }
    LOG("Waited for group!");

    LOG("Joining asio context thread..");
    context_thread.join();
    LOG("Joined asio context thread!");

    size_t time_end = __rdtsc();

    test_result_t result;
    shared->m_data.read([&result, &shared](const data_t& data) {
        result.succeeded = (shared->m_n_total_runs_total == data.size);
        result.got = data.size;
    });


    result.expected = shared->m_n_total_runs_total;
    result.time_taken = static_cast<double>(time_end - time_start);
    result.time_read = shared->m_time_read_total;
    result.time_write = shared->m_time_write_total;

    deinit();

    return result;
}

int main(int argc, char** argv) {
    srand(42);

    if (argc == 3) {
        return process_routine(argc, argv);
    }

    const size_t n_of_threads = 50;
    const size_t n_of_tests = 5;
    double time_taken_total = 0.0;

    printf("Number of threads per test: %zu\n", n_of_threads);
    for (size_t test_index = 0; test_index < n_of_tests; ++test_index) {
        test_result_t test_result = test(argv[0], n_of_threads);
        // test_result_t test_result = test(n_of_threads);

        time_taken_total += test_result.time_taken;
        if (!test_result.succeeded) {
            printf(
                "!!! FAILURE !!! expected: %zu, got: %zu ",
                test_result.expected,
                test_result.got
            );
        }
        printf(
            "time taken (MCy): %.3lf, time read (MCy): %.3lf, time write (MCy): %.3lf\n",
            test_result.time_taken / 1000000.0,
            test_result.time_read / 1000000.0,
            test_result.time_write / 1000000.0
        );
    }

    LOG("Average time taken (MCy): " << (time_taken_total / 1000000.0) / n_of_tests);

    return 0;
}
