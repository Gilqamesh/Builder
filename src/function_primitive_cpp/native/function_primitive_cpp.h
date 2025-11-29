#ifndef FUNCTION_PRIMITIVE_CPP_H
# define FUNCTION_PRIMITIVE_CPP_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <function.h>

template <typename FunctionSignature = void()>
class function_primitive_cpp_t {
public:
    template <typename F>
    static function_t* function(typesystem_t& typesystem, std::string ns, std::string name, F&& f) {
        using traits = function_traits<F>;
        using signature = typename traits::signature;

        return function_impl<signature>(typesystem, std::move(ns), std::move(name), std::forward<F>(f));
    }

private:
    template <typename Sig, typename F>
    static function_t* function_impl(typesystem_t& typesystem, std::string ns, std::string name, F&& f) {
        using wrapper = primitive_wrapper<Sig>;

        return new function_t(
            typesystem,
            function_ir_t {
                .function_id = function_id_t {
                    .ns = std::move(ns),
                    .name = std::move(name),
                    .creation_time = std::chrono::system_clock::now()
                },
                .left = {},
                .right = {},
                .top = {},
                .bottom = {},
                .children = {},
                .connections = {}
            },
            wrapper::template make(std::forward<F>(f))
        );
    }

    // -----------------------------------------------------------------------------
    // function_traits (minimal)
    // -----------------------------------------------------------------------------
    template <typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using signature = Ret(Args...);
        using pointer = Ret(*)(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    template <typename C, typename Ret, typename... Args>
    struct function_traits<Ret(C::*)(Args...)> {
        using signature = Ret(Args...);
        using pointer = Ret(*)(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    template <typename C, typename Ret, typename... Args>
    struct function_traits<Ret(C::*)(Args...) const> {
        using signature = Ret(Args...);
        using pointer = Ret(*)(Args...);
        static constexpr size_t arity = sizeof...(Args);
    };

    // -----------------------------------------------------------------------------
    // primitive_wrapper — ultra-simple raw memory argument passing
    // -----------------------------------------------------------------------------
    template <typename Signature>
    struct primitive_wrapper;

    template <typename Ret, typename... Args>
    struct primitive_wrapper<Ret(Args...)> {
        using fn_ptr = Ret(*)(Args...);

        template <typename F>
        static function_t::function_call_t make(F&& f) {
            static_assert(std::is_convertible_v<F, fn_ptr>, "F must be convertible to a primitive function pointer");

            static fn_ptr fn = static_cast<fn_ptr>(std::forward<F>(f));
            return &invoker<&fn>::call;
        }

    private:
        template <fn_ptr* Fn>
        struct invoker {
            static void call(function_t& ft, uint8_t) {
                constexpr size_t N = sizeof...(Args);
                constexpr size_t sizes[N] = { sizeof(Args)... };

                void* buffer[N];
                for (size_t i = 0; i < N; i++) {
                    buffer[i] = operator new(sizes[i]);
                }

                // Read into raw memory (your coercion would memcpy here)
                read_args(ft, buffer);

                // Call with dereferenced pointers
                if constexpr (std::is_void_v<Ret>) {
                    invoke_with_args(buffer, std::index_sequence_for<Args...>{});
                } else {
                    Ret r = invoke_with_args_ret(buffer, std::index_sequence_for<Args...>{});
                    throw std::runtime_error("Return value handling not implemented.");
                    ft.write<Ret>(-1, r);
                }
            }

            template <size_t... I>
            static void invoke_with_args(auto buffer, std::index_sequence<I...>) {
                (*Fn)(*reinterpret_cast<Args*>(buffer[I])...);
            }

            template <size_t... I>
            static auto invoke_with_args_ret(auto buffer, std::index_sequence<I...>) {
                return (*Fn)(*reinterpret_cast<Args*>(buffer[I])...);
            }

            static void read_args(function_t& ft, void** buffer) {
                read_loop<0, Args...>(ft, buffer);
            }

            template <size_t Index, typename T, typename... Rest>
            static void read_loop(function_t& ft, void** buffer) {
                *static_cast<T*>(buffer[Index]) = ft.read(Index);

                if constexpr (0 < sizeof...(Rest)) {
                    read_loop<Index + 1, Rest...>(ft, buffer);
                }
            }

            template <size_t Index>
            static void read_loop(function_t&, void**, bool&) {}
        };
    };
};

#endif // FUNCTION_PRIMITIVE_CPP_H
