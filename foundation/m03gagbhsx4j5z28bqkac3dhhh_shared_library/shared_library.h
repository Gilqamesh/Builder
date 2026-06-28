#ifndef M03GAGBHSX4J5Z28BQKAC3DHHH_SHARED_LIBRARY_SHARED_LIBRARY_H
# define M03GAGBHSX4J5Z28BQKAC3DHHH_SHARED_LIBRARY_SHARED_LIBRARY_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbhsx4j5z28bqkac3dhhh_shared_library {

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
 * Function symbol returned by loader_t::resolve().
 *
 * Cast it to the expected function pointer type before calling it.
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
 * Loads a shared library and resolves function symbols by name.
 */
class loader_t {
public:
    loader_t(
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
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
     * Returns a function symbol by name.
     *
     * Example:
     * using fn_t = void (*)();
     * fn_t fn = loader.resolve("entry");
     */
    symbol_t resolve(const char* symbol) const;

private:
    loader_t();
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

} // namespace m03gagbhsx4j5z28bqkac3dhhh_shared_library

#endif // M03GAGBHSX4J5Z28BQKAC3DHHH_SHARED_LIBRARY_SHARED_LIBRARY_H
