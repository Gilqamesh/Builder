#include "number_types.h"

#include <iostream>
#include <numeric>

static bool is_initialized;
static type_t* number_type;
static type_t* int_type;
static type_t* real_type;
static type_t* rational_type;
static type_t* complex_type;

void init_number_module() {
    if (!is_initialized) {
        is_initialized = true;
        number_type = new type_t(
            [](state_t& state) {
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        number_type->state.add_abstract<function<string(const instance_t&)>>("to-string");
        number_type->state.add_abstract<function<instance_t(const instance_t&, const instance_t&)>>("add-impl");
        number_type->state.add<function<optional<instance_t>(const instance_t&, const instance_t&)>>("add", [](const instance_t& i1, const instance_t& i2) -> optional<instance_t> {
            if (i1.m_type == i2.m_type) {
                return i1.read<function<instance_t(const instance_t&, const instance_t&)>>("add-impl")(i1, i2);
            }
            if (optional<instance_t> result = i1.convert(i2.m_type)) {
                return result->read<function<instance_t(const instance_t&, const instance_t&)>>("add-impl")(*result, i2);
            }
            if (optional<instance_t> result = i2.convert(i1.m_type)) {
                return result->read<function<instance_t(const instance_t&, const instance_t&)>>("add-impl")(i1, *result);
            }
            return {};
        });

        int_type = new type_t(
            [](state_t& state) {
                state.add<int>("i");
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        int_type->inherit(number_type);
        int_type->state.add<function<instance_t(int)>>("make-int", [](int a) {
            instance_t result(int_type);
            result.write<int>("i") = a;
            return result;
        });

        // try {
        //     instance_t instance = int_type->state.read<function<instance_t(int)>>("make-int")(42);
        // } catch (const std::exception& e) {
        //     cout << e.what() << endl;
        // }
        
        int_type->state.add<function<string(const instance_t&)>>("to-string", [](const instance_t& instance) {
            return to_string(instance.read<int>("i"));
        });
        int_type->state.add<function<instance_t(const instance_t&, const instance_t&)>>("add-impl", [](const instance_t& i1, const instance_t& i2) {
            return instance_int_t(i1.read<int>("i") + i2.read<int>("i"));
        });

        rational_type = new type_t(
            [](state_t& state) {
                state.add<int>("n");
                state.add<int>("d");
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        rational_type->inherit(number_type);
        rational_type->state.add<function<instance_t(int, int)>>("make-rat", [](int n, int d) {
            instance_t result(rational_type);
            int g = gcd(n, d);
            result.write<int>("n") = n / g;
            result.write<int>("d") = d / g;
            return result;
        });
        rational_type->state.add<function<string(const instance_t&)>>("to-string", [](const instance_t& instance) {
            return to_string(instance.read<int>("n")) + "/" + to_string(instance.read<int>("d"));
        });
        rational_type->state.add<function<instance_t(const instance_t&, const instance_t&)>>("add-impl", [](const instance_t& i1, const instance_t& i2) {
            int n1 = i1.read<int>("n");
            int d1 = i1.read<int>("d");
            int n2 = i2.read<int>("n");
            int d2 = i2.read<int>("d");
            return instance_rational_t(n1 * d2 + n2 * d1, d1 * d2);
        });

        real_type = new type_t(
            [](state_t& state) {
                state.add<double>("d");
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        real_type->inherit(number_type);
        real_type->state.add<function<instance_t(double)>>("make-real", [](double d) {
            instance_t result(real_type);
            result.write<double>("d") = d;
            return result;
        });
        real_type->state.add<function<string(const instance_t&)>>("to-string", [](const instance_t& instance) {
            return to_string(instance.read<double>("d"));
        });
        real_type->state.add<function<instance_t(const instance_t&, const instance_t&)>>("add-impl", [](const instance_t& i1, const instance_t& i2) {
            return instance_real_t(i1.read<double>("d") + i2.read<double>("d"));
        });

        complex_type = new type_t(
            [](state_t& state) {
                state.add<double>("a");
                state.add<double>("b");
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        complex_type->inherit(number_type);
        complex_type->state.add<function<instance_t(double a, double b)>>("make-complex", [](double a, double b) {
            instance_t result(complex_type);
            result.write<double>("a") = a;
            result.write<double>("b") = b;
            return result;
        });
        complex_type->state.add<function<string(const instance_t&)>>("to-string", [](const instance_t& instance) {
            return to_string(instance.read<double>("a")) + " + " + to_string(instance.read<double>("b")) + "i";
        });
        complex_type->state.add<function<instance_t(const instance_t&, const instance_t&)>>("add-impl", [](const instance_t& i1, const instance_t& i2) {
            return instance_complex_t(i1.read<double>("a") + i2.read<double>("a"), i1.read<double>("b") + i2.read<double>("b"));
        });

        complex_type->add_coercion(int_type, [](const instance_t& from, instance_t& to) {
            cout << "complex -> int" << endl;
            if (from.read<double>("b") != 0.0) {
                return false;
            }
            to = int_type->state.read<function<instance_t(int)>>("make-int")((int) from.read<double>("a"));
            return true;
        });
        complex_type->add_coercion(rational_type, [](const instance_t& from, instance_t& to) {
            cout << "complex -> rat" << endl;
            double b = from.read<double>("b");
            if (b != 0.0) {
                return false;
            }
            double a = from.read<double>("a");
            if (a != (double) (int) a) {
                return false;
            }
            to = to.read<function<instance_t(int, int)>>("make-rat")(a, 1);
            return true;
        });
        complex_type->add_coercion(real_type, [](const instance_t& from, instance_t& to) {
            cout << "complex -> real" << endl;
            double b = from.read<double>("b");
            if (b != 0.0) {
                return false;
            }
            to = real_type->state.read<function<instance_t(double)>>("make-real")(from.read<double>("a"));
            return true;
        });
        int_type->add_coercion(complex_type, [](const instance_t& from, instance_t& to) {
            cout << "int -> complex" << endl;
            assert(to.m_type == complex_type);
            to = to.read<function<instance_t(double, double)>>("make-complex")((double) from.read<int>("i"), 0.0);
            return true;
        });
        rational_type->add_coercion(real_type, [](const instance_t& from, instance_t& to) {
            cout << "rat -> real" << endl;
            to = to.read<function<instance_t(double)>>("make-real")((double) from.read<int>("n") / (double) from.read<int>("d"));
            return true;
        });
        rational_type->add_coercion(complex_type, [](const instance_t& from, instance_t& to) {
            cout << "rat -> complex" << endl;
            to = to.read<function<instance_t(double, double)>>("make-complex")((double) from.read<int>("n") / (double) from.read<int>("d"), 0.0);
            return true;
        });

    }
}

void deinit_number_module() {
    if (is_initialized) {
        is_initialized = false;
        delete number_type;
        delete int_type;
        delete real_type;
        delete rational_type;
        delete complex_type;
    }
}

instance_number_t::instance_number_t(instance_t&& instance):
    instance_t(std::move(instance))
{
}

string instance_number_t::to_string() const {
    return m_type->read<function<string(const instance_t&)>>("to-string")(*this);
}

optional<instance_number_t> operator+(const instance_number_t& i1, const instance_number_t& i2) {
    return i1.read<function<optional<instance_t>(const instance_t&, const instance_t&)>>("add")(i1, i2);
}

instance_int_t::instance_int_t(): instance_int_t(0)
{
}

instance_int_t::instance_int_t(int integer):
    instance_number_t(int_type->state.read<function<instance_t(int)>>("make-int")(integer))
{
}

instance_real_t::instance_real_t(): instance_real_t(0.0)
{
}

instance_real_t::instance_real_t(double real):
    instance_number_t(real_type->state.read<function<instance_t(double)>>("make-real")(real))
{
}

instance_rational_t::instance_rational_t(): instance_rational_t(1, 0)
{
}

instance_rational_t::instance_rational_t(int numerator, int denominator):
    instance_number_t(rational_type->state.read<function<instance_t(int, int)>>("make-rat")(numerator, denominator))
{
}

instance_complex_t::instance_complex_t(): instance_complex_t(0.0, 0.0)
{
}

instance_complex_t::instance_complex_t(double real_part, double imaginary_part):
    instance_number_t(complex_type->state.read<function<instance_t(double, double)>>("make-complex")(real_part, imaginary_part))
{
}
