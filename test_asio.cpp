#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/thread/thread.hpp>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

#include "ipc_cpp/mem.h"

struct stuff {
    stuff(
        const std::string& process_name,
        boost::asio::io_context& io,
        boost::asio::strand<boost::asio::io_context::executor_type>& strand
    ):
        m_p(io),
        m_buf(4096) {
        read(strand);

        using namespace boost::process;
        m_c = new child(
            process_name + " kekw",
            std_out > m_p
        );
    }

    void read(boost::asio::strand<boost::asio::io_context::executor_type>& strand) {
        using namespace boost::process;
        using namespace boost::asio;

        m_p.async_read_some(
            buffer(m_buf),
            bind_executor(
                strand,
                boost::bind(
                    &stuff::read_handler,
                    this,
                    placeholders::error,
                    placeholders::bytes_transferred,
                    boost::ref(m_p),
                    boost::ref(m_buf),
                    boost::ref(strand)
                )
            )
        );
    }

    void read_handler(
        const boost::system::error_code& e,
        std::size_t size,
        boost::process::async_pipe& pipe,
        std::vector<char>& buf,
        boost::asio::strand<boost::asio::io_context::executor_type>& strand
    ) {
        using namespace boost::process;
        using namespace boost::asio;

        if (!e) {
            // lock
            std::cout << "'";
            std::cout.write(buf.data(), size);
            std::cout << "'" << std::endl;
            std::cout << "Read " << size << " bytes" << std::endl;
            pipe.async_read_some(
                buffer(buf),
                bind_executor(
                    strand,
                    boost::bind(
                        &stuff::read_handler,
                        this,
                        placeholders::error,
                        placeholders::bytes_transferred,
                        boost::ref(pipe),
                        boost::ref(buf),
                        boost::ref(strand)
                    )
                )
            );
            // unlock
        } else if (e == boost::asio::error::eof) {
            // lock
            std::cout << "'";
            std::cout.write(buf.data(), size);
            std::cout << "'" << std::endl;
            std::cout << "Read " << size << " bytes" << std::endl;
            // unlock
        } else {
            // lock
            std::cerr << "async_read " << e.what() << std::endl;
            // unlock
        }
    }

    boost::process::async_pipe m_p;
    std::vector<char> m_buf;
    boost::process::child* m_c;

    void wait() {
        m_c->wait();
    }
};

int main(int argc, char** argv) {
    using namespace boost::asio;

    if (argc == 1) {
        io_context io;
        boost::asio::strand<boost::asio::io_context::executor_type> strand(boost::asio::make_strand(io));
        boost::asio::io_service::work fake_work(io);
        boost::thread t(boost::bind(&io_context::run, &io));

        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));

        stuff g(argv[0], io, strand);
        for (int i = 0; i < 10; ++i) {
            std::cout << "doing stuff in parent process" << std::endl;
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
        }
        g.wait();
        io.stop();
        t.join();
    } else {
        io_context io;
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand(boost::asio::make_strand(io));
        boost::asio::io_service::work fake_work(io);
        boost::thread t(boost::bind(&io_context::run, &io));

        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
        // printf("%s", "Hello from child process\n");
        std::cout << "Hello from child process" << std::endl;
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
        // printf("%s", "Supppppppppppppp\n");
        std::cout << "Supppppppppppppp" << std::endl;
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(10));

        io.stop();
        t.join();
    }

    return 0;
}
