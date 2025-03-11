#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <cassert>
#include <functional>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <numeric>

using namespace std;

struct signature_base_t {
    unordered_map<
        string,
        shared_ptr<void>
    > m_symbol_str_to_fn_pointer;
};

template <typename signature_t>
struct signature_derived_t : public signature_base_t {
    signature_t add(const string& symbol_name, const signature_t& fn) {
        auto fn_pointer_it = m_symbol_str_to_fn_pointer.find(symbol_name);
        if (fn_pointer_it != m_symbol_str_to_fn_pointer.end()) {
            return 0;
        }
        fn_pointer_it = m_symbol_str_to_fn_pointer.insert({ symbol_name, make_shared<signature_t>(fn) }).first;
        return *static_pointer_cast<signature_t>(fn_pointer_it->second);
    }

    signature_t find(const string& symbol_name) {
        auto fn_pointer_it = m_symbol_str_to_fn_pointer.find(symbol_name);
        if (fn_pointer_it == m_symbol_str_to_fn_pointer.end()) {
            return 0;
        }
        return *static_pointer_cast<signature_t>(fn_pointer_it->second);
    }
};

struct signature_container_t {
    unordered_map<const char*, signature_base_t*> m_signature_str_to_signature_base;

    template <typename signature_t>
    signature_t add(const string& symbol_name, const signature_t& fn) {
        const char* signature_str = typeid(signature_t).name();
        auto signature_base_it = m_signature_str_to_signature_base.find(signature_str);
        if (signature_base_it == m_signature_str_to_signature_base.end()) {
            signature_base_it = m_signature_str_to_signature_base.insert({ signature_str, new signature_derived_t<signature_t> }).first;
        }
        return ((signature_derived_t<signature_t>*)(signature_base_it->second))->add(symbol_name, fn);
    }

    template <typename signature_t>
    signature_t find(const string& symbol_name) {
        const char* signature_str = typeid(signature_t).name();
        auto signature_base_it = m_signature_str_to_signature_base.find(signature_str);
        if (signature_base_it == m_signature_str_to_signature_base.end()) {
            return 0;
        }
        return ((signature_derived_t<signature_t>*)(signature_base_it->second))->find(symbol_name);
    }

    void print(int depth = 0) {
        for (const auto& p1 : m_signature_str_to_signature_base) {
            cout << string(depth * 2, ' ') << p1.first << ":" << endl;
            for (const auto& p2 : p1.second->m_symbol_str_to_fn_pointer) {
                cout << string(depth * 2, ' ') << "  " << p2.first << " -> " << p2.second << endl;
            }
        }
    }
};

struct interface_t {
    // unordered_set<interface_t*> m_derived;
    signature_container_t m_signature_containers;

    // void derive_from(interface_t* interface) {
    //     assert(interface != this);
    //     m_derived.insert(interface);
    // }

    template <typename signature_t>
    signature_t add(const string& symbol_name, const signature_t& fn) {
        return m_signature_containers.add<signature_t>(symbol_name, fn);
    }

    template <typename signature_t>
    signature_t find(const string& symbol_name) {
        return m_signature_containers.find<signature_t>(symbol_name);
        // signature_t result = m_signature_containers.find<signature_t>(symbol_name);
        // if (result) {
        //     return result;
        // }
        // for (interface_t* interface : m_derived) {
        //     result = interface->find<signature_t>(symbol_name);
        //     if (result) {
        //         return result;
        //     }
        // }
        // return 0;
    }

    void print(int depth = 0) {
        cout << string(depth * 2, ' ') << "Interface:" << endl;
        m_signature_containers.print(depth + 1);
        // for (interface_t* interface : m_derived) {
        //     interface->print(depth + 1);
        // }
    }
};

struct state_base_t {
    struct entry_t {
        void* data;
        function<void(void*)> dtor;
    };
    unordered_map<string, entry_t> m;

    ~state_base_t() {
        for (auto& p : m) {
            p.second.dtor(p.second.data);
        }
    }

    template <typename T, typename... Args>
    T* add(const string& field_name, Args&&... args) {
        auto it = m.find(field_name);
        if (it != m.end()) {
            throw runtime_error("field already exists");
        }
        it = m.insert({ field_name, { new T(forward<Args>(args)...) }, [](void* data) { delete static_cast<T*>(data); } }).first;
        return (T*) it->second;
    }

    template <typename T>
    T* find(const string& field_name) {
        auto it = m.find(field_name);
        if (it == m.end()) {
            throw runtime_error("field does not exist");
        }
        return (T*) it->second;
    }
};

struct program_t {
    interface_t m_interface;
    vector<program_t*> m_inputs;
    vector<program_t*> m_outputs;
    function<bool(program_t&)> m_run_fn;
    function<string(program_t&)> m_display_fn;
    state_base_t m_state;

    program_t(
        const function<void(program_t&)>& init_fn,
        const function<bool(program_t&)>& run_fn,
        const function<string(program_t&)>& display_fn
    ):
        m_run_fn(run_fn),
        m_display_fn(display_fn)
    {
        init_fn(*this);
    }

    void add_input(program_t* program) {
        m_inputs.push_back(program);
        program->m_outputs.push_back(this);
    }

    bool run() {
        if (m_run_fn(*this)) {
            for (program_t* output : m_outputs) {
                output->run();
            }
            return true;
        }
        return false;
    }

    string display() {
        return m_display_fn(*this);
    }
};

struct program_database_t {
    unordered_map<string, program_t*> program_database;

    template <typename program_type_t>
    program_t* get(const string& name) {
        auto it = program_database.find(name);
        if (it != program_database.end()) {
            return it->second;
        }
        it = program_database.insert({ name, new program_type_t }).first;
        return it->second;
    }
} program_database;

struct rectangular_package_t : public program_t {
    struct rect_t : public state_base_t { };
    rectangular_package_t():
        program_t(
            [](program_t& self) {
                self.m_state.add<int>("major", 1);
                self.m_state.add<int>("minor", 0);

                self.m_state.add<function<rect_t(double, double)>>("make-from-real-imag", [](double a, double b) {
                    rect_t result;
                    result.add<double>("a", a);
                    result.add<double>("b", b);
                    return result;
                });

                self.m_interface.add<function<rect_t(double, double)>>("make-from-real-imag", [](double a, double b) {
                    rect_t result;
                    result.add<double>("a", a);
                    result.add<double>("b", b);
                    return result;
                });
                self.m_interface.add<function<rect_t(double, double)>>("make-from-mag-ang", [](double r, double a) {
                    rect_t result;
                    result.add<double>("a", r * cos(a));
                    result.add<double>("b", r * sin(a));
                    return result;
                });
                auto real_part_fn = self.m_interface.add<function<double(rect_t)>>("real-part", [](rect_t rect) {
                    double* a = rect.find<double>("a");
                    assert(a);
                    return *a;
                });
                auto imag_part_fn = self.m_interface.add<function<double(rect_t)>>("imag-part", [](rect_t rect) {
                    double* b = rect.find<double>("b");
                    assert(b);
                    return *b;
                });
                self.m_interface.add<function<double(rect_t)>>("magnitude", [real_part_fn, imag_part_fn](rect_t rect) {
                    double a = real_part_fn(rect);
                    double b = imag_part_fn(rect);
                    return sqrt(a * a + b * b);
                });
                self.m_interface.add<function<double(rect_t)>>("angle", [imag_part_fn, real_part_fn](rect_t rect) {
                    double a = real_part_fn(rect);
                    double b = imag_part_fn(rect);
                    return (double) atan2(b, a);
                });
                self.m_interface.add<function<string(rect_t)>>("display-rect", [real_part_fn, imag_part_fn](rect_t r) {
                    return to_string(real_part_fn(r)) + to_string(imag_part_fn(r)) + "i";
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                return "rectangular package v" + to_string(*self.m_state.find<int>("major")) + "." + to_string(*self.m_state.find<int>("minor"));
            }
        )
    {
    }
};

struct polar_package_t : public program_t {
    struct state_t {
        int major;
        int minor;
    };
    struct polar_t {
        double r;
        double a;
    };
    polar_package_t():
        program_t(
            [](program_t& self) {
                self.m_interface.add<function<polar_t(double, double)>>("make-from-mag-ang", [](double r, double a) {
                    return polar_t{ r, a };
                });
                auto magnitude_fn = self.m_interface.add<function<double(polar_t)>>("magnitude", [](polar_t polar) {
                    return polar.r;
                });
                auto angle_fn = self.m_interface.add<function<double(polar_t)>>("angle", [](polar_t polar) {
                    return polar.a;
                });
                self.m_interface.add<function<double(polar_t)>>("real-part", [magnitude_fn, angle_fn](polar_t polar) {
                    return magnitude_fn(polar) * cos(angle_fn(polar));
                });
                self.m_interface.add<function<double(polar_t)>>("imag-part", [magnitude_fn, angle_fn](polar_t polar) {
                    return magnitude_fn(polar) * sin(angle_fn(polar));
                });
                self.m_interface.add<function<polar_t(double, double)>>("make-from-real-imag", [](double a, double b) {
                    return polar_t{ sqrt(a * a + b * b), atan2(b, a) };
                });
                self.m_interface.add<function<string(polar_t)>>("display-polar", [magnitude_fn, angle_fn](polar_t p) {
                    return "r: " + to_string(magnitude_fn(p)) + "phi: " + to_string(angle_fn(p));
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                return "polar package";
            }
        )
    {
    }
};

struct complex_package_t : public program_t {
    struct state_t {
        int major;
        int minor;
    };
    struct complex_t {
        union {
            polar_package_t::polar_t polar;
            rectangular_package_t::rect_t rect;
        } v;
        enum {
            POLAR,
            RECT
        } t;
    };
    complex_package_t():
        program_t(
            [](program_t& self) {
                program_t* rectanglar_package = program_database.get<rectangular_package_t>("rectangular package");
                program_t* polar_package = program_database.get<polar_package_t>("polar package");
                self.add_input(rectanglar_package);
                self.add_input(polar_package);

                auto make_from_mag_ang_polar_fn = polar_package->m_interface.find<function<polar_package_t::polar_t(double, double)>>("make-from-mag-ang");
                assert(make_from_mag_ang_polar_fn);
                auto make_from_mag_ang_fn = self.m_interface.add<function<complex_t(double, double)>>("make-from-mag-ang", [make_from_mag_ang_polar_fn](double r, double a) {
                    return complex_t{ .v.polar = make_from_mag_ang_polar_fn(r, a), .t = complex_t::POLAR };
                });

                auto make_from_real_imag_rect_fn = rectanglar_package->m_interface.find<function<rectangular_package_t::rect_t(double, double)>>("make-from-real-imag");
                assert(make_from_real_imag_rect_fn);
                auto make_from_real_imag_fn = self.m_interface.add<function<complex_t(double, double)>>("make-from-real-imag", [make_from_real_imag_rect_fn](double a, double b) {
                    return complex_t{ .v.rect = make_from_real_imag_rect_fn(a, b), .t = complex_t::RECT };
                });

                auto magnitude_rect_fn = rectanglar_package->m_interface.find<function<double(rectangular_package_t::rect_t)>>("magnitude");
                auto magnitude_polar_fn = polar_package->m_interface.find<function<double(polar_package_t::polar_t)>>("magnitude");
                assert(magnitude_rect_fn);
                assert(magnitude_polar_fn);
                auto magnitude_fn = self.m_interface.add<function<double(complex_t)>>("magnitude", [magnitude_rect_fn, magnitude_polar_fn](complex_t polar) {
                    switch (polar.t) {
                    case complex_t::POLAR: {
                        return magnitude_polar_fn(polar.v.polar);
                    } break ;
                    case complex_t::RECT: {
                        return magnitude_rect_fn(polar.v.rect);
                    } break ;
                    default: assert(0);
                    }
                    return double{};
                });

                auto angle_rect_fn = rectanglar_package->m_interface.find<function<double(rectangular_package_t::rect_t)>>("angle");
                auto angle_polar_fn = polar_package->m_interface.find<function<double(polar_package_t::polar_t)>>("angle");
                assert(angle_rect_fn);
                assert(angle_polar_fn);
                auto angle_fn = self.m_interface.add<function<double(complex_t)>>("angle", [angle_rect_fn, angle_polar_fn](complex_t polar) {
                    switch (polar.t) {
                    case complex_t::POLAR: {
                        return angle_polar_fn(polar.v.polar);
                    } break ;
                    case complex_t::RECT: {
                        return angle_rect_fn(polar.v.rect);
                    } break ;
                    default: assert(0);
                    }
                    return double{};
                });

                auto real_part_rect_fn = rectanglar_package->m_interface.find<function<double(rectangular_package_t::rect_t)>>("real-part");
                auto real_part_polar_fn = polar_package->m_interface.find<function<double(polar_package_t::polar_t)>>("real-part");
                assert(real_part_rect_fn);
                assert(real_part_polar_fn);
                auto real_part_fn = self.m_interface.add<function<double(complex_t)>>("real-part", [real_part_rect_fn, real_part_polar_fn](complex_t polar) {
                    switch (polar.t) {
                    case complex_t::POLAR: {
                        return real_part_polar_fn(polar.v.polar);
                    } break ;
                    case complex_t::RECT: {
                        return real_part_rect_fn(polar.v.rect);
                    } break ;
                    default: assert(0);
                    }
                    return double{};
                });

                auto imag_part_rect_fn = rectanglar_package->m_interface.find<function<double(rectangular_package_t::rect_t)>>("imag-part");
                auto imag_part_polar_fn = polar_package->m_interface.find<function<double(polar_package_t::polar_t)>>("imag-part");
                assert(imag_part_rect_fn);
                assert(imag_part_polar_fn);
                auto imag_part_fn = self.m_interface.add<function<double(complex_t)>>("imag-part", [imag_part_rect_fn, imag_part_polar_fn](complex_t c) {
                    switch (c.t) {
                    case complex_t::POLAR: {
                        return imag_part_polar_fn(c.v.polar);
                    } break ;
                    case complex_t::RECT: {
                        return imag_part_rect_fn(c.v.rect);
                    } break ;
                    default: assert(0);
                    }
                    return double{};
                });


                self.m_interface.add<function<complex_t(complex_t, complex_t)>>("add-complex", [make_from_real_imag_fn, real_part_fn, imag_part_fn](complex_t z1, complex_t z2) {
                    return make_from_real_imag_fn(
                        real_part_fn(z1) + real_part_fn(z2),
                        imag_part_fn(z1) + imag_part_fn(z2)
                    );
                });
                self.m_interface.add<function<complex_t(complex_t, complex_t)>>("sub-complex", [make_from_real_imag_fn, real_part_fn, imag_part_fn](complex_t z1, complex_t z2) {
                    return make_from_real_imag_fn(
                        real_part_fn(z1) - real_part_fn(z2),
                        imag_part_fn(z1) - imag_part_fn(z2)
                    );
                });
                self.m_interface.add<function<complex_t(complex_t, complex_t)>>("mul-complex", [make_from_mag_ang_fn, magnitude_fn, angle_fn](complex_t z1, complex_t z2) {
                    return make_from_mag_ang_fn(
                        magnitude_fn(z1) * magnitude_fn(z2),
                        angle_fn(z1) + angle_fn(z2)
                    );
                });
                self.m_interface.add<function<complex_t(complex_t, complex_t)>>("div-complex", [make_from_mag_ang_fn, magnitude_fn, angle_fn](complex_t z1, complex_t z2) {
                    return make_from_mag_ang_fn(
                        magnitude_fn(z1) / magnitude_fn(z2),
                        angle_fn(z1) - angle_fn(z2)
                    );
                });

                auto display_polar_fn = polar_package->m_interface.find<function<string(polar_package_t::polar_t)>>("display-polar");
                auto display_rect_fn = rectanglar_package->m_interface.find<function<string(rectangular_package_t::rect_t)>>("display-rect");
                assert(display_polar_fn);
                assert(display_rect_fn);
                self.m_interface.add<function<string(complex_t)>>("display-complex", [display_polar_fn, display_rect_fn](complex_t c) {
                    switch (c.t) {
                    case complex_t::POLAR: {
                        return display_polar_fn(c.v.polar);
                    } break ;
                    case complex_t::RECT: {
                        return display_rect_fn(c.v.rect);
                    } break ;
                    default: throw runtime_error("operation is not implemented for type");
                    }
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                state_t* state = (state_t*) self.m_state;
                return "complex package v" + to_string(state->major) + "." + to_string(state->minor);
            }
        )
    {
    }
};

struct double_package_t : public program_t {
    struct state_t {
        int major;
        int minor;
    };
    struct double_t {
        double a;
    };
    double_package_t():
        program_t(
            [](program_t& self) {
                auto make_double_fn = self.m_interface.add<function<double_t(double)>>("make-double", [](double a) {
                    return double_t{ a };
                });
                auto get_double_fn = self.m_interface.add<function<double(double_t)>>("get-double", [](double_t d) {
                    return d.a;
                });
                self.m_interface.add<function<double_t(double_t, double_t)>>("add-double", [make_double_fn, get_double_fn](double_t a, double_t b) {
                    return make_double_fn(get_double_fn(a) + get_double_fn(b));
                });
                self.m_interface.add<function<double_t(double_t, double_t)>>("sub-double", [make_double_fn, get_double_fn](double_t a, double_t b) {
                    return make_double_fn(get_double_fn(a) - get_double_fn(b));
                });
                self.m_interface.add<function<double_t(double_t, double_t)>>("mul-double", [make_double_fn, get_double_fn](double_t a, double_t b) {
                    return make_double_fn(get_double_fn(a) * get_double_fn(b));
                });
                self.m_interface.add<function<double_t(double_t, double_t)>>("div-double", [make_double_fn, get_double_fn](double_t a, double_t b) {
                    return make_double_fn(get_double_fn(a) / get_double_fn(b));
                });
                self.m_interface.add<function<string(double_t)>>("display-double", [get_double_fn](double_t d) {
                    return to_string(get_double_fn(d));
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                return "double package"
            }
        )
    {
    }
};

struct rational_package_t : public program_t {
    struct state_t {
        int major;
        int minor;
    };
    struct rat_t {
        int n;
        int d;
    };
    rational_package_t():
        program_t(
            [](program_t& self) {
                auto make_rat_fn = self.m_interface.add<function<rat_t(int, int)>>("make-rat", [](int n, int d) {
                    int g = gcd(n, d);
                    return rat_t{
                        n / g, d / g
                    };
                });
                auto numer_fn = self.m_interface.add<function<int(rat_t)>>("numer", [](rat_t r) {
                    return r.n;
                });
                auto denom_fn = self.m_interface.add<function<int(rat_t)>>("denom", [](rat_t r) {
                    return r.d;
                });
                self.m_interface.add<function<rat_t(rat_t, rat_t)>>("add-rat", [make_rat_fn, numer_fn, denom_fn](rat_t a, rat_t b) {
                    return make_rat_fn(
                        numer_fn(a) * denom_fn(b) + numer_fn(b) * denom_fn(a),
                        denom_fn(a) * denom_fn(b)
                    );
                });
                self.m_interface.add<function<rat_t(rat_t, rat_t)>>("sub-rat", [make_rat_fn, numer_fn, denom_fn](rat_t a, rat_t b) {
                    return make_rat_fn(
                        numer_fn(a) * denom_fn(b) - numer_fn(b) * denom_fn(a),
                        denom_fn(a) * denom_fn(b)
                    );
                });
                self.m_interface.add<function<rat_t(rat_t, rat_t)>>("mul-rat", [make_rat_fn, numer_fn, denom_fn](rat_t a, rat_t b) {
                    return make_rat_fn(
                        numer_fn(a) * numer_fn(b),
                        denom_fn(a) * denom_fn(b)
                    );
                });
                self.m_interface.add<function<rat_t(rat_t, rat_t)>>("div-rat", [make_rat_fn, numer_fn, denom_fn](rat_t a, rat_t b) {
                    return make_rat_fn(
                        numer_fn(a) * denom_fn(b),
                        denom_fn(a) * numer_fn(b)
                    );
                });

                self.m_interface.add<function<string(rat_t)>>("display-rat", [](rat_t r) {
                    return to_string(r.n) + "/" + to_string(r.d);
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                return "double package";
            }
        )
    {
    }
};

struct arithmetic_package_t : public program_t {
    struct state_t {
        int major;
        int minor;
    };
    struct arithmetic_t {
        union {
            rational_package_t::rat_t r;
            complex_package_t::complex_t c;
            double_package_t::double_t d;
        } v;
        enum {
            RAT,
            COMPLEX,
            DOUBLE
        } t;
    };
    arithmetic_package_t():
        program_t(
            [](program_t& self) {
                auto make_from_complex_fn = self.m_interface.add<function<arithmetic_t(complex_package_t::complex_t)>>("make-from-complex", [](complex_package_t::complex_t c) {
                    return arithmetic_t{ .v.c = c, .t = arithmetic_t::COMPLEX };
                });
                auto make_from_double_fn = self.m_interface.add<function<arithmetic_t(double_package_t::double_t)>>("make-from-double", [](double_package_t::double_t d) {
                    return arithmetic_t{ .v.d = d, .t = arithmetic_t::DOUBLE };
                });
                auto make_from_rat_fn = self.m_interface.add<function<arithmetic_t(rational_package_t::rat_t)>>("make-from-rat", [](rational_package_t::rat_t r) {
                    return arithmetic_t{ .v.r = r, .t = arithmetic_t::RAT };
                });

                program_t* complex_package = program_database.get<complex_package_t>("complex package");
                program_t* double_package = program_database.get<double_package_t>("double package");
                program_t* rational_package = program_database.get<rational_package_t>("rational package");
                self.add_input(complex_package);
                self.add_input(double_package);
                self.add_input(rational_package);

                auto make_double_fn = double_package->m_interface.find<function<double_package_t::double_t(double)>>("make-double");
                assert(make_double_fn);
                self.m_interface.add<function<arithmetic_t()>>("make-zero", [make_from_double_fn, make_double_fn]() {
                    return make_from_double_fn(make_double_fn(0.0));
                });

                auto add_complex_fn = complex_package->m_interface.find<function<complex_package_t::complex_t(complex_package_t::complex_t, complex_package_t::complex_t)>>("add-complex");
                auto add_double_fn = double_package->m_interface.find<function<double_package_t::double_t(double_package_t::double_t, double_package_t::double_t)>>("add-double");
                auto add_rational_fn = rational_package->m_interface.find<function<rational_package_t::rat_t(rational_package_t::rat_t, rational_package_t::rat_t)>>("add-rat");
                assert(add_complex_fn);
                assert(add_double_fn);
                assert(add_rational_fn);
                auto add_fn = self.m_interface.add<function<arithmetic_t(arithmetic_t, arithmetic_t)>>("add-arithmetic", [add_complex_fn, add_double_fn, add_rational_fn, make_from_complex_fn, make_from_double_fn, make_from_rat_fn](arithmetic_t a, arithmetic_t b) {
                    if (a.t != b.t) {
                        throw runtime_error("the two arithmetic types differ");
                    }
                    switch (a.t) {
                    case arithmetic_t::COMPLEX: {
                        return make_from_complex_fn(add_complex_fn(a.v.c, b.v.c));
                    } break ;
                    case arithmetic_t::RAT: {
                        return make_from_rat_fn(add_rational_fn(a.v.r, b.v.r));
                    } break ;
                    case arithmetic_t::DOUBLE: {
                        return make_from_double_fn(add_double_fn(a.v.d, b.v.d));
                    } break ;
                    default: throw runtime_error("operation is not supported for this type");
                    }
                });

                auto sub_complex_fn = complex_package->m_interface.find<function<complex_package_t::complex_t(complex_package_t::complex_t, complex_package_t::complex_t)>>("sub-complex");
                auto sub_double_fn = double_package->m_interface.find<function<double_package_t::double_t(double_package_t::double_t, double_package_t::double_t)>>("sub-double");
                auto sub_rational_fn = rational_package->m_interface.find<function<rational_package_t::rat_t(rational_package_t::rat_t, rational_package_t::rat_t)>>("sub-rat");
                assert(sub_complex_fn);
                assert(sub_double_fn);
                assert(sub_rational_fn);
                auto sub_fn = self.m_interface.add<function<arithmetic_t(arithmetic_t, arithmetic_t)>>("sub-arithmetic", [sub_complex_fn, sub_double_fn, sub_rational_fn, make_from_complex_fn, make_from_double_fn, make_from_rat_fn](arithmetic_t a, arithmetic_t b) {
                    if (a.t != b.t) {
                        throw runtime_error("the two arithmetic types differ");
                    }
                    switch (a.t) {
                    case arithmetic_t::COMPLEX: {
                        return make_from_complex_fn(sub_complex_fn(a.v.c, b.v.c));
                    } break ;
                    case arithmetic_t::RAT: {
                        return make_from_rat_fn(sub_rational_fn(a.v.r, b.v.r));
                    } break ;
                    case arithmetic_t::DOUBLE: {
                        return make_from_double_fn(sub_double_fn(a.v.d, b.v.d));
                    } break ;
                    default: throw runtime_error("operation is not supported for this type");
                    }
                });

                auto mul_complex_fn = complex_package->m_interface.find<function<complex_package_t::complex_t(complex_package_t::complex_t, complex_package_t::complex_t)>>("mul-complex");
                auto mul_double_fn = double_package->m_interface.find<function<double_package_t::double_t(double_package_t::double_t, double_package_t::double_t)>>("mul-double");
                auto mul_rational_fn = rational_package->m_interface.find<function<rational_package_t::rat_t(rational_package_t::rat_t, rational_package_t::rat_t)>>("mul-rat");
                assert(mul_complex_fn);
                assert(mul_double_fn);
                assert(mul_rational_fn);
                auto mul_fn = self.m_interface.add<function<arithmetic_t(arithmetic_t, arithmetic_t)>>("mul-arithmetic", [mul_complex_fn, mul_double_fn, mul_rational_fn, make_from_complex_fn, make_from_double_fn, make_from_rat_fn](arithmetic_t a, arithmetic_t b) {
                    if (a.t != b.t) {
                        throw runtime_error("the two arithmetic types differ");
                    }
                    switch (a.t) {
                    case arithmetic_t::COMPLEX: {
                        return make_from_complex_fn(mul_complex_fn(a.v.c, b.v.c));
                    } break ;
                    case arithmetic_t::RAT: {
                        return make_from_rat_fn(mul_rational_fn(a.v.r, b.v.r));
                    } break ;
                    case arithmetic_t::DOUBLE: {
                        return make_from_double_fn(mul_double_fn(a.v.d, b.v.d));
                    } break ;
                    default: throw runtime_error("operation is not supported for this type");
                    }
                });

                auto div_complex_fn = complex_package->m_interface.find<function<complex_package_t::complex_t(complex_package_t::complex_t, complex_package_t::complex_t)>>("div-complex");
                auto div_double_fn = double_package->m_interface.find<function<double_package_t::double_t(double_package_t::double_t, double_package_t::double_t)>>("div-double");
                auto div_rational_fn = rational_package->m_interface.find<function<rational_package_t::rat_t(rational_package_t::rat_t, rational_package_t::rat_t)>>("div-rat");
                assert(div_complex_fn);
                assert(div_double_fn);
                assert(div_rational_fn);
                auto div_fn = self.m_interface.add<function<arithmetic_t(arithmetic_t, arithmetic_t)>>("div-arithmetic", [div_complex_fn, div_double_fn, div_rational_fn, make_from_complex_fn, make_from_double_fn, make_from_rat_fn](arithmetic_t a, arithmetic_t b) {
                    if (a.t != b.t) {
                        throw runtime_error("the two arithmetic types differ");
                    }
                    switch (a.t) {
                    case arithmetic_t::COMPLEX: {
                        return make_from_complex_fn(div_complex_fn(a.v.c, b.v.c));
                    } break ;
                    case arithmetic_t::RAT: {
                        return make_from_rat_fn(div_rational_fn(a.v.r, b.v.r));
                    } break ;
                    case arithmetic_t::DOUBLE: {
                        return make_from_double_fn(div_double_fn(a.v.d, b.v.d));
                    } break ;
                    default: throw runtime_error("operation is not supported for this type");
                    }
                });

                auto display_complex_fn = complex_package->m_interface.find<function<string(complex_package_t::complex_t)>>("display-complex");
                auto display_double_fn = double_package->m_interface.find<function<string(double_package_t::double_t)>>("display-double");
                auto display_rational_fn = rational_package->m_interface.find<function<string(rational_package_t::rat_t)>>("display-rat");
                assert(display_complex_fn);
                assert(display_double_fn);
                assert(display_rational_fn);
                auto display_fn = self.m_interface.add<function<string(arithmetic_t)>>("display-arithmetic", [display_complex_fn, display_double_fn, display_rational_fn](arithmetic_t a) {
                    switch (a.t) {
                    case arithmetic_t::COMPLEX: {
                        return display_complex_fn(a.v.c);
                    } break ;
                    case arithmetic_t::RAT: {
                        return display_rational_fn(a.v.r);
                    } break ;
                    case arithmetic_t::DOUBLE: {
                        return display_double_fn(a.v.d);
                    } break ;
                    default: throw runtime_error("operation is not supported for this type");
                    }
                });
            },
            [](program_t& self) {
                return true;
            },
            [](program_t& self) {
                return "arithmetic package";
            }
        )
    {
    }
};

#include <x86intrin.h>
int main() {
    uint64_t time_start = __rdtsc();
    program_t c1(
        [](program_t& self) {
            program_t* complex_package = program_database.get<complex_package_t>("complex package");
            assert(complex_package);

            self.add_input(complex_package);

            auto fn = complex_package->m_interface.find<function<complex_package_t::complex_t(double, double)>>("make-from-real-imag");
            assert(fn);

            complex_package_t::complex_t* z = self.m_state.add<complex_package_t::complex_t>("z", fn(42.0, -42.0));
        },
        [](program_t& self) {
            return true;
        },
        [](program_t& self) {
            program_t* complex_package = program_database.get<complex_package_t>("complex package");
            auto fn = complex_package->m_interface.find<function<string(complex_package_t::complex_t)>>("display");
            assert(fn);
            return fn(*self.m_state.find<complex_package_t::complex_t>("z"));
        }
    );
    program_t c2(
        [](program_t& self) {
            program_t* double_package = program_database.get<double_package_t>("double package");
            assert(double_package);

            self.add_input(double_package);

            auto fn = double_package->m_interface.find<function<double_package_t::double_t(double)>>("make-from-double");
            assert(fn);

            self.m_state.add<double_package_t::double_t>("d", fn(42.42));
        },
        [](program_t& self) {
            return true;
        },
        [](program_t& self) {
            program_t* double_package = program_database.get<double_package_t>("double package");
            auto get_double_fn = double_package->m_interface.find<function<double(double_package_t::double_t)>>("get-double");
            assert(get_double_fn);

            return to_string(get_double_fn(*self.m_state.find<double_package_t::double_t>("d")));
        }
    );
    program_t adder(
        [](program_t& self) {
            self.m_state_size = sizeof(arithmetic_package_t::arithmetic_t);
            self.m_state = calloc(1, self.m_state_size);
            
            program_t* arithmetic_package = program_database.get<arithmetic_package_t>("arithmetic package");

            self.add_input(arithmetic_package);
        },
        [](program_t& self) {
            if (self.m_inputs.empty()) {
                return false;
            }

            program_t* arithmetic_package = self.m_inputs[0];
            auto add_fn = arithmetic_package->m_interface.find<function<arithmetic_package_t::arithmetic_t(arithmetic_package_t::arithmetic_t, arithmetic_package_t::arithmetic_t)>>("add-arithmetic");
            if (!add_fn) {
                return false;
            }

            auto make_zero_fn = arithmetic_package->m_interface.find<function<arithmetic_package_t::arithmetic_t()>>("make-zero");
            arithmetic_package_t::arithmetic_t* arithmetic = (arithmetic_package_t::arithmetic_t*) self.m_state;
            *arithmetic = make_zero_fn();
            for (int i = 1; i < self.m_inputs.size(); ++i) {
                auto get_arithmetic_fn = self.m_inputs[i]->m_interface.find<function<arithmetic_package_t::arithmetic_t()>>("get-arithmetic");
                if (get_arithmetic_fn) {
                    *arithmetic = add_fn(*arithmetic, get_arithmetic_fn());
                }
            }

            return true;
        },
        [](program_t& self) -> string {
            program_t* arithmetic_package = self.m_inputs[0];
            if (!arithmetic_package) {
                return "";
            }

            auto display_fn = arithmetic_package->m_interface.find<function<string(arithmetic_package_t::arithmetic_t)>>("display-arithmetic");
            assert(display_fn);
            return display_fn(*(arithmetic_package_t::arithmetic_t*) self.m_state);
        }
    );

    cout << "Running adder: " << adder.run() << endl;
    cout << "Display of adder: " << adder.display() << endl;
    adder.add_input(&c2);
    for (int i = 0; i < 10; ++i) {
        cout << "Running adder: " << adder.run() << endl;
        cout << "Display of adder: " << adder.display() << endl;
    }
    uint64_t time_end = __rdtsc();
    cout << (time_end - time_start) / 1000.0 << "k cycles" << endl;

    return 0;
}
