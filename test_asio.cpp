#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/thread/thread.hpp>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

#include "ipc_cpp/mem.h"

struct garbage {
    garbage(
        const std::string& process_name,
        boost::asio::io_context& io
    ):
        m_strand(boost::asio::make_strand(io)),
        m_p(io),
        m_buf(1024) {
        using namespace boost::asio;
        using namespace boost::process;
        std::function<void(const boost::system::error_code& e, std::size_t size)> read_handler;
        read_handler = [&read_handler, this](const boost::system::error_code& e, std::size_t size) {
            if (!e) {
                // lock
                std::cout.write(m_buf.data(), size);
                // unlock
                async_read(m_p, buffer(m_buf), bind_executor(m_strand, read_handler));
            } else if (e == boost::asio::error::eof) {
                // lock
                std::cout.write(m_buf.data(), size);
                // unlock
            } else {
                // lock
                std::cerr << "async_read " << e.what() << std::endl;
                // unlock
            }
        };

        async_read(m_p, buffer(m_buf), bind_executor(m_strand, read_handler));

        m_c = new child(
            process_name + " kekw",
            std_out > m_p
        );
    }

    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::process::async_pipe m_p;
    std::vector<char> m_buf;
    boost::process::child* m_c;

    void wait() {
        m_c->wait();
    }
};

int main(int argc, char** argv) {
    if (argc == 1) {
        using namespace boost::asio;

        io_context io;
        garbage g(argv[0], io);
        boost::thread t(boost::bind(&io_context::run, &io));
        for (int i = 0; i < 10; ++i) {
            std::cout << "doing stuff in parent process" << std::endl;
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
        }
        t.join();
    } else {
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(2));
        std::cout << "Hello from child process" << std::endl;
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(2));
    }

    return 0;
}
