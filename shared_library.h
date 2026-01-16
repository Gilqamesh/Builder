#ifndef SHARED_LIBRARY_SHARED_LIBRARY_H
# define SHARED_LIBRARY_SHARED_LIBRARY_H

# include "filesystem.h"

namespace shared_library {
// TODO: move symbol out of here; introduce better abstractions for loader / binary semantics

enum class lifetime_t {
    PROCESS, // Library is intended to live for the entire process lifetime
    DTOR // Library lifetime is tied to loader_t object lifetime
};

enum class symbol_resolution_t {
    NOW, // Resolve relocations at load time; fail early on missing symbols
    LAZY // Defer relocations until first use; failures may surface later
};

enum class symbol_visibility_t {
    LOCAL, // Symbols are not added to the global symbol table
    GLOBAL // Symbols are added to the global symbol table
};

/**
 * symbol_t
 * - Non-owning, type-erased symbol address returned by loader_t::resolve()
 * - Convertible to a function pointer type
 *
 * Invariants:
 * - m_symbol != nullptr
 *
 * Example:
 *   using fn_t = void (*)(const module_builder_t*, library_type_t);
 *   fn_t fn = lib.resolve("builder__export_interface");
 */
class symbol_t {
public:
    explicit symbol_t(void* symbol);

    template <typename F>
    operator F() const;

private:
    void* m_symbol;
};

/**
 * loader_t
 * 
 * RAII wrapper around a dynamically loaded shared library.
 *
 * Invariants:
 * - m_handle != nullptr after successful construction
 * - m_handle == nullptr only for moved-from objects
 *
 * Semantics:
 * - Loads a shared library in the ctor
 * - Resolves symbols by name via resolve()
 * - Throws std::runtime_error on any load/resolve failure
 *
 * Notes:
 * - symbol_resolution / symbol_visibility are ctor-only policy
 * - No type safety across ABI boundaries
 */
class loader_t {
public:
    loader_t(
        const filesystem::path_t& path,
        lifetime_t shared_library_lifetime,
        symbol_resolution_t symbol_resolution,
        symbol_visibility_t symbol_visibility
    );

    ~loader_t();

    loader_t(const loader_t& other) = delete;
    loader_t& operator=(const loader_t& other) = delete;

    loader_t(loader_t&& other) noexcept;
    loader_t& operator=(loader_t&& other) noexcept;

    /**
     * Resolve a symbol by name.
     * - Returns a symbol_t proxy
     * - Throws if symbol is missing or resolution fails
     */
    symbol_t resolve(const char* symbol) const;

private:
    void close_handle();

private:
    lifetime_t m_shared_library_lifetime;
    void* m_handle;
};

template <typename F>
symbol_t::operator F() const {
    static_assert(std::is_pointer_v<F>);
    static_assert(std::is_function_v<std::remove_pointer_t<F>>);
    return reinterpret_cast<F>(m_symbol);
}

} // namespace shared_library

#endif // SHARED_LIBRARY_SHARED_LIBRARY_H
