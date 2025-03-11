#include "type.h"
#include "data.h"

#include <iostream>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

#include <x86intrin.h>
int main() {
    size_t time_start = __rdtsc();

    type_t type_number("number");
    type_number.add_abstract<function<string(const data_t&)>>("to_string");

    type_t type_complex("complex");
    auto make_complex_fn = type_complex.add<function<data_t(double a, double b)>>("make-complex", [&type_complex](double a, double b) {
        data_t result(&type_complex);
        result.add_state<double>("a", a);
        result.add_state<double>("b", b);
        return result;
    });
    auto complex_get_real_part_fn = type_complex.add<function<double(const data_t&)>>("get-real", [](const data_t& data) {
        return data.find_state<double>("a");
    });
    auto complex_get_imag_part_fn = type_complex.add<function<double(const data_t&)>>("get-imag", [](const data_t& data) {
        return data.find_state<double>("b");
    });
    auto to_string_complex_fn = type_complex.add<function<string(const data_t&)>>("to_string", [&type_complex](const data_t& data) -> string {
        if (data.type != &type_complex) {
            data_t result(&type_complex);
            if (data.convert(result)) {
                return to_string(result.find_state<double>("a")) + " + " + to_string(result.find_state<double>("b")) + "i";
            } else {
                return "couldnt to_string as complex";
            }
        } else {
            return to_string(data.find_state<double>("a")) + " + " + to_string(data.find_state<double>("b")) + "i";
        }
    });
    type_complex.add_input(&type_number);

    type_t type_real("real");
    auto make_real_fn = type_real.add<function<data_t(double)>>("make-real", [&type_real](double a) {
        data_t result(&type_real);
        result.add_state<double>("a", a);
        return result;
    });
    auto get_real_fn = type_real.add<function<double(const data_t&)>>("get-real", [](const data_t& data) {
        return data.find_state<double>("a");
    });
    auto to_string_real_fn = type_real.add<function<string(const data_t&)>>("to_string", [&type_real](const data_t& data) -> string {
        if (data.type != &type_real) {
            data_t result(&type_real);
            if (data.convert(result)) {
                return to_string(result.find_state<double>("a"));
            } else {
                return "couldnt to_string as real";
            }
        } else {
            return to_string(data.find_state<double>("a"));
        }
    });
    type_real.add_input(&type_complex, [get_real_fn, make_complex_fn](const data_t& data) -> data_t {
        return make_complex_fn(get_real_fn(data), 0.0);
    });

    type_t type_rational("rational");
    auto make_rat_fn = type_rational.add<function<data_t(int, int)>>("make-real", [&type_rational](int n, int d) {
        data_t result(&type_rational);
        result.add_state<int>("n", n);
        result.add_state<int>("d", d);
        return result;
    });
    auto get_denom_fn = type_rational.add<function<int(const data_t&)>>("get-denom", [](const data_t& data) {
        return data.find_state<int>("d");
    });
    auto get_numer_fn = type_rational.add<function<int(const data_t&)>>("get-numer", [](const data_t& data) {
        return data.find_state<int>("n");
    });
    auto to_string_rat_fn = type_rational.add<function<string(const data_t&)>>("to_string", [&type_rational](const data_t& data) -> string {
        if (data.type != &type_rational) {
            data_t result(&type_rational);
            if (data.convert(result)) {
                return to_string(result.find_state<int>("n")) + "/" + to_string(result.find_state<int>("d"));
            } else {
                return "couldnt to_string as rational";
            }
        } else {
            return to_string(data.find_state<int>("n")) + "/" + to_string(data.find_state<int>("d"));
        }
    });
    type_rational.add_input(&type_real, [make_real_fn, get_denom_fn, get_numer_fn](const data_t& data) -> data_t {
        return make_real_fn((double) get_numer_fn(data) / (double) get_denom_fn(data));
    });

    type_t type_int("int");
    auto make_int_fn = type_int.add<function<data_t(int)>>("make-int", [&type_int](int i) {
        data_t result(&type_int);
        result.add_state<int>("i", i);
        return result;
    });
    auto get_int_fn = type_int.add<function<int(const data_t&)>>("get-int", [](const data_t& data) {
        return data.find_state<int>("i");
    });
    auto to_string_int_fn = type_int.add<function<string(const data_t&)>>("to_string", [&type_int](const data_t& data) -> string {
        if (data.type != &type_int) {
            data_t result(&type_int);
            if (data.convert(result)) {
                return to_string(result.find_state<int>("i"));
            } else {
                return "couldnt to_string as int";
            }
        } else {
            return to_string(data.find_state<int>("i"));
        }
    });
    type_int.add_input(&type_rational, [make_rat_fn, get_int_fn](const data_t& data) {
        return make_rat_fn(get_int_fn(data), 1);
    });
    
    type_complex.print();

    cout << "complex as complex: " << to_string_complex_fn(make_complex_fn(42.42, -32.0)) << endl;
    cout << "complex as real: " << to_string_real_fn(make_complex_fn(42.42, -32.0)) << endl;
    type_complex.add_convert_to(&type_real, [&type_real, make_real_fn, complex_get_real_part_fn, complex_get_imag_part_fn](const data_t& from, data_t& result) -> bool {
        if (complex_get_imag_part_fn(from) != 0.0) {
            return false;
        }
        result = make_real_fn(complex_get_real_part_fn(from));
        return true;
    });
    cout << "complex as real: " << to_string_real_fn(make_complex_fn(42.42, -32.0)) << endl;

    cout << "real as real: " << to_string_real_fn(make_real_fn(2.3)) << endl;
    cout << "real as complex: " << to_string_complex_fn(make_real_fn(2.3)) << endl;
    cout << "real as complex: " << to_string_complex_fn(make_real_fn(2.3)) << endl;

    cout << "complex as complex: " << to_string_complex_fn(make_complex_fn(42.42, 0.0)) << endl;
    cout << "complex as real: " << to_string_real_fn(make_complex_fn(42.42, 0.0)) << endl;

    cout << "integer as integer: " << to_string_int_fn(make_int_fn(2)) << endl;
    cout << "integer as complex: " << to_string_complex_fn(make_int_fn(2)) << endl;
    // type_int.add_convert_to(&type_complex, [&type_complex, make_complex_fn, get_int_fn](const data_t& from, data_t& result) -> bool {
    //     result = make_complex_fn(get_int_fn(from), 0.0);
    //     return true;
    // });
    cout << "integer as complex: " << to_string_complex_fn(make_int_fn(2)) << endl;

    cout << "complex as int: " << to_string_int_fn(make_complex_fn(42.42, 0.0)) << endl;
    // type_complex.add_convert_to(&type_int, [&type_int, make_int_fn, complex_get_real_part_fn, complex_get_imag_part_fn](const data_t& from, data_t& result) -> bool {
    //     if (complex_get_imag_part_fn(from) != 0.0) {
    //         return false;
    //     }
    //     result = make_int_fn(complex_get_real_part_fn(from));
    //     return true;
    // });
    cout << "complex as int: " << to_string_int_fn(make_complex_fn(42.42, 0.0)) << endl;

    type_real.add_convert_to(&type_rational, [&type_rational, make_rat_fn, get_real_fn](const data_t& from, data_t& result) -> bool {
        double r = get_real_fn(from);
        if (r != (double) (int) r) {
            return false;
        }
        result = make_rat_fn(r, 1);
        return true;
    });
    cout << "real as rational: " << to_string_rat_fn(make_real_fn(42.42)) << endl;
    cout << "real as rational: " << to_string_rat_fn(make_real_fn(42.0)) << endl;

    cout << "int as rational: " << to_string_rat_fn(make_int_fn(2)) << endl;

    type_complex.print();
    type_int.print();

    size_t time_end = __rdtsc();
    cout << (time_end - time_start) / 1000.0 << "k cycles" << endl;

    return 0;
}
