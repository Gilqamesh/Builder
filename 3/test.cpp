#include <iostream>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/bind/bind.hpp>

int main() {
    boost::asio::io_context io_context;
    boost::asio::strand<boost::asio::io_context::executor_type> strand(boost::asio::make_strand(io_context));

    boost::asio::steady_timer timer(io_context, boost::asio::chrono::seconds(1));

    size_t n_repeats = 5;
    std::function<void(const boost::system::error_code&)> fn;
    fn = [&](const boost::system::error_code& e) {
        (void) e;
        std::cout << "Timer: " << n_repeats << " " << boost::asio::chrono::high_resolution_clock::now() << std::endl;
        if (n_repeats--) {
            timer.expires_at(timer.expiry() + boost::asio::chrono::seconds(1));
            timer.async_wait(fn);
        }
    };
    timer.async_wait(fn);

    std::cout << "Running io context..." << std::endl;
    io_context.run();
    std::cout << "Io context finished running!" << std::endl;

    return 0;
}
