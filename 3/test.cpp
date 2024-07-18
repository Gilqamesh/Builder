#include <iostream>
#include <cassert>

struct dynamic_array_t;
struct data_structure_t;

struct data_structure_t {
    dynamic_array_t* dynamic_array_t;
};

struct dynamic_array_t {
    data_structure_t* data_structure;
};

size_t total_size = 0;

void* my_malloc(size_t size) {
    total_size += size;
    std::cout << "Total size: " << total_size << std::endl;
    return malloc(size);
}

static data_structure_t* data_structure__create();
static dynamic_array_t* dynamic_array__create();

static data_structure_t* data_structure__create() {
    data_structure_t* result = static_cast<data_structure_t*>(my_malloc(sizeof(*result)));
    result->dynamic_array_t = dynamic_array__create();
    return result;
}

static dynamic_array_t* dynamic_array__create() {
    dynamic_array_t* result = static_cast<dynamic_array_t*>(my_malloc(sizeof(*result)));
    result->data_structure = data_structure__create();
    return result;
}

int main() {
    dynamic_array_t* dynamic_array = dynamic_array__create();

    return 0;
}
