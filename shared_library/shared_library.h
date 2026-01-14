#ifndef BUILDER_PROJECT_BUILDER_SHARED_LIBRARY_SHARED_LIBRARY_H
# define BUILDER_PROJECT_BUILDER_SHARED_LIBRARY_SHARED_LIBRARY_H

# include <builder/filesystem/filesystem.h>

enum class shared_library_lifetime_t {
    PROCESS, // Library is intended to live for the entire process lifetime
    DTOR // Library lifetime is tied to shared_library_t object lifetime
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
 * - Non-owning, type-erased symbol address returned by shared_library_t::resolve()
 * - Convertible to a function pointer type (unchecked; caller must provide correct signature)
 *
 * Invariants:
 * - m_symbol != nullptr
 *
 * Example:
 *   using fn_t = void (*)(const builder_t*, library_type_t);
 *   fn_t fn = lib.resolve("builder__export_interface");
 */
class symbol_t {
public:
    symbol_t(void* symbol);

    template <typename F>
    operator F() const;

private:
    void* m_symbol;
};

/**
 * shared_library_t
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
class shared_library_t {
public:
    shared_library_t(
        const path_t& path,
        shared_library_lifetime_t shared_library_lifetime,
        symbol_resolution_t symbol_resolution,
        symbol_visibility_t symbol_visibility
    );

    ~shared_library_t();

    shared_library_t(const shared_library_t& other) = delete;
    shared_library_t& operator=(const shared_library_t& other) = delete;

    shared_library_t(shared_library_t&& other) noexcept;
    shared_library_t& operator=(shared_library_t&& other) noexcept;

    /**
     * Resolve a symbol by name.
     * - Returns a symbol_t proxy
     * - Throws if symbol is missing or resolution fails
     */
    symbol_t resolve(const char* symbol) const;

private:
    void close_handle();

private:
    shared_library_lifetime_t m_shared_library_lifetime;
    void* m_handle;
};

template <typename F>
symbol_t::operator F() const {
    return reinterpret_cast<F>(m_symbol);
}

#endif // BUILDER_PROJECT_BUILDER_SHARED_LIBRARY_SHARED_LIBRARY_H
