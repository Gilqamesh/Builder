#include "ipc_cpp/mem.h"

#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <chrono>
#include <boost/process.hpp>

using namespace boost::interprocess;

struct idk {
    char c;
};

struct smthin {
    smthin(shared_memory_t* shared_seg_memory):
        v(shared_seg_memory->get_allocator<offset_ptr_t<idk>>()) {
    }

    int a;
    double b;
    shared_vector_t<offset_ptr_t<idk>> v;

    std::string describe() const {
        return std::to_string(a);
    }
};

struct obj_t {
    obj_t(shared_memory_t* shared_seg_memory):
        ptr2(0),
        vector(shared_seg_memory->get_allocator<int>()),
        vector2(shared_seg_memory->get_allocator<shared_ptr_t<smthin>>()),
        vector3(shared_seg_memory->get_allocator<offset_ptr<smthin>>()),
        set(shared_seg_memory->get_allocator<int>()),
        string(shared_seg_memory->get_allocator<char>()) {
    }

    int i;
    offset_ptr<double> ptr2;
    shared_vector_t<int> vector;
    shared_vector_t<shared_ptr_t<smthin>> vector2;
    shared_vector_t<offset_ptr<smthin>> vector3;
    shared_set_t<int> set;
    shared_string_t<char> string;
    shared_ptr_t<double> ptr;
    offset_ptr<void(*)(int)> fns[3];
    offset_ptr<std::string(*)()> fn2;
    offset_ptr<std::string(*)(offset_ptr<smthin>)> fn3;
};

std::ostream& operator<<(std::ostream& os, const shared_set_t<int>& container) {
    auto it = container.begin();
    for (size_t i = 0; i < container.size(); ++i, ++it) {
        assert(it != container.end());
        os << *it;
        if (i < container.size() - 1) {
            os << ", ";
        }
    }

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_vector_t<T>& container) {
    auto it = container.begin();
    for (size_t i = 0; i < container.size(); ++i, ++it) {
        assert(it != container.end());
        os << *it;
        if (i < container.size() - 1) {
            os << ", ";
        }
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const shared_vector_t<offset_ptr<idk>>& container) {
    for (size_t i = 0; i < container.size(); ++i) {
        os << container[i]->c;
        if (i < container.size() - 1) {
            os << ", ";
        }
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const shared_vector_t<offset_ptr<smthin>>& container) {
    for (size_t i = 0; i < container.size(); ++i) {
        os << "{ " << container[i]->a << ", " << container[i]->b << " { " << container[i]->v << " } }";
        if (i < container.size() - 1) {
            os << ", ";
        }
    }

    return os;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::string shared_memory_name("fsdhd");

        shared_memory_t* shared_seg_memory = new shared_memory_t(shared_memory_name, 2 * 1024);

        {
            offset_ptr_t<obj_t> obj = shared_seg_memory->malloc_named<obj_t>("obj", shared_seg_memory);
            obj->i = 2;
            obj->ptr2 = shared_seg_memory->malloc<double>(2.3);
            obj->vector.push_back(4);
            obj->vector.push_back(2);
            obj->vector2.push_back(shared_seg_memory->malloc_shared<smthin>(shared_seg_memory));
            offset_ptr_t<smthin> sm1 = shared_seg_memory->malloc_named<smthin>("abcd", smthin(shared_seg_memory));
            sm1->a = 4;
            sm1->b = 2.0;
            sm1->v.push_back(shared_seg_memory->malloc<idk>(idk{ .c = 'c' }));
            obj->vector3.push_back(sm1);
            obj->string = "sup";
            obj->ptr = shared_seg_memory->malloc_shared<double>(2.3);
            const auto fn3 = [](offset_ptr<smthin> sm) -> std::string {
                std::string result("Sup from parent's lambda: " + std::to_string(sm->a) + " " + std::to_string(sm->b));
                for (auto i : sm->v) {
                    result += i->c;
                    result += " ";
                }
                return result;
            };
            obj->fn3 = shared_seg_memory->malloc<std::string(*)(offset_ptr<smthin>)>(fn3);

            std::cout << "obj->i before running child process: " << obj->i << std::endl;
            std::cout << "obj->vector before running child process: " << obj->vector << std::endl;
            std::cout << "obj->vector2 before running child process: " << obj->vector2 << std::endl;
            std::cout << "obj->vector3 before running child process: " << obj->vector3 << std::endl;
            std::cout << "obj->set before running child process: " << obj->set << std::endl;
            std::cout << "obj->string before running child process: " << obj->string << std::endl;
            std::cout << "*obj->ptr before running child process: " << *obj->ptr << std::endl;
            std::cout << "*obj->ptr2 before running child process: " << *obj->ptr2 << std::endl;
            std::cout << "obj->fns[0] before running child process: " << obj->fns[0] << std::endl;
            std::cout << "obj->fns[1] before running child process: " << obj->fns[1] << std::endl;
            std::cout << "obj->fns[2] before running child process: " << obj->fns[2] << std::endl;
            std::cout << "obj->fn2 before running child process: " << obj->fn2 << std::endl;

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
            std::cout << "obj->string after running child process: " << obj->string << std::endl;
            std::cout << "*obj->ptr after running child process: " << *obj->ptr << std::endl;
            std::cout << "*obj->ptr2 after running child process: " << *obj->ptr2 << std::endl;
            std::cout << "obj->fns[0] after running child process: " << obj->fns[0] << std::endl;
            std::cout << "obj->fns[1] after running child process: " << obj->fns[1] << std::endl;
            std::cout << "obj->fns[2] after running child process: " << obj->fns[2] << std::endl;
            std::cout << "obj->fn2 after running child process: " << obj->fn2 << std::endl;
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

            shared_seg_memory->free(obj);
        }

        delete shared_seg_memory;
    } else {
        shared_memory_t* shared_seg_memory = new shared_memory_t(argv[1]);

        offset_ptr<obj_t> obj = shared_seg_memory->find_named<obj_t>("obj");

        obj->i = 4;
        obj->vector.push_back(4);

        std::cerr << "Child process: something went wrong (not really)" << std::endl;
        std::cout << "Hello from child process" << std::endl;
        
        obj->vector2.push_back(shared_seg_memory->malloc_shared<smthin>(shared_seg_memory));
        offset_ptr_t<smthin> sm2 = shared_seg_memory->malloc<smthin>(smthin(shared_seg_memory));
        sm2->a = 4;
        sm2->b = 5.3;
        obj->vector3.push_back(sm2);
        offset_ptr_t<smthin> sm3 = shared_seg_memory->malloc<smthin>(smthin(shared_seg_memory));
        sm3->a = 2;
        sm3->b = 6.4;

        obj->vector3.push_back(sm3);

        offset_ptr_t<smthin> abcd = shared_seg_memory->find_named<smthin>("abcd");
        if (abcd) {
            abcd->a = -4;
            abcd->b = -2.0;
            offset_ptr_t<idk> idk = abcd->v[0].get();
            idk->c = 'd';
        }

        obj->set.insert(2);
        obj->set.insert(2);
        obj->set.insert(3);
        obj->set.insert(4);
        obj->string = "hello from child process";
        *obj->ptr = -42.42;
        *obj->ptr2 = -4322.42;

        obj->fns[0] = shared_seg_memory->malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda created by child: " << a << std::endl;
        });
        obj->fns[1] = shared_seg_memory->malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda2 created by child: " << a << std::endl;
        });
        obj->fns[2] = shared_seg_memory->malloc<void(*)(int)>([](int a) {
            std::cout << "Hello from lambda3 created by child: " << a << std::endl;
        });
        obj->fn2 = shared_seg_memory->malloc<std::string(*)()>([]() {
            return std::string("Sup from child's lambda");
        });
        if (!obj->vector3.empty()) {
            std::cout << (*obj->fn3)(obj->vector3[0]) << std::endl;
            std::cout << "Describe: " << obj->vector3[0]->describe() << std::endl;
        }

        delete shared_seg_memory;
    }

    return 0;
}
