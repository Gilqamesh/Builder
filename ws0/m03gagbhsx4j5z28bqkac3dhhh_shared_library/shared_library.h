#ifndef M03GAGBHSX4J5Z28BQKAC3DHHH_SHARED_LIBRARY_SHARED_LIBRARY_H
# define M03GAGBHSX4J5Z28BQKAC3DHHH_SHARED_LIBRARY_SHARED_LIBRARY_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <optional>
# include <type_traits>

namespace m03gagbhsx4j5z28bqkac3dhhh_shared_library {

/** @brief Selects whether closing a loader permits the library to unload. */
enum class lifetime_t {
    PROCESS, ///< Keeps the library resident until process exit, even after loader destruction.
    DTOR ///< Releases the loader's reference on destruction; other references may keep the library loaded.
};

/** @brief Selects when the dynamic loader resolves relocations. */
enum class symbol_resolution_t {
    NOW, ///< Resolves bindings while loading and fails if a required symbol is missing.
    LAZY ///< Defers function binding until first use; failures may surface after construction.
};

/** @brief Selects whether loaded symbols are visible to subsequently loaded libraries. */
enum class symbol_visibility_t {
    LOCAL, ///< Keeps symbols out of the global lookup scope.
    GLOBAL ///< Adds symbols to the global lookup scope.
};

/**
 * @brief Borrows a dynamically resolved function address without keeping its library loaded.
 *
 * Convert to the function's exact pointer type and calling convention before
 * calling it; the conversion checks only that the target is a function pointer,
 * not that its signature matches. A null address is representable and must not
 * be called. Both this wrapper and converted pointers require the defining
 * library to remain loaded; see loader_t and lifetime_t.
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
 * @brief Owns a dynamic-loader reference and resolves borrowed function symbols by name.
 *
 * Destruction closes the reference. DTOR permits unloading then; PROCESS keeps
 * the library resident using RTLD_NODELETE. Moving transfers the reference and
 * lifetime policy; move assignment first closes the destination's old reference.
 * For DTOR, keep the owning loader (or its move destination) alive while using
 * symbols; assigning another library can invalidate previously resolved pointers.
 * Do not move, assign, or destroy a loader concurrently with lookups or calls
 * that depend on its reference. Function-call thread safety belongs to the library.
 *
 * The example expects a library exporting `extern "C" void entry()` and,
 * optionally, `extern "C" int version()`.
 * @code{.cpp}
 * #include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * void use_plugin(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
 *     namespace library = m03gagbhsx4j5z28bqkac3dhhh_shared_library;
 *     using entry_fn_t = void (*)();
 *     using version_fn_t = int (*)();
 *     {
 *         library::loader_t loader(
 *             path, library::lifetime_t::DTOR,
 *             library::symbol_resolution_t::NOW, library::symbol_visibility_t::LOCAL
 *         );
 *         entry_fn_t entry = loader.resolve("entry");
 *         entry(); // The loader keeps the library loaded for this call.
 *         if (const auto symbol = loader.resolve_optional("version")) {
 *             version_fn_t version = *symbol;
 *             (void)version();
 *         }
 *     }
 *     entry_fn_t resident_entry;
 *     {
 *         library::loader_t loader(
 *             path, library::lifetime_t::PROCESS,
 *             library::symbol_resolution_t::NOW, library::symbol_visibility_t::LOCAL
 *         );
 *         resident_entry = loader.resolve("entry");
 *     }
 *     resident_entry(); // PROCESS keeps the code resident after loader destruction.
 * }
 * @endcode
 */
class loader_t {
public:
    /**
     * @brief Loads the library at path using the selected lifetime, relocation, and visibility policies.
     *
     * Borrows path only during construction. Throws std::runtime_error when a
     * policy value is unknown or the dynamic loader cannot open the library.
     */
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
     * @brief Resolves a function symbol, throwing on a dynamic-loader lookup error.
     *
     * symbol must point to a null-terminated name, borrowed only during this call.
     * Throws std::invalid_argument for a null name, std::logic_error for a
     * moved-from loader, or std::runtime_error for a loader lookup error.
     * A successful lookup can still contain a null address; see symbol_t.
     */
    symbol_t resolve(const char* symbol) const;

    /**
     * @brief Resolves a function symbol, returning std::nullopt on a dynamic-loader lookup error.
     *
     * Uses the same name and borrowed-symbol lifetime requirements as resolve().
     * A null name still throws std::invalid_argument, and a moved-from loader
     * still throws std::logic_error. An engaged optional means no lookup error
     * was reported; it does not validate the address or function signature.
     */
    std::optional<symbol_t> resolve_optional(const char* symbol) const;

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
