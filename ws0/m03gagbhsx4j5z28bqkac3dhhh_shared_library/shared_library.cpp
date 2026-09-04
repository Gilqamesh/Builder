# include "shared_library.h"

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <dlfcn.h>

# include <format>
# include <stdexcept>

namespace m03gagbhsx4j5z28bqkac3dhhh_shared_library {

symbol_t::symbol_t(void* symbol):
    m_symbol(symbol)
{
}

loader_t::loader_t(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
    lifetime_t lifetime,
    symbol_resolution_t symbol_resolution,
    symbol_visibility_t symbol_visibility
):
    m_shared_library_lifetime(lifetime),
    m_handle(nullptr)
{
    int dlopen_flags = 0;

    switch (lifetime) {
        case lifetime_t::PROCESS: {
            dlopen_flags |= RTLD_NODELETE;
        } break ;
        case lifetime_t::DTOR: {
        } break ;
        default: throw std::runtime_error(std::format("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::loader_t: unknown lifetime {}", static_cast<std::underlying_type_t<lifetime_t>>(lifetime)));
    }

    switch (symbol_resolution) {
        case symbol_resolution_t::NOW: {
            dlopen_flags |= RTLD_NOW;
        } break ;
        case symbol_resolution_t::LAZY: {
            dlopen_flags |= RTLD_LAZY;
        } break ;
        default: throw std::runtime_error(std::format("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::loader_t: unknown symbol_resolution {}", static_cast<std::underlying_type_t<symbol_resolution_t>>(symbol_resolution)));
    }

    switch (symbol_visibility) {
        case symbol_visibility_t::LOCAL: {
            dlopen_flags |= RTLD_LOCAL;
        } break ;
        case symbol_visibility_t::GLOBAL: {
            dlopen_flags |= RTLD_GLOBAL;
        } break ;
        default: throw std::runtime_error(std::format("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::loader_t: unknown symbol_visibility {}", static_cast<std::underlying_type_t<symbol_visibility_t>>(symbol_visibility)));
    }

    m_handle = dlopen(path.c_str(), dlopen_flags);
    if (m_handle == nullptr) {
        throw std::runtime_error(std::format("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::loader_t: failed to load shared library '{}': {}", path.string(), dlerror()));
    }
}

loader_t::~loader_t() {
    close_handle();
}

loader_t::loader_t(loader_t&& other) noexcept: loader_t()
{
    *this = std::move(other);
}

loader_t& loader_t::operator=(loader_t&& other) noexcept {
    if (this != &other) {
        close_handle();

        m_shared_library_lifetime = other.m_shared_library_lifetime;
        m_handle = other.m_handle;
        other.m_handle = nullptr;
    }

    return *this;

}

symbol_t loader_t::resolve(const char* symbol) const {
    if (symbol == nullptr) {
        throw std::invalid_argument("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::resolve: symbol must not be null");
    }
    if (m_handle == nullptr) {
        throw std::logic_error("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::resolve: loader has been moved from");
    }

    dlerror();
    void* result = dlsym(m_handle, symbol);
    const char* error = dlerror();
    if (error != nullptr) {
        throw std::runtime_error(std::format("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::resolve: failed to resolve symbol '{}': {}", symbol, error));
    }

    return symbol_t(result);
}

std::optional<symbol_t> loader_t::resolve_optional(const char* symbol) const {
    if (symbol == nullptr) {
        throw std::invalid_argument("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::resolve_optional: symbol must not be null");
    }
    if (m_handle == nullptr) {
        throw std::logic_error("m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t::resolve_optional: loader has been moved from");
    }

    dlerror();
    void* result = dlsym(m_handle, symbol);
    if (dlerror() != nullptr) {
        return std::nullopt;
    }

    return symbol_t(result);
}

loader_t::loader_t():
    m_shared_library_lifetime(lifetime_t::DTOR),
    m_handle(nullptr)
{
}

void loader_t::close_handle() {
    if (m_handle == nullptr) {
        return ;
    }

    // TODO: close on Windows only if lifetime is DTOR

    dlclose(m_handle);
    m_handle = nullptr;
}

} // namespace m03gagbhsx4j5z28bqkac3dhhh_shared_library
