#include "builder.h"

int main() {
    COMPILER.add<mul_component_t>("mul");
    COMPILER.add<display_component_t>("display");
    COMPILER.add(
        "mul_display",
        std::vector<unsigned char>{
            OP_CREATE_WIRE,                                            // create wire 0
            OP_EXPOSE_PINS, 2,                                         // expose 2 pins of resulting component
            OP_CREATE_COMPONENT, 'm', 'u', 'l', 0,                     // create and add primitive component 0 "mul"
            OP_CREATE_COMPONENT, 'd', 'i', 's', 'p', 'l', 'a', 'y', 0, // create and add primitive component 1 "display"
            OP_CONNECT_WIRE_TO_COMPONENT, 0, 0, 2,                     // connect wire 0 to component 0's 2 pin
            OP_CONNECT_WIRE_TO_COMPONENT, 0, 1, 0,                     // connect wire 0 to component 1's 0 pin
            OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN, 0, 0, 0,           // connect component 0's 0 pin to resulting component's 0 pin
            OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN, 1, 0, 1,           // connect component 0's 1 pin to resulting component's 1 pin
        }
    );

    wire_t a;
    wire_t b;
    component_t* mul_display = COMPILER.create("mul_display");
    mul_display->connect(0, &a);
    mul_display->connect(1, &b);
    a.write<int>(3);
    b.write<int>(4);

    return 0;
}
