#include "ipc_cpp/mem.h"

#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <chrono>
#include <boost/process.hpp>

using namespace boost::interprocess;
using namespace ipc_mem;

struct idk {
    char c;

    friend std::ostream& operator<<(std::ostream& os, idk& self);
};

std::ostream& operator<<(std::ostream& os, idk& self) {
    os << self.c;
    
    return os;
}

struct smthin {
    int a;
    double b;
    ipc_mem::shared_vector_t<ipc_mem::offset_ptr_t<idk>> v;

    bool operator<(const smthin& other) const {
        return a < other.a;
    }

    std::string describe() const {
        return std::to_string(a);
    }

    friend std::ostream& operator<<(std::ostream& os, smthin& self);
};

std::ostream& operator<<(std::ostream& os, smthin& self) {
    os << "{ " << self.a << ", " << self.b << ", " << self.v << " }";

    return os;
}

struct obj_t {
    int i;
    ipc_mem::offset_ptr_t<double> ptr2;
    ipc_mem::shared_vector_t<int> vector;
    ipc_mem::shared_vector_t<ipc_mem::shared_ptr_t<smthin>> vector2;
    ipc_mem::shared_vector_t<ipc_mem::offset_ptr_t<smthin>> vector3;
    ipc_mem::shared_set_t<int> set;
    ipc_mem::shared_set_t<offset_ptr<smthin>> set2;
    ipc_mem::shared_string_t<char> string;
    ipc_mem::shared_ptr_t<double> ptr;
    ipc_mem::offset_ptr_t<void(*)(int)> fns[3];
    ipc_mem::offset_ptr_t<std::string(*)()> fn2;
    ipc_mem::offset_ptr_t<std::string(*)(ipc_mem::offset_ptr_t<smthin>)> fn3;
    ipc_mem::shared_map_t<ipc_mem::shared_string_t<char>, int> map;
    ipc_mem::shared_deque_t<int> deque;
};

int main(int argc, char** argv) {
    if (argc == 1) {
        std::string shared_memory_name("fsdhd");

        init(shared_memory_name, 16 * 1024);

        {
            offset_ptr_t<obj_t> obj = malloc_named<obj_t>("obj");
            obj->i = 2;
            obj->ptr2 = malloc<double>(2.3);
            obj->vector.write([](auto& self, auto& vector) {
                (void) self;
                vector.push_back(4);
                vector.push_back(2);
            });
            obj->vector2.write([](auto& self, auto& vector) {
                (void) self;
                vector.push_back(malloc_shared<smthin>());
            });
            offset_ptr_t<smthin> sm1 = malloc_named<smthin>("abcd");
            sm1->a = 4;
            sm1->b = 2.0;
            sm1->v.write([](auto& self, auto& v) {
                (void) self;
                v.push_back(malloc<idk>(idk{ .c = 'c' }));
            });
            obj->vector3.write([sm1](auto& self, auto& vector) {
                (void) self;
                vector.push_back(sm1);
            });
            obj->string.write([](auto& self, auto& string) {
                (void) self;
                string = "sup";
            });
            obj->ptr = malloc_shared<double>(2.3);
            obj->fn3 = malloc<typename decltype(obj->fn3)::value_type>([](auto sm) {
                std::string result("Sup from parent's lambda: " + std::to_string(sm->a) + " " + std::to_string(sm->b));
                sm->v.read([&result](auto& self, auto& v) {
                    (void) self;
                    for (auto i : v) {
                        result += i->c;
                        result += " ";
                    }
                });
                return result;
            });
            obj->map.write([](auto& self, auto& map) {
                (void) self;
                // map.emplace("idk", 2);
                map.emplace(std::string("idk"), 2);
            });

            std::cout << "obj->i before running child process: " << obj->i << std::endl;
            std::cout << "obj->vector before running child process: " << obj->vector << std::endl;
            std::cout << "obj->vector2 before running child process: " << obj->vector2 << std::endl;
            std::cout << "obj->vector3 before running child process: " << obj->vector3 << std::endl;
            std::cout << "obj->set before running child process: " << obj->set << std::endl;
            std::cout << "obj->set2 before running child process: " << obj->set2 << std::endl;
            std::cout << "obj->string before running child process: " << obj->string << std::endl;
            std::cout << "*obj->ptr before running child process: " << *obj->ptr << std::endl;
            std::cout << "*obj->ptr2 before running child process: " << *obj->ptr2 << std::endl;
            std::cout << "obj->fns[0] before running child process: " << obj->fns[0] << std::endl;
            std::cout << "obj->fns[1] before running child process: " << obj->fns[1] << std::endl;
            std::cout << "obj->fns[2] before running child process: " << obj->fns[2] << std::endl;
            std::cout << "obj->fn2 before running child process: " << obj->fn2 << std::endl;
            std::cout << "obj->map before running child process: " << obj->map << std::endl;
            std::cout << "obj->deque before running child process: " << obj->deque << std::endl;

            using namespace boost::process;
            ipstream c_cout;
            ipstream c_cerr;
            child c("test_allocator " + shared_memory_name, std_out > c_cout, std_err > c_cerr);

            const auto get_messages = [&]() {
                std::string s;
                while (std::getline(c_cout, s)) {
                    if (!s.empty()) {
                        std::cout << s << std::endl;
                    }
                }
                while (std::getline(c_cerr, s)) {
                    if (!s.empty()) {
                        std::cerr << s << std::endl;
                    }
                }
            };
            while (c.running()) {
                get_messages();
                std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.1));
            }
            get_messages();

            c.wait();

            std::cout << "obj->i after running child process: " << obj->i << std::endl;
            std::cout << "obj->vector after running child process: " << obj->vector << std::endl;
            std::cout << "obj->vector2 after running child process: " << obj->vector2 << std::endl;
            std::cout << "obj->vector3 after running child process: " << obj->vector3 << std::endl;
            std::cout << "obj->set after running child process: " << obj->set << std::endl;
            std::cout << "obj->set2 after running child process: " << obj->set2 << std::endl;
            std::cout << "obj->string after running child process: " << obj->string << std::endl;
            std::cout << "*obj->ptr after running child process: " << *obj->ptr << std::endl;
            std::cout << "*obj->ptr2 after running child process: " << *obj->ptr2 << std::endl;
            std::cout << "obj->fns[0] after running child process: " << obj->fns[0] << std::endl;
            std::cout << "obj->fns[1] after running child process: " << obj->fns[1] << std::endl;
            std::cout << "obj->fns[2] after running child process: " << obj->fns[2] << std::endl;
            std::cout << "obj->fn2 after running child process: " << obj->fn2 << std::endl;
            std::cout << "obj->map after running child process: " << obj->map << std::endl;
            std::cout << "obj->deque after running child process: " << obj->deque << std::endl;
            if (obj->fns[0]) {
                (*obj->fns[0])(5);
            }
            if (obj->fns[1]) {
                (*obj->fns[1])(5);
            }
            if (obj->fns[2]) {
                (*obj->fns[2])(5);
            }
            if (obj->fn2) {
                std::cout << (*obj->fn2)() << std::endl;
            }

            free(obj);
        }


        deinit();
    } else {
        init(argv[1]);

        offset_ptr<obj_t> obj = find_named<obj_t>("obj");

        obj->i = 4;
        obj->vector.write([](auto& self, auto& vector) {
            (void) self;
            vector.push_back(4);
        });

        std::cerr << "Child process: something went wrong (not really)" << std::endl;
        std::cout << "Hello from child process" << std::endl;
        
        obj->vector2.write([](auto& self, auto& vector) {
            (void) self;
            vector.push_back(malloc_shared<smthin>());
        });
        offset_ptr_t<smthin> sm2 = malloc<smthin>();
        sm2->a = 4;
        sm2->b = 5.3;
        offset_ptr_t<smthin> sm3 = malloc<smthin>();
        sm3->a = 2;
        sm3->b = 6.4;

        obj->vector3.write([&sm2, &sm3](auto& self, auto& vector){
            (void) self;
            vector.push_back(sm2);
            vector.push_back(sm3);
        });


        obj->set2.write([](auto& self, auto& set){
            (void) self;
            offset_ptr_t<smthin> s = malloc<smthin>();
            s->a = 4;
            s->b = 2.0;
            s->v.write([](auto& self, auto& v) {
                (void) self;
                offset_ptr_t<idk> i2 = malloc<idk>();
                i2->c = '8';
                v.push_back(i2);
            });

            set.emplace(s);
        });

        offset_ptr_t<smthin> abcd = find_named<smthin>("abcd");
        if (abcd) {
            abcd->a = -4;
            abcd->b = -2.0;
            offset_ptr_t<idk> idk;
            abcd->v.read([&idk](auto& self, auto& v){
                (void) self;
                idk = v[0];
            });
            idk->c = 'd';
        }

        {
            obj->deque.write([](auto& self, auto& deque) {
                (void) self;
                deque.push_back(4);
                deque.push_back(2);
                deque.push_back(4);
                deque.push_back(2);
            });
        }

        obj->set.write([](auto& self, auto& set){
            (void) self;
            set.insert(2);
            set.insert(2);
            set.insert(3);
            set.insert(4);
        });
        obj->string.write([](auto& self, auto& string) {
            (void) self;
            string = "hello from child process";
        });
        *obj->ptr = -42.42;
        *obj->ptr2 = -4322.42;

        obj->map.write([](auto& self, auto& map){
            (void) self;
            map.emplace("yo sup from child", 42);
        });

        obj->fns[0] = malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda created by child: " << a << std::endl;
        });
        obj->fns[1] = malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda2 created by child: " << a << std::endl;
        });
        obj->fns[2] = malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda3 created by child: " << a << std::endl;
        });
        obj->fn2 = malloc<std::string(*)()>([]() {
            return std::string("Sup from child's lambda");
        });

        obj->vector3.read([&obj](auto& self, auto& vector) {
            (void) self;
            if (!vector.empty()) {
                std::cout << (*obj->fn3)(vector[0]) << std::endl;
                std::cout << "Describe: " << vector[0]->describe() << std::endl;
            }
        });

        deinit();
    }

    return 0;
}
