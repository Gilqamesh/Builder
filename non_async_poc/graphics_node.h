#include "node.h"

void update();

struct graphics_engine_node_t : public node_t {
    graphics_engine_node_t();
    ~graphics_engine_node_t();
};

struct graphics_node_t : public node_t {
    graphics_node_t(
        const function<bool(node_t&)>& init_fn,
        const function<bool(node_t&)>& run_fn,
        const function<string(const node_t&)>& describe_fn,
        const function<void(node_t&)>& deinit_fn,
        float x, float y
    );
    ~graphics_node_t();
};

struct constant_node_t : public graphics_node_t {
    constant_node_t();
};

struct sum_node_t : public graphics_node_t {
    sum_node_t();
};

struct accumulate_node_t : public graphics_node_t {
    accumulate_node_t();
};

struct decrement_node_t : public graphics_node_t {
    decrement_node_t();
};

struct time_node_t : public graphics_node_t {
    time_node_t();
};

struct sin_node_t : public graphics_node_t {
    sin_node_t();
};

struct mul_node_t : public graphics_node_t {
    mul_node_t();
};

struct floor_node_t : public graphics_node_t {
    floor_node_t();
};

struct sine_wave_node_t : public graphics_node_t {
    sine_wave_node_t();
};

struct audio_mixer_node_t : public graphics_node_t {
    audio_mixer_node_t();
};
