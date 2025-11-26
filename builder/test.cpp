#include <iostream>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <cstring>
#include <vector>

// -----------------------------------------------------------------------------
// Fake function_t for testing (your real version does TYPESYSTEM.coerce)
// -----------------------------------------------------------------------------
struct function_t {
    enum class argument_index_t { ARG0 = 0, ARG1 = 1 };

    double arg0 = 3.14;
    int    arg1 = 42;

    template <typename T>
    bool read(argument_index_t idx, T& out) {
        if (idx == argument_index_t::ARG0) {
            std::memcpy(&out, &arg0, sizeof(T));
            return true;
        }
        if (idx == argument_index_t::ARG1) {
            std::memcpy(&out, &arg1, sizeof(T));
            return true;
        }
        return false;
    }

    template <typename T>
    void write(argument_index_t, const T& v) {
        std::cout << "[write] result = " << v << "\n";
    }
};

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

        // Allocate raw memory per argument
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
            ft->write<Ret>(function_t::argument_index_t::ARG1, r);
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

// -----------------------------------------------------------------------------
// B — registry
// -----------------------------------------------------------------------------
struct B {
    std::unordered_map<std::string, std::function<void(function_t*)>> primitives;

    template <typename Func>
    void register_primitive(const std::string& name, Func&& f) {
        using traits    = function_traits<std::decay_t<Func>>;
        using signature = typename traits::signature;

        primitives[name] =
            [f = std::forward<Func>(f)](function_t* ft) mutable {
                primitive_wrapper<signature>::call(ft, f);
            };
    }
};

// Member binding helper
template <typename C, typename Ret, typename... Args>
auto bind_member(Ret(C::*mf)(Args...), C* inst) {
    return [inst, mf](Args... a) -> Ret { return (inst->*mf)(a...); };
}

// -----------------------------------------------------------------------------
// A — user code
// -----------------------------------------------------------------------------
struct A {
    void register_primitives(B* b) {
        b->register_primitive("f", bind_member(&A::f, this));
        b->register_primitive("f2", bind_member(&A::f2, this));
    }

    int f(double a, int b) {
        std::cout << "[A::f] a=" << a << ", b=" << b << "\n";
        return (int)(a + b);
    }

    double f2(std::string s, float x, std::function<int(int)> callback) {
        std::cout << "[A::f2] s=" << s << ", x=" << x << "\n";
        int cb_result = callback(10);
        std::cout << "[A::f2] callback(10) = " << cb_result << "\n";
        return x * 2.0;
    }
};

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main() {
    B b;
    A a;
    a.register_primitives(&b);

    function_t ft;
    b.primitives["f"](&ft);
    b.primitives["f2"](&ft);
}
