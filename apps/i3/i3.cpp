#include "builder.h"

component_t* fact_component() {
    wire_t* a = new wire_t();
    wire_t* b = new wire_t();
    wire_t* c = new wire_t();
    wire_t* d = new wire_t();
    wire_t* e = new wire_t();
    wire_t* f = new wire_t();
    wire_t* g = new wire_t();
    wire_t* h = new wire_t();
    wire_t* i = new wire_t();
    wire_t* j = new wire_t();
    wire_t* k = new wire_t();
    wire_t* l = new wire_t();
    is_zero_component_t* is_zero = new is_zero_component_t;
    if_component_t* if_component = new if_component_t;
    one_component_t* one_1 = new one_component_t;
    one_component_t* one_2 = new one_component_t;
    mul_component_t* mul = new mul_component_t;
    sub_component_t* sub = new sub_component_t;
    fact_component_t* fact = new fact_component_t;
    pin_component_t* pin_1 = new pin_component_t;
    pin_component_t* pin_2 = new pin_component_t;
    pin_component_t* pin_3 = new pin_component_t;

    is_zero->set_name("is_zero");
    if_component->set_name("if");
    one_1->set_name("one");
    one_2->set_name("one");
    mul->set_name("mul");
    sub->set_name("sub");
    fact->set_name("fact");
    pin_1->set_name("pin");
    pin_2->set_name("pin");
    pin_3->set_name("pin");

    COMPILER.add<is_zero_component_t>("is_zero");
    COMPILER.add<if_component_t>("if");
    COMPILER.add<one_component_t>("one");
    COMPILER.add<mul_component_t>("mul");
    COMPILER.add<sub_component_t>("sub");
    COMPILER.add<pin_component_t>("pin");

    pin_1->connect(0, a);
    pin_1->connect(1, b);

    pin_3->connect(0, fact->at(1));

    pin_2->connect(0, g);
    pin_2->connect(1, h);

    mul->connect(0, i);

    fact->connect(0, j);

    sub->connect(0, k);

    one_2->connect(0, l);

    is_zero->connect(0, c);

    if_component->connect(0, e);
    if_component->connect(1, f);

    one_1->connect(0, d);

    pin_1->connect(0, fact->at(0));

    pin_3->connect(0, e);
    pin_3->connect(1, i);

    pin_2->connect(0, f);

    mul->connect(0, g);
    mul->connect(1, j);

    sub->connect(0, h);
    sub->connect(1, l);

    if_component->connect(0, c);
    if_component->connect(1, d);
    if_component->connect(2, b);

    is_zero->connect(0, a);

    fact->connect(0, k);

    std::vector<unsigned char> blueprint = COMPILER.assemble(
        2,
        {},
        {},
        { a, b, c, d, e, f, g, h, i, j, k, l },
        { is_zero, if_component, one_1, one_2, mul, sub, pin_1, pin_2, pin_3, fact }
    );

    std::cout << "fact blueprint: " << std::endl;
    for (auto c : blueprint) {
        std::cout << (int) c << ", ";
    }
    std::cout << std::endl;
    std::cout << COMPILER.disassemble(blueprint) << std::endl;

    /*
0, 3, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 105, 115, 95, 122, 101, 114, 111, 0, 6, 4, 0, 0, 5, 2, 0, 0, 1, 105, 102, 0, 6, 7, 1, 1, 6, 6, 1, 0, 5, 3, 1, 2, 5, 5, 1, 1, 5, 4, 1, 0, 1, 111, 110, 101, 0, 6, 5, 2, 0, 1, 111, 110, 101, 0, 6, 13, 3, 0, 1, 109, 117, 108, 0, 6, 10, 4, 0, 5, 11, 4, 1, 5, 8, 4, 0, 1, 115, 117, 98, 0, 6, 12, 5, 0, 5, 13, 5, 1, 5, 9, 5, 0, 1, 112, 105, 110, 0, 6, 3, 6, 1, 6, 2, 6, 0, 5, 0, 6, 0, 1, 112, 105, 110, 0, 6, 9, 7, 1, 6, 8, 7, 0, 5, 7, 7, 0, 1, 112, 105, 110, 0, 6, 1, 8, 0, 5, 10, 8, 1, 5, 6, 8, 0, 2, 102, 97, 99, 116, 0, 6, 11, 9, 0, 5, 12, 9, 0,
    */

    /*
        0, // wire in [0]
        4, 0, 0, // connect "wire in [0]" to resulting component input 0
        0, // wire a [1]
        5, 1, 0, // connect "wire a [1]" to resulting component output 0
        0, // wire b [2]
        0, // wire c [3]
        0, // wire d [4]
        0, // wire e [5]
        0, // wire f [6]
        0, // wire out [7]
        1, 105, 115, 95, 122, 101, 114, 111, 0, // create component "is_zero" [0]
        2, 0, 0, 0, // connect "wire in [0]" to component "is_zero"'s input 0
        3, 2, 0, 0, // connect component "is_zero"'s output 0 to "wire a [1]"
        1, 105, 102, 0, // create component "if" [1]
        2, 0, 1, 2, // connect "wire in [0]" to component "if"'s input 2
        2, 3, 1, 1, // connect "wire b [2]" to component "if"'s input 1
        2, 2, 1, 0, // connect "wire a [1]" to component "if"'s input 0
        3, 4, 1, 1, // connect component "if"'s output 1 to "wire c [3]"
        3, 1, 1, 0, // connect component "if"'s output 0 to "wire out [7]"
        1, 111, 110, 101, 0, // create component "one" [2]
        3, 3, 2, 0, // connect component "one"'s output 0 to "wire b [2]"
        1, 111, 110, 101, 0, // create component "one" [3]
        3, 7, 3, 0, // connect component "one"'s output 0 to "wire f [6]"
        1, 109, 117, 108, 0, // create component "mul" [4]
        2, 4, 4, 0, // connect "wire c [3]" to component "mul"'s input 0
        2, 5, 4, 1, // connect "wire d [4]" to component "mul"'s input 1
        3, 1, 4, 0, // connect component "mul"'s output 0 to "wire out [7]"
        1, 115, 117, 98, 0, // create component "sub" [5]
        2, 7, 5, 1, // connect "wire f [6]" to component "sub"'s input 1
        2, 4, 5, 0, // connect "wire c [3]" to component "sub"'s input 0
        3, 6, 5, 0, // connect component "sub"'s output 0 to "wire e [5]"
        6, 102, 97, 99, 116, 0, // create stub component "fact" [6]
        2, 6, 6, 0, // connect "wire e [5]" to component "fact"'s input 0
        3, 5, 6, 0 // connect component "fact"'s output 0 to "wire d [4]"
    */

    COMPILER.add("fact", blueprint);

    return COMPILER.create("fact");
}

int main() {
    COMPILER.add<pin_component_t>("display");
    COMPILER.add<display_component_t>("display");
    COMPILER.add<is_zero_component_t>("is_zero");
    COMPILER.add<if_component_t>("if");
    COMPILER.add<mul_two_component_t>("mul_two");
    COMPILER.add<one_component_t>("one");

    display_component_t display1;
    display_component_t display2;
    is_zero_component_t is_zero;
    one_component_t one;
    if_component_t if_component;
    mul_two_component_t mul_two;
    pin_component_t pin;
    wire_t a;
    wire_t b;
    wire_t c;
    wire_t d;
    wire_t e;
    wire_t f;
    wire_t g;

    pin.set_name("pin");
    display1.set_name("display");
    display2.set_name("display");
    is_zero.set_name("is_zero");
    one.set_name("one");
    if_component.set_name("if");
    mul_two.set_name("mul_two");

    is_zero.connect(0, &a);
    is_zero.connect(1, &g);

    one.connect(0, &c);

    if_component.connect(0, &g);
    if_component.connect(1, &c);
    if_component.connect(2, &b);
    if_component.connect(3, &d);
    if_component.connect(4, &e);

    display1.connect(0, &d);

    mul_two.connect(0, &e);
    mul_two.connect(1, &f);

    display2.connect(0, &f);

    std::vector<unsigned char> blueprint = COMPILER.assemble(
        1,
        { &pin },
        { 0 },
        { &a, &b, &c, &d, &e, &f, &g },
        { &is_zero, &one, &if_component, &mul_two, &display1, &display2 }
    );

    std::cout << "blueprint: " << std::endl;
    for (auto c : blueprint) {
        std::cout << (int) c << ", ";
    }
    std::cout << std::endl;

    std::string blueprint_name = "prototype1";
    COMPILER.add(blueprint_name, blueprint);

    /*
        0, // wire in [0]
        4, 0, 0, // connect input wire 0 to resulting component input 0
        0, // wire a [1]
        0, // wire b [2]
        0, // wire c [3]
        0, // wire d [4]
        0, // wire e [5]
        1, 105, 115, 95, 122, 101, 114, 111, 0, // create component "is_zero" [0]
        2, 0, 0, 0, // connect "wire in" to component "is_zero"'s input 0
        3, 1, 0, 0, // connect component "is_zero"'s output 0 to "wire a"
        1, 111, 110, 101, 0, // create component "one" [1]
        3, 5, 1, 0, // connect component "one"'s output 0 to "wire e"
        1, 105, 102, 0, // create component "if" [2]
        2, 0, 2, 2, // connect "wire in" to component "if"'s input 2
        2, 5, 2, 1, // connect "wire e" to component "if"'s input 1
        2, 1, 2, 0, // connect "wire a" to component "if"'s input 0
        3, 3, 2, 1, // connect component "if"'s output 1 to "wire c"
        3, 2, 2, 0, // connect component "if"'s output 0 to "wire b"
        1, 109, 117, 108, 95, 116, 119, 111, 0, // create component "mul_two" [3]
        2, 3, 3, 0, // connect "wire c" to component "mul_two"'s input 0
        3, 4, 3, 0, // connect component "mul_two"'s output 0 to "wire d"
        1, 100, 105, 115, 112, 108, 97, 121, 0, // create component "display" [4]
        2, 2, 4, 0, // connect "wire b" to component "display"'s input 0
        1, 100, 105, 115, 112, 108, 97, 121, 0, // create component "display" [5]
        2, 4, 5, 0 // connect "wire d" to component "display"'s input 0
    */

    component_t* prototype1 = COMPILER.create(blueprint_name);

    wire_t in;
    in.write<int>(0); // -> 1
    in.write<int>(5); // -> 10

    prototype1->connect(0, &in);

    in.write<int>(0); // -> 1
    in.write<int>(6); // -> 12

    prototype1->disconnect(0, &in);

    component_t* prototype2 = COMPILER.create(blueprint_name);

    prototype2->connect(0, &in);
    in.write<int>(0); // -> 1
    in.write<int>(6); // -> 12

    component_t* fact = fact_component();
    fact->at(0)->write<int>(5); // -> 120
    int result;
    if (fact->at(1)->read<int>(result)) {
        std::cout << "fact(5) = " << result << std::endl;
    }

    return 0;
}
