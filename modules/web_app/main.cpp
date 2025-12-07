#include <iostream>
#include <web.h.in>
#include <thread>

int main() {
    /**
     * Example call:
     * std::string fetched_result;
     * async_fetch__create_async(
     *   "http", "127.0.0.1", 8081, "local.html",
     *   [&fetched_result](const char* serialized_data, size_t serialized_data_size) {
     *     fetched_result.assign(serialized_data, serialized_data + serialized_data_size);
     *     // process fetched result ...
     *   },
     *   []() {
     *     // log failure ...
     *   }
     * )
    */
    get_async(
        "https", "www.google.com", 443, "/",
        [](const unsigned char* serialized_data, size_t serialized_data_size) {
            std::string data((const char*) serialized_data, serialized_data_size);
            std::cout << "Asynchronously received data (" << serialized_data_size << " bytes):\n";
            std::cout << data << std::endl;
        },
        []() {
            std::cerr << "Failed to asynchronously fetch data." << std::endl;
        }
    );

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
