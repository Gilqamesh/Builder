#ifndef NUMBER_TYPES_H
# define NUMBER_TYPES_H

# include "instance.h"

void init_number_module();
void deinit_number_module();

struct instance_int_t;
struct instance_real_t;
struct instance_rational_t;
struct instance_complex_t;

struct instance_number_t : public instance_t {
    instance_number_t(instance_t&& instance);

    string to_string() const;
};

optional<instance_number_t> operator+(const instance_number_t& i1, const instance_number_t& i2);

struct instance_int_t : public instance_number_t {
    instance_int_t();
    instance_int_t(int integer);
};

struct instance_real_t : public instance_number_t {
    instance_real_t();
    instance_real_t(double real);
};

struct instance_rational_t : public instance_number_t {
    instance_rational_t();
    instance_rational_t(int numerator, int denominator);
};

struct instance_complex_t : public instance_number_t {
    instance_complex_t();
    instance_complex_t(double real_part, double imaginary_part);
};

#endif // NUMBER_TYPES_H
