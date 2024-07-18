#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <utility>
#include <memory>

#define LOG(x) do { \
    std::cout << x << std::endl; \
} while (0)

struct unique_wrapper_t {
    unique_wrapper_t(int* a): m_a(a) {
        LOG("unique_wrapper_t::unique_wrapper_t");
    };
    ~unique_wrapper_t() {
        LOG("unique_wrapper_t::~unique_wrapper_t " << m_a.get());
    }

    unique_wrapper_t(const unique_wrapper_t& other) = delete;
    unique_wrapper_t& operator=(const unique_wrapper_t& other) = delete;

    unique_wrapper_t(unique_wrapper_t&& other) noexcept = default;
    unique_wrapper_t& operator=(unique_wrapper_t&& other) noexcept = default;

    std::unique_ptr<int> m_a;
};

struct tmp_t {
    tmp_t(int* a):
        m_unique_wrapper(a) {
        LOG("tmp_t::tmp_t");
    }

    ~tmp_t() {
        LOG("tmp_t::~tmp_t");
    }

    tmp_t(const tmp_t& other) = delete;
    tmp_t& operator=(const tmp_t& other) = delete;

    tmp_t(tmp_t&& other) noexcept = default;
    tmp_t& operator=(tmp_t&& other) noexcept = default;

    unique_wrapper_t m_unique_wrapper;
};

int main() {
    tmp_t a(new int(5));
    tmp_t b(std::move(a));

    return 0;
}
