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
    double time_mixed;
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

    std::cout << "Starting test with '" << n_of_threads << "' threads, size of thread_safe_data_t: " << sizeof(data) << std::endl;

    std::mutex mutex_thread_result;
    size_t n_total_runs_total = 0;
    double time_read_total = 0.0;
    double time_write_total = 0.0;
    double time_mixed_total = 0.0;
    const size_t n_of_operations = 50;

    std::vector<std::thread> threads;
    std::mutex print_mutex;
    const auto read_fn = [&print_mutex, &mutex_thread_result, &time_read_total, &data]() {
        size_t time_read_start = __rdtsc();
        data.read([&print_mutex](auto& self, data_t& data) {
            (void) data;
            (void) self;
            (void) print_mutex;
            // {
            //     std::unique_lock<std::mutex> guard_print_mutex(print_mutex);
            //     std::cout << std::this_thread::get_id() << " read: " << data.size << ", ownership count: " << self.ownership_count() << std::endl;
            // }
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(rand_inclusive(0.001, 0.01)));
        });
        size_t time_read_end = __rdtsc();
        mutex_thread_result.lock();
        time_read_total += time_read_end - time_read_start;
        mutex_thread_result.unlock();
    };
    const auto write_fn = [&print_mutex, &mutex_thread_result, &n_total_runs_total, &time_write_total, &data]() {
        size_t n_runs = rand_inclusive(100000, 100000 * 100);
        mutex_thread_result.lock();
        n_total_runs_total += n_runs;
        mutex_thread_result.unlock();
        size_t time_write_start = __rdtsc();
        data.write([n_runs, &print_mutex](auto& self, data_t& data) {
            (void) self;
            (void) print_mutex;
            (void) data;
            // {
            //     std::unique_lock<std::mutex> guard_print_mutex(print_mutex);
            //     std::cout << std::this_thread::get_id() << " write: " << n_runs << ", ownership count: " << self.ownership_count() << std::endl;
            // }
            for (size_t i = 0; i < n_runs; ++i) {
                ++data.size;
            }
        });
        size_t time_write_end = __rdtsc();
        mutex_thread_result.lock();
        time_write_total += time_write_end - time_write_start;
        mutex_thread_result.unlock();
    };

    const auto mixed_fn = [&write_fn, &read_fn, &mutex_thread_result, &data, &time_mixed_total]() {
        size_t time_read_start = __rdtsc();
        data.write([&read_fn, &write_fn](auto& self, data_t& data) {
            (void) data;
            read_fn();
            self.write([&read_fn, &write_fn](auto& self, data_t& data) {
                (void) data;
                write_fn();
                self.read([&read_fn](auto& self, data_t& data) {
                    (void) data;
                    (void) self;
                    read_fn();
                    for (size_t i = 0; i < 3; ++i) {
                        read_fn();
                    }
                });
                for (size_t i = 0; i < 3; ++i) {
                    write_fn();
                }
                write_fn();
                read_fn();
            });
            read_fn();
        });
        size_t time_read_end = __rdtsc();
        mutex_thread_result.lock();
        time_mixed_total += time_read_end - time_read_start;
        mutex_thread_result.unlock();
    };

    for (size_t thread_index = 0; thread_index < n_of_threads; ++thread_index) {
        threads.emplace_back([&read_fn, &write_fn, &mixed_fn]() {
            for (size_t operation_index = 0; operation_index < n_of_operations; ++operation_index) {
                size_t operation_type = rand_inclusive(0, 10);
                if (operation_type == 0) {
                    write_fn();
                } else if (operation_type < 10) {
                    read_fn();
                } else {
                    mixed_fn();
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    size_t time_end = __rdtsc();

    test_result_t result;

    data.read([&result, n_total_runs_total](auto& self, const data_t& data) {
        (void) self;

        result.succeeded = (n_total_runs_total == data.size);
        result.got = data.size;
    });

    result.expected = n_total_runs_total;
    result.time_taken = static_cast<double>(time_end - time_start);
    result.time_read = time_read_total;
    result.time_write = time_write_total;
    result.time_mixed = time_mixed_total;

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
            shared->m_data.write([n_runs](auto& self, data_t& data) {
                (void) self;
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
            shared->m_data.read([](auto& self, const data_t& data) {
                (void) self;
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

static test_result_t test(const std::string& process_name, size_t n_of_processes) {
    size_t time_start = __rdtsc();

    std::string shared_memory_name("asd");
    init(shared_memory_name, 16 * 1024);
    std::string shared_name("sdf");
    offset_ptr_t<shared_t> shared = malloc_named<shared_t>(shared_name);

    boost::process::group g;

    for (size_t process_index = 0; process_index < n_of_processes; ++process_index) {
        boost::process::spawn(
            process_name + " " + shared_memory_name + " " + shared_name,
            g
        );
    }

    std::error_code ec;
    g.wait(ec);
    if (ec.value()) {
        LOG("Error while waiting for group: " << ec.message());
    }

    size_t time_end = __rdtsc();

    test_result_t result;
    shared->m_data.read([&result, &shared](auto& self, data_t& data) {
        (void) self;
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
        // test_result_t test_result = test(argv[0], n_of_threads);
        test_result_t test_result = test(n_of_threads);

        time_taken_total += test_result.time_taken;
        if (!test_result.succeeded) {
            printf(
                "!!! FAILURE !!! expected: %zu, got: %zu ",
                test_result.expected,
                test_result.got
            );
        }
        printf(
            "time taken (MCy): %.3lf, time read (MCy): %.3lf, time write (MCy): %.3lf, time mixed (MCy): %.3lf\n",
            test_result.time_taken / 1000000.0,
            test_result.time_read / 1000000.0,
            test_result.time_write / 1000000.0,
            test_result.time_mixed / 1000000.0
        );
    }

    LOG("Average time taken (MCy): " << (time_taken_total / 1000000.0) / n_of_tests);

    return 0;
}
