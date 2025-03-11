#ifndef AUDIO_NODES_H
# define AUDIO_NODES_H

# include "node.h"

struct sine_wave_node_t : public node_t {
    struct sine_wave_t {
        double frequency;
        double amplitude;
        double phase;
    };
    sine_wave_t wave_params;

public:
    sine_wave_node_t();
};

struct audio_mixer_node_t : public node_t {
    thread mixer_thread;
    bool mixer_thread_is_running = false;
public:
    audio_mixer_node_t();
};

#endif // AUDIO_NODES_H
