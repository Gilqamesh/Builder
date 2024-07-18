#include "ipc_cpp/mem.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include <cstdio>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/process.hpp>

struct shared {
    boost::interprocess::file_lock lock;
};

#define LOG(x) do { \
    std::cout << "[" << boost::this_process::get_id() << "] " << x << std::endl; \
} while (0)

int main(int argc, char** argv) {
    if (argc == 1) {
        shared_memory_t mem("asdsdfgag", 1024);

        std::string shared_name("sup");
        std::string lock_file_name(shared_name + ".lock");

        {
            std::ofstream lock_file(lock_file_name);
            if (!lock_file) {
                std::cerr << "could not create lock file '" << lock_file_name << "'" << std::endl;
                return 1;
            }
        }

        offset_ptr_t<shared> shr = mem.malloc_named<shared>(shared_name);

        shr->lock = boost::interprocess::file_lock(lock_file_name.c_str());

        LOG("Waiting on lock");
        shr->lock.lock();
        LOG("Finished waiting on lock");

        LOG("Creating child process");
        boost::process::child c((std::string(argv[0]) + " " + mem.m_shared_memory_name + " " + shared_name).c_str());

        LOG("Waiting 1s..");
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));

        LOG("Unlocking lock");
        shr->lock.unlock();
        LOG("Finished unlocking lock");

        LOG("Waiting 1s..");
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));

        // LOG("Terminating child");
        // c.terminate();
        // LOG("Terminated child");

        LOG("Waiting on child process");
        c.wait();
        LOG("Finished waiting on child");

        LOG("Waiting on lock");
        shr->lock.lock();
        LOG("Finished waiting on lock");

        std::remove(lock_file_name.c_str());
    } else {
        if (argc != 3) {
            std::cerr << "usage: ..." << std::endl;
            return 1;
        }

        shared_memory_t mem(argv[1]);

        offset_ptr_t<shared> shr = mem.find_named<shared>(argv[2]);

        LOG("Waiting on lock");
        shr->lock.lock();
        LOG("Finished waiting on lock");

        LOG("Doing work..");
        while (1) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
        }
        LOG("Finished doing work!");

        LOG("Unlocking lock");
        shr->lock.unlock();
        LOG("Finished unlocking lock");
    }

    return 0;
}
