#include "audio_nodes.h"
#include "portaudio.h"

#include <mutex>
#include <unordered_set>
#include <iostream>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <numeric>

using namespace std;

static bool check_pa_error(PaError err) {
    if (err == paNoError) {
        return false;
    }

    if (err == paUnanticipatedHostError) {
        const PaHostErrorInfo* host_error_info = Pa_GetLastHostErrorInfo();
        cerr << "Host error occured:" << endl;
        cerr << "  host API type: " << host_error_info->hostApiType << endl;
        cerr << "  error code: " << host_error_info->errorCode << endl;
        cerr << "  text: " << host_error_info->errorText << endl;
    } else {
        cerr << "Error occured: " << Pa_GetErrorText(err) << " (" << err << ")" << endl;
    }

    return true;
}

sine_wave_node_t::sine_wave_node_t():
    node_t(
        [](node_t& node) {
            memory_slice_t& result = node.write();
            result.size = sizeof(sine_wave_t); // freq
            result.memory = malloc(result.size);

            return true;
        },
        std::move([](const vector<node_t*>& inputs, memory_slice_t& result) -> bool {
            sine_wave_t* sine_wave = (sine_wave_t*) result.memory;

            if (0 < inputs.size()) {
                const memory_slice_t& result = inputs[0]->read();
                if (sizeof(double) <= result.size) {
                    sine_wave->frequency = *(double*) result.memory;
                }
            }
            if (1 < inputs.size()) {
                const memory_slice_t& result = inputs[1]->read();
                if (sizeof(double) <= result.size) {
                    sine_wave->amplitude = *(double*) result.memory;
                }
            }
            if (2 < inputs.size()) {
                const memory_slice_t& result = inputs[2]->read();
                if (sizeof(double) <= result.size) {
                    sine_wave->phase = *(double*) result.memory;
                }
            }

            return true;
        }),
        std::move([](const memory_slice_t& result, string& str) {
            sine_wave_t* sine_wave = (sine_wave_t*) result.memory;
            str = "frequency [Hz]: " + to_string(sine_wave->frequency) + ", amplitude: " + to_string(sine_wave->amplitude) + ", phase [rad]: " + to_string(sine_wave->phase);
        }),
        std::move([](node_t& node) {
        })
    )
{
}

struct output_device_node_data_t {
    PaStream* stream = 0;
    PaHostApiIndex host_api_index = 0;
    const PaHostApiInfo* host_api_info = 0;
    const PaDeviceInfo* device_info = 0;
    static constexpr size_t buffer_size = 4096;
    size_t buffer_write = 0;
    size_t buffer_read = 0; // ensure only 1 consumer advances this
    float buffer[buffer_size] = { 0 };
};

static int n_output_device_nodes = 0;
audio_mixer_node_t::audio_mixer_node_t():
    node_t(
        [this](node_t& node) -> bool {
            if (n_output_device_nodes == 0) {
                if (check_pa_error(Pa_Initialize())) {
                    return false;
                }
            }

            memory_slice_t result = node.write();
            result.size = sizeof(output_device_node_data_t);
            result.memory = calloc(1, result.size);

            output_device_node_data_t* output_device_node_data = (output_device_node_data_t*) result.memory;

            output_device_node_data->host_api_index = Pa_GetDefaultHostApi();
            output_device_node_data->host_api_info = Pa_GetHostApiInfo(output_device_node_data->host_api_index);
            output_device_node_data->device_info = Pa_GetDeviceInfo(output_device_node_data->host_api_info->defaultOutputDevice);

            PaStreamParameters stream_parameters = {
                .device = output_device_node_data->host_api_info->defaultOutputDevice,
                .channelCount = output_device_node_data->device_info->maxOutputChannels,
                .sampleFormat = paFloat32,
                .suggestedLatency = output_device_node_data->device_info->defaultLowOutputLatency,
                .hostApiSpecificStreamInfo = 0
            };

            if (check_pa_error(Pa_OpenStream(
                &output_device_node_data->stream,
                0,
                &stream_parameters,
                output_device_node_data->device_info->defaultSampleRate,
                paFramesPerBufferUnspecified,
                paNoFlag,
                [](
                    const void* input,
                    void* output,
                    unsigned long frame_count,
                    const PaStreamCallbackTimeInfo* time_info,
                    PaStreamCallbackFlags status_flags,
                    void* user_data
                ) -> int {
                    output_device_node_data_t* output_device_node_data = (output_device_node_data_t*) user_data;

                    // LOG("  CPU load to maintain real-time operations [%]: " << Pa_GetStreamCpuLoad(callback_data->stream) * 100.0);

                    // cout << "  Input: " << input << endl;
                    // cout << "  Output: " << output << endl;

                    // cout << "  Frame count: " << frame_count << endl;

                    // cout << "  Time of invocation: " << time_info->currentTime << endl;
                    // cout << "  Time of first ADC sample: " << time_info->inputBufferAdcTime << endl;
                    // cout << "  Expected time of first DAC sample: " << time_info->outputBufferDacTime << endl;

                    // cout << "  Status flags:" << endl;
                    // cout << "    underflow: ";
                    // if (paOutputUnderflow & status_flags) {
                    //     cout << "output data (or a gap) was inserted, possibly stream is using too much CPU time";
                    // } else {
                    //     cout << "output data (or a gap) wasn't inserted, callback is doing fine";
                    // }
                    // cout << endl;

                    // cout << "    overflow: ";
                    // if (paOutputOverflow & status_flags) {
                    //     cout << "output data will be discarded, because no room is available";
                    // } else {
                    //     cout << "output data won't be discard, there is enough room";
                    // }
                    // cout << endl;

                    // cout << "    priming output: ";
                    // if (paPrimingOutput & status_flags) {
                    //     cout << "some of all of the output data will be used to prime the stream, input data may be zero";
                    // } else {
                    //     cout << "none of the output data will be used to prime the stream";
                    // }
                    // cout << endl;

                    // cout << "  User_data: " << user_data << endl;

                    // paAbort - stream will finish asap
                    // paComplete - stream will finish once all buffers generated are played
                    // these are not necessary tho, as Pa_StopStream(), Pa_AbortStream() or Pa_CloseStream() can also be used

                    {
                        size_t available_to_read = 0;
                        size_t write_p = output_device_node_data->buffer_write;
                        size_t read_p = output_device_node_data->buffer_read;
                        if (read_p <= write_p) {
                            available_to_read = write_p - read_p;
                        } else {
                            available_to_read = (output_device_node_data->buffer_size + write_p - read_p) % output_device_node_data->buffer_size;
                        }
                        // LOG("[CALLBACK] before, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
                        bool write_zeros = available_to_read < frame_count;

                        assert(output);
                        float* buffer = (float*) output;
                        // interleaved data: 1st sample: [channel0 channel1 ..], 2nd sample: [channel0 channel1 ..], ...
                        if (write_zeros) {
                            for (size_t i = 0; i < frame_count; ++i) {
                                for (size_t channel_index = 0; channel_index < output_device_node_data->device_info->maxOutputChannels; ++channel_index) {
                                    *buffer++ = 0.0f;
                                }
                            }
                        } else {
                            for (size_t i = 0; i < frame_count; ++i) {
                                float read_val = output_device_node_data->buffer[output_device_node_data->buffer_read++];
                                if (output_device_node_data->buffer_size <= output_device_node_data->buffer_read) {
                                    output_device_node_data->buffer_read = 0;
                                }
                                for (size_t channel_index = 0; channel_index < output_device_node_data->device_info->maxOutputChannels; ++channel_index) {
                                    *buffer++ = read_val;
                                }
                            }
                        }

                        // write_p = output_device_node_data->buffer_write;
                        // read_p = output_device_node_data->buffer_read;
                        // if (read_p <= write_p) {
                        //     available_to_read = write_p - read_p;
                        // } else {
                        //     available_to_read = (output_device_node_data->buffer_size + write_p - read_p) % output_device_node_data->buffer_size;
                        // }
                        // LOG("[CALLBACK] after, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
                    }

                    return paContinue;
                },
                (void*) output_device_node_data
            ))) {
                return false;
            }
            if (check_pa_error(Pa_StartStream(output_device_node_data->stream))) {
                return false;
            }

            mixer_thread_is_running = true;
            mixer_thread = thread([this]() {
                memory_slice_t& result = write();
                output_device_node_data_t* output_device_node_data = (output_device_node_data_t*) result.memory;

                double t = 0.0;
                const double time_step = 1.0f / output_device_node_data->device_info->defaultSampleRate;

                while (mixer_thread_is_running) {
                    size_t available_to_write = 0;
                    size_t write_p = output_device_node_data->buffer_write;
                    size_t read_p = output_device_node_data->buffer_read;
                    if (read_p <= write_p) {
                        available_to_write = (output_device_node_data->buffer_size - (write_p - read_p)) - 1;
                    } else {
                        available_to_write = (read_p - write_p) - 1;
                    }
                    // LOG("[MIXER] before, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);

                    if (!available_to_write) {
                        Pa_Sleep(1);
                        continue ;
                    }

                    {
                        // first pass to get informations on the resulting wave
                        constexpr float desired_min_mixed_value = -1.0f;
                        constexpr float desired_max_mixed_value = 1.0;
                        float min_mixed_value = FLT_MAX;
                        float max_mixed_value = -FLT_MAX;
                        double tmp_t = t;

                        const vector<node_t*>& inputs = read_inputs();
                        for (size_t i = 0; i < available_to_write; ++i) {
                            float mixed_value = 0.0f;
                            for (size_t sine_wave_index = 0; sine_wave_index < inputs.size(); ++sine_wave_index) {
                                memory_slice_t& result = inputs[sine_wave_index]->write();
                                if (sizeof(sine_wave_node_t::sine_wave_t) <= result.size) {
                                    sine_wave_node_t::sine_wave_t* sine_wave = (sine_wave_node_t::sine_wave_t*) result.memory;
                                    mixed_value += (float) (sine_wave->amplitude * sin(2.0 * M_PI * sine_wave->frequency * tmp_t + sine_wave->phase));
                                }
                            }
                            min_mixed_value = min(min_mixed_value, mixed_value);
                            max_mixed_value = max(max_mixed_value, mixed_value);
                            tmp_t += time_step;
                        }

                        // second pass to commit the changes
                        float mixed_value_multiplier = 1.0f;
                        if (min_mixed_value < desired_min_mixed_value) {
                            mixed_value_multiplier = desired_min_mixed_value / min_mixed_value;
                        }
                        if (desired_max_mixed_value < max_mixed_value) {
                            mixed_value_multiplier = min(mixed_value_multiplier, desired_max_mixed_value / max_mixed_value);
                        }
                        for (size_t i = 0; i < available_to_write; ++i) {
                            float mixed_value = 0.0f;
                            for (size_t sine_wave_index = 0; sine_wave_index < inputs.size(); ++sine_wave_index) {
                                if (sizeof(sine_wave_node_t::sine_wave_t) <= result.size) {
                                    sine_wave_node_t::sine_wave_t* sine_wave = (sine_wave_node_t::sine_wave_t*) result.memory;
                                    mixed_value += (float) (sine_wave->amplitude * sin(2.0 * M_PI * sine_wave->frequency * t + sine_wave->phase));
                                }
                            }
                            output_device_node_data->buffer[output_device_node_data->buffer_write++] = mixed_value * mixed_value_multiplier;
                            if (output_device_node_data->buffer_size <= output_device_node_data->buffer_write) {
                                output_device_node_data->buffer_write = 0;
                            }
                            t += time_step;
                        }
                    }

                    write_p = output_device_node_data->buffer_write;
                    read_p = output_device_node_data->buffer_read;
                    if (read_p <= write_p) {
                        available_to_write = (output_device_node_data->buffer_size - (write_p - read_p)) - 1;
                    } else {
                        available_to_write = (read_p - write_p) - 1;
                    }
                    // LOG("[MIXER] after, acquire, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);
                }
            });

            return true;
        },
        std::move([](const vector<node_t*>& node, memory_slice_t& result) -> bool {
            return true;
        }),
        std::move([](const memory_slice_t& result, string& str) {
            output_device_node_data_t* output_device_node_data = (output_device_node_data_t*) result.memory;
            // this is not very useful as it changes rapidly
            str = to_string(output_device_node_data->buffer[output_device_node_data->buffer_read]);
        }),
        std::move(([this](node_t& node) {
            if (mixer_thread_is_running) {
                mixer_thread_is_running = false;
                mixer_thread.join();
            }

            if (--n_output_device_nodes == 0) {
                (void) check_pa_error(Pa_Terminate());
            }
        }))
    )
{
}
