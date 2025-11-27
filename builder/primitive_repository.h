#ifndef PRIMITIVE_REPOSITORY_H
# define PRIMITIVE_REPOSITORY_H

# include "typesystem_call.h"
# include "function.h"

class primitive_repository_t {
public:
    template <typename Func>
    void save(const function_id_t& id, Func&& f) {
        using traits    = function_traits<std::decay_t<Func>>;
        using signature = typename traits::signature;

        m_primitives[id] = [f = std::forward<Func>(f)](function_t* ft) mutable {
            primitive_wrapper<signature>::call(ft, f);
        };
    }

    function_t::function_call_t load(const function_id_t& id) {
        auto it = m_primitives.find(id);
        if (it == m_primitives.end()) {
            return {};
        }
        return it->second;
    }

private:
    // -----------------------------------------------------------------------------
    // function_traits (minimal)
    // -----------------------------------------------------------------------------
    template <typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using signature = Ret(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    template <typename C, typename Ret, typename... Args>
    struct function_traits<Ret(C::*)(Args...)> {
        using signature = Ret(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    template <typename C, typename Ret, typename... Args>
    struct function_traits<Ret(C::*)(Args...) const> {
        using signature = Ret(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    // -----------------------------------------------------------------------------
    // primitive_wrapper — ultra-simple raw memory argument passing
    // -----------------------------------------------------------------------------
    template <typename Signature>
    struct primitive_wrapper;

    template <typename Ret, typename... Args>
    struct primitive_wrapper<Ret(Args...)> {

        template <typename F>
        static void call(function_t* ft, F&& f) {
            constexpr size_t N = sizeof...(Args);
            constexpr size_t sizes[N] = { sizeof(Args)... };

            void* buffer[N];
            for (size_t i = 0; i < N; i++)
                buffer[i] = operator new(sizes[i]);

            // Read into raw memory (your coercion would memcpy here)
            bool ok = true;
            read_args(ft, buffer, ok);
            if (!ok) {
                std::cout << "Argument read failed.\n";
                return;
            }

            // Call with dereferenced pointers
            if constexpr (std::is_void_v<Ret>) {
                invoke_with_args(f, buffer, std::index_sequence_for<Args...>{});
            } else {
                Ret r = invoke_with_args_ret(f, buffer, std::index_sequence_for<Args...>{});
                ft->write<Ret>(function_t::argument_index_t::RETURN_ARGUMENT, r);
            }
        }

    private:
        template <size_t... I>
        static void invoke_with_args(auto& f, void** buffer, std::index_sequence<I...>) {
            f(*reinterpret_cast<Args*>(buffer[I])...);
        }

        template <size_t... I>
        static auto invoke_with_args_ret(auto& f, void** buffer, std::index_sequence<I...>) {
            return f(*reinterpret_cast<Args*>(buffer[I])...);
        }

        static void read_args(function_t* ft, void** buffer, bool& ok) {
            read_loop<0, Args...>(ft, buffer, ok);
        }

        template <size_t Index, typename T, typename... Rest>
        static void read_loop(function_t* ft, void** buffer, bool& ok) {
            if (!ok) return;
            ft->read(static_cast<typename function_t::argument_index_t>(Index),
                    *reinterpret_cast<T*>(buffer[Index]));
            if constexpr (sizeof...(Rest) > 0)
                read_loop<Index + 1, Rest...>(ft, buffer, ok);
        }

        template <size_t Index>
        static void read_loop(function_t*, void**, bool&) {}
    };

private:
    std::unordered_map<function_id_t, function_t::function_call_t> m_primitives;
};

#endif // PRIMITIVE_REPOSITORY_H
