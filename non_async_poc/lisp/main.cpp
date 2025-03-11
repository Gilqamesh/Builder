#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <climits>
#include <limits>
#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>

#include "raylib.h"

using namespace std;

struct obj_t;

struct env_t {
    unordered_map<string, obj_t*> env;
};

struct obj_t {
    obj_t() = default;
    virtual ~obj_t() = default;

    virtual obj_t* eval(env_t env) = 0;
    virtual void print(ostream&, env_t) = 0;
};

bool is_nil(obj_t* obj);

struct obj_pair_t : public obj_t {
    obj_t* car = 0;
    obj_t* cdr = 0;

    obj_t* eval(env_t env) override {
        return 0;
    }

    void print(ostream& os, env_t env) override {
        obj_t* last = cdr;
        while (!is_nil(last)) {
            if (obj_pair_t* obj_pair = dynamic_cast<obj_pair_t*>(last)) {
                last = obj_pair->cdr;
            } else {
                break ;
            }
        }

        os << "(";
        if (!is_nil(last)) {
            car->print(os, env);
            os << " . ";
            cdr->print(os, env);
        } else {
            obj_pair_t* cur_pair = this;
            while (!is_nil(cur_pair)) {
                cur_pair->car->print(os, env);
                cur_pair = static_cast<obj_pair_t*>(cur_pair->cdr);
                if (!is_nil(cur_pair)) {
                    cout << " ";
                }
            }
        }
        os << ")";
    }
};

struct obj_int64_t : public obj_t {
    obj_int64_t(int64_t val): val(val) {}

    int64_t val = 0;

    obj_t* eval(env_t env) override {
        return this;
    }

    void print(ostream& os, env_t env) override {
        os << val;
    }
};

struct obj_str_t : public obj_t {
    obj_str_t(const string& s) : str(s) {}

    string str;

    obj_t* eval(env_t env) override {
        return this;
    }

    void print(ostream& os, env_t env) override {
        os << "\"" << str << "\"";
    }
};

struct obj_symbol_t : public obj_t {
    obj_symbol_t(const string& str) : symbol(str) {}

    string symbol;

    obj_t* eval(env_t env) override {
        auto it = env.env.find(symbol);
        if (it == env.env.end()) {
            cerr << "could not find symbol in the environment" << endl;
            return 0;
        }
        return it->second;
    }

    void print(ostream& os, env_t env) override {
        if (symbol == "nil") {
            os << "nil";
            return ;
        }
        auto it = env.env.find(symbol);
        if (it == env.env.end()) {
            cerr << "could not find symbol in the environment" << endl;
            return ;
        }
        it->second->print(os, env);
    }
};

struct obj_ptr_t : public obj_t {
    void* ptr = 0;

    obj_t* eval(env_t env) override {
        return 0;
    }

    void print(ostream& os, env_t env) override {
        return ;
    }
};

bool is_nil(obj_t* obj) {
    obj_symbol_t* obj_symbol = dynamic_cast<obj_symbol_t*>(obj);
    return obj_symbol && obj_symbol->symbol == "nil";
}

obj_pair_t* cons(obj_t* a, obj_t* b) {
    obj_pair_t* obj_pair = new obj_pair_t;
    obj_pair->car = a;
    obj_pair->cdr = b;
    return obj_pair;
}

obj_pair_t* list(const vector<obj_t*> objs) {
    obj_pair_t* obj = new obj_pair_t;
    for (int i = objs.size() - 1; 0 <= i; --i) {
        obj = cons(objs[i], obj);
    }
    return obj;
}

obj_t* car(obj_pair_t* obj_pair) {
    return obj_pair->car;
}

obj_t* cdr(obj_pair_t* obj_pair) {
    return obj_pair->cdr;
}

obj_t* read(const char** str);

void read_whitespaces(const char** str) {
    while (**str == ' ') {
        ++*str;
    }
}

obj_t* read_combination(const char** str) {
    obj_t* obj = read(str);
    if (**str && **str != ')') {
        read_whitespaces(str);
        obj = cons(obj, read_combination(str));
    }
    return obj;
}

obj_str_t* read_string(const char** str) {
    const char* start = *str;
    while (**str && **str != '"') {
        ++*str;
    }
    if (**str != '"') {
        cerr << "expected '\"' at the end of string" << endl;
        return 0;
    }
    obj_str_t* obj_str = new obj_str_t(string(start, *str - start));
    ++*str;
    return obj_str;
}

obj_int64_t* read_number(const char** str) {
    int64_t val = 0;
    while (**str && **str != ' ' && **str != ')') {
        if (**str < '0' || '9' < **str) {
            cerr << "expected number to only contain digits" << endl;
            return 0;
        }
        val = 10 * val + **str - '0';
        ++*str;
    }
    return new obj_int64_t(val);
}

obj_symbol_t* read_symbol(const char** str) {
    const char* start = *str;
    while (**str && **str != ' ' && **str != ')') {
        ++*str;
    }
    if (start == *str) {
        cerr << "cannot read symbol size of 0" << endl;
        return 0;
    }
    return new obj_symbol_t(string(start, *str - start));
}

// todo: take in istream as argument
obj_t* read(const char** str) {
    read_whitespaces(str);
    if (**str == '(') {
        ++*str;
        obj_t* obj = read_combination(str);
        if (**str != ')') {
            throw runtime_error("expected ')' at the end of combination");
        }
        ++*str;
        return obj;
    } else if (**str == '"') {
        return read_string(str);
    } else if ('0' <= **str && **str <= '9') {
        return read_number(str);
    } else {
        return read_symbol(str);
    }
}

int main() {
    env_t env;
    env.env["nil"] = 0;
    string line;
    while (getline(cin, line) && !line.empty()) {
        const char* data = line.c_str();
        while (*data) {
            try {
                obj_t* obj = read(&data);
                assert(obj);
                obj->eval(env);
                obj->print(cout, env);
                cout << endl;
            } catch (const exception& err) {
                cerr << "Exception occured: " << err.what() << endl;
            }
        }
    }

    return 0;
}

