#include "builder.h"

#include "x86intrin.h"

void if_gate(
    wire_t& condition,
    wire_t& consequence_in, wire_t& consequence_out,
    wire_t& alternative_in, wire_t& alternative_out
) {
    condition.add_action([&]() {
        bool result;
        if (!condition.read<bool>(result)) {
            return ;
        }

        if (result) {
            consequence_out.write(consequence_in);
        } else {
            alternative_out.write(alternative_in);
        }
    });
}

void mul(wire_t& a, wire_t& b, wire_t& result) {
    auto f = [&]() {
        int ra;
        if (!a.read<int>(ra)) {
            return ;
        }

        int rb;
        if (!b.read<int>(rb)) {
            return ;
        }
        result.write<int>(ra * rb);
    };
    a.add_action(f);
    b.add_action(f);
}

void is_zero(wire_t& in, wire_t& out) {
    in.add_action([&]() {
        int result;
        if (!in.read<int>(result)) {
            return ;
        }

        out.write<bool>(result == 0);
    });
}

void one(wire_t& out) {
    out.write<int>(1);
}

void sub(wire_t& a, wire_t& b, wire_t& result) {
    auto f = [&]() {
        int ra;
        if (!a.read<int>(ra)) {
            return ;
        }

        int rb;
        if (!b.read<int>(rb)) {
            return ;
        }
        result.write<int>(ra - rb);
    };
    a.add_action(f);
    b.add_action(f);
}

int wires_top = 0;
wire_t wires[1024 * 128];

void fact(wire_t& in, wire_t& out) {
    wires_top += 6;
    // wire_t a; -6
    // wire_t b; -5
    // wire_t c; -4
    // wire_t d; -3
    // wire_t e; -2
    // wire_t f; -1
    is_zero(in, wires[wires_top - 6]);
    if_gate(wires[wires_top - 6], wires[wires_top - 5], out, in, wires[wires_top - 4]);
    mul(wires[wires_top - 4], wires[wires_top - 3], out);
    one(wires[wires_top - 1]);
    sub(wires[wires_top - 4], wires[wires_top - 1], wires[wires_top - 2]);
    one(wires[wires_top - 5]);
    wires[wires_top - 2].add_action([&, current_top = wires_top]() {
        int result;
        if (!wires[current_top - 2].read<int>(result)) {
            return ;
        }

        wire_t tmp_in;
        fact(tmp_in, wires[current_top - 3]);
        tmp_in.write(result);
    });
}

struct if_t {
    if_t(
        wire_t* condition,
        wire_t* consequence_in, wire_t* consequence_out,
        wire_t* alternative_in, wire_t* alternative_out
    ) {
        condition->add_action([=]() {
            bool result;
            if (!condition->read<bool>(result)) {
                return ;
            }

            if (result) {
                consequence_out->write(consequence_in);
            } else {
                alternative_out->write(alternative_in);
            }
        });
    }
};

struct one_t {
    one_t(wire_t* out) {
        out->write<int>(1);
    }
};

struct is_zero_t {
    is_zero_t(wire_t* in, wire_t* out) {
        in->add_action([=]() {
            int result;
            if (!in->read<int>(result)) {
                return ;
            }

            out->write<bool>(result == 0);
        });
    }
};

struct mul_t {
    mul_t(wire_t* a, wire_t* b, wire_t* result) {
        auto f = [=]() {
            int ra;
            if (!a->read<int>(ra)) {
                return ;
            }

            int rb;
            if (!b->read<int>(rb)) {
                return ;
            }
            result->write<int>(ra * rb);
        };
        a->add_action(f);
        b->add_action(f);
    }
};

struct sub_t {
    sub_t(wire_t* a, wire_t* b, wire_t* result) {
        auto f = [=]() {
            int ra;
            if (!a->read<int>(ra)) {
                return ;
            }

            int rb;
            if (!b->read<int>(rb)) {
                return ;
            }
            result->write<int>(ra - rb);
        };
        a->add_action(f);
        b->add_action(f);
    }
};

struct if_component_t: public component_t {
private:
    // in: [condition <bool>, consequence_in, alternative_in]
    // out: [consequence_out, alternative_out]
    void call_impl() override {
        bool condition;
        if (!read<bool>(0, condition)) {
            return ;
        }

        if (condition) {
            write(1, 0);
        } else {
            write(2, 1);
        }
    }
};

struct mul_component_t : public component_t {
private:
    // in: [a <int>, b <int>]
    // out: [result <int>]
    void call_impl() override {
        int a;
        if (!read<int>(0, a)) {
            return ;
        }

        int b;
        if (!read<int>(1, b)) {
            return ;
        }

        write<int>(0, a * b);
    }
};

struct is_zero_component_t : public component_t {
    // in: [in <int>]
    // out: [out <bool>]
    void call_impl() override {
        int in;
        if (!read<int>(0, in)) {
            return ;
        }

        write<bool>(0, in == 0);
    }
};

struct one_component_t : public component_t {
    // out: [out <int>]
    void call_impl() override {
        write<int>(0, 1);
    }
};

struct sub_component_t : public component_t {
    // in: [a <int>, b <int>]
    // out: [result <int>]
    void call_impl() override {
        int a;
        if (!read<int>(0, a)) {
            return ;
        }

        int b;
        if (!read<int>(1, b)) {
            return ;
        }

        write<int>(0, a - b);
    }
};

struct fact_component_t : public component_t {
    fact_component_t() {
    }

    // in: [in <int>]
    // out: [out <int>]
    void call_impl() override {
    }
};

struct fact_t {
    fact_t(wire_t* in, wire_t* out) {
        m_a = new wire_t();
        m_b = new wire_t();
        m_c = new wire_t();
        m_d = new wire_t();
        m_e = new wire_t();
        m_f = new wire_t();
        m_if = new if_t(m_a, m_b, out, in, m_c);
        m_is_zero = new is_zero_t(in, m_a);
        m_one_1 = new one_t(m_b);
        m_one_2 = new one_t(m_f);
        m_mul = new mul_t(m_c, m_d, out);
        m_sub = new sub_t(m_c, m_f, m_e);
        m_e->add_action([this] {
            int result;
            if (!m_e->read<int>(result)) {
                return ;
            }

            wire_t* m_temp_in = new wire_t;
            wire_t* m_temp_out = new wire_t;
            fact_t* subfact = new fact_t(m_temp_in, m_temp_out);
            m_temp_in->write<int>(result);
            m_d->write(m_temp_out);
        });
    }

    wire_t* m_a = nullptr;
    wire_t* m_b = nullptr;
    wire_t* m_c = nullptr;
    wire_t* m_d = nullptr;
    wire_t* m_e = nullptr;
    wire_t* m_f = nullptr;

    if_t* m_if = nullptr;
    is_zero_t* m_is_zero = nullptr;
    one_t* m_one_1 = nullptr;
    one_t* m_one_2 = nullptr;
    mul_t* m_mul = nullptr;
    sub_t* m_sub = nullptr;
};

int fact_native(int a) {
    if (a == 0) {
        return 1;
    }
    return a * fact_native(a - 1);
}

int main() {
    wire_t in;
    wire_t out;
    fact(in, out);

    int result;

    for (int i = 0; i < 1000; ++i) {
        size_t t_start = __rdtsc();
        in.write<int>(10);
        size_t t_end = __rdtsc();
        if (out.read<int>(result)) {
            std::cout << "result: " << result << std::endl;
        }
        std::cout << "i: " << i << ", " << (t_end - t_start) / 1000.0 << "kCy" << std::endl;
    }

    size_t t_start = __rdtsc();
    int res = fact_native(10);
    size_t t_end = __rdtsc();
    std::cout << "res: " << res << ", fact_native: " << (t_end - t_start) / 1000.0 << "kCy" << std::endl;

    in.write<int>(5);
    if (out.read<int>(result)) {
        std::cout << "result: " << result << std::endl;
    }

    in.write<int>(10);
    if (out.read<int>(result)) {
        std::cout << "result: " << result << std::endl;
    }

    t_start = __rdtsc();
    in.write<int>(20);
    if (out.read<int>(result)) {
        std::cout << "result: " << result << std::endl;
    }
    t_end = __rdtsc();
    std::cout << (t_end - t_start) / 1000.0 << "kCy" << std::endl;

    return 0;
}
