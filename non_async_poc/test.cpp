#include "portaudio.h"
#include "raylib.h"
#include "rlgl.h"

#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cfloat>
#include <numeric>

using namespace std;

mutex cout_mutex;
#define LOG(x) do { \
    { \
        scoped_lock<mutex> lock(cout_mutex); \
        cout << x << endl; \
    } \
} while (0)

PaError err = paNoError;

int w_width = 1800;
int w_height = 1000;

void DrawLines(Vector2* points, size_t count, Color color) {
    rlBegin(RL_LINES);
    rlColor4ub(color.r, color.g, color.b, color.a);

    for (size_t i = 0; i < count; ++i) {
        rlVertex2f(points[i].x, points[i].y);
        if (i < count - 1) {
            rlVertex2f(points[i + 1].x, points[i + 1].y);
        }
    }

    rlEnd();
}

void DrawPoints(Vector2* point, size_t count, float radius, Color color) {
    const int num_segments = 12;
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);

    for (size_t i = 0; i < count; ++i) {
        for (int j = 0; j < num_segments; ++j) {
            float theta1 = 2.0f * PI * (float) j / (float) num_segments;
            float theta2 = 2.0f * PI * (float) (j + 1) / (float) num_segments;

            rlVertex2f(point->x, point->y);
            rlVertex2f(point->x + radius * cosf(theta2), point->y + radius * sinf(theta2));
            rlVertex2f(point->x + radius * cosf(theta1), point->y + radius * sinf(theta1));
        }
        ++point;
    }

    rlEnd();
}

constexpr size_t sizeof_output_buffer = 4096;

int main() {
    int sample_size = 0;
    PaHostApiIndex host_api_index = 0;
    const PaHostApiInfo* host_api_info = 0;
    const PaDeviceInfo* input_device_info = 0;
    const PaDeviceInfo* output_device_info = 0;
    PaStream* input_stream = 0;
    PaStream* output_stream = 0;
    double average_output_stream_cpu_load = 0.0;
    size_t n_frames = 0;
    RenderTexture2D mixed_wave_render_texture;

    struct sine_wave_t {
        double frequency;
        double amplitude;
        double phase;
    };
    thread mixer_thread;
    mutex sine_waves_mutex;
    bool mixer_thread_is_running = false;
    vector<sine_wave_t> sine_waves;

    struct callback_data_t {
        PaStream* stream = 0;
        const PaDeviceInfo* device_info = 0;

        size_t output_mixed_buffer_write = 0;
        size_t output_mixed_buffer_read = 0;
        float output_mixed_buffer[sizeof_output_buffer] = { 0 };
    };
    callback_data_t output_callback_data;
    callback_data_t input_callback_data;

    PaStreamParameters input_stream_parameters = { 0 };
    PaStreamParameters output_stream_parameters = { 0 };

    LOG("initializing");
    Pa_Initialize();

    InitWindow(w_width, w_height, "The Window");
    SetTargetFPS(60);

    LOG("retrieving device information");
    auto version_info = Pa_GetVersionInfo();
    assert(version_info);
    cout << "Version info:" << endl;
    cout << "  version: " << version_info->versionMajor << "." << version_info->versionMinor << "." << version_info->versionSubMinor << endl;
    cout << "  control revision" << version_info->versionControlRevision << endl;
    cout << "  text: " << version_info->versionText << endl;
    sample_size = Pa_GetSampleSize(paFloat32);
    cout << "Sample size for 32-bit floats: " << sample_size << endl;
    host_api_index = Pa_GetDefaultHostApi();
    host_api_info = Pa_GetHostApiInfo(host_api_index);
    assert(host_api_info);
    cout << "Host API info:" << endl;
    cout << "  index: " << host_api_index << endl;
    cout << "  struct version: " << host_api_info->structVersion << endl;
    cout << "  type: " << host_api_info->type << endl;
    cout << "  name: " << host_api_info->name << endl;
    cout << "  device count: " << host_api_info->deviceCount << endl;
    cout << "  default input device index: " << host_api_info->defaultInputDevice << endl;
    cout << "  default output device index: " << host_api_info->defaultOutputDevice << endl;
    input_device_info = Pa_GetDeviceInfo(host_api_info->defaultInputDevice);
    assert(input_device_info);
    cout << "Input device info:" << endl;
    cout << "  struct version: " << input_device_info->structVersion << endl;
    cout << "  name: " << input_device_info->name << endl;
    cout << "  host API index: " << input_device_info->hostApi << endl;
    cout << "  max input channels: " << input_device_info->maxInputChannels << endl;
    cout << "  max output channels: " << input_device_info->maxOutputChannels << endl;
    cout << "  default low input latency [s]: " << input_device_info->defaultLowInputLatency << endl;
    cout << "  default low output latency [s]: " << input_device_info->defaultLowOutputLatency << endl;
    cout << "  default high input latency [s]: " << input_device_info->defaultHighInputLatency << endl;
    cout << "  default high output latency [s]: " << input_device_info->defaultHighOutputLatency << endl;
    cout << "  default sample rate: " << input_device_info->defaultSampleRate << endl;         
    output_device_info = Pa_GetDeviceInfo(host_api_info->defaultOutputDevice);
    assert(output_device_info);
    cout << "Output device info:" << endl;
    cout << "  struct version: " << output_device_info->structVersion << endl;
    cout << "  name: " << output_device_info->name << endl;
    cout << "  host API index: " << output_device_info->hostApi << endl;
    cout << "  max input channels: " << output_device_info->maxInputChannels << endl;
    cout << "  max output channels: " << output_device_info->maxOutputChannels << endl;
    cout << "  default low input latency [s]: " << output_device_info->defaultLowInputLatency << endl;
    cout << "  default low output latency [s]: " << output_device_info->defaultLowOutputLatency << endl;
    cout << "  default high input latency [s]: " << output_device_info->defaultHighInputLatency << endl;
    cout << "  default high output latency [s]: " << output_device_info->defaultHighOutputLatency << endl;
    cout << "  default sample rate: " << output_device_info->defaultSampleRate << endl;

    LOG("opening input stream");
    input_stream_parameters = {
        .device = host_api_info->defaultInputDevice,
        .channelCount = input_device_info->maxInputChannels,
        .sampleFormat = paFloat32,
        .suggestedLatency = input_device_info->defaultLowInputLatency,
        .hostApiSpecificStreamInfo = 0
    };
    input_callback_data.device_info = input_device_info;

    err = Pa_OpenStream(
        &input_stream,
        &input_stream_parameters,
        0,
        input_device_info->defaultSampleRate,
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
            assert(user_data);
            callback_data_t* callback_data = (callback_data_t*) user_data;

            // cout << "Sup from input stream callback" << endl;

            // cout << "  CPU load to maintain real-time operations [%]: " << Pa_GetStreamCpuLoad(callback_data->stream) * 100.0 << endl;

            // cout << "  Input: " << input << endl;
            // cout << "  Output: " << output << endl;

            // cout << "  Frame count: " << frame_count << endl;

            // cout << "  Time of invocation: " << time_info->currentTime << endl;
            // cout << "  Time of first ADC sample: " << time_info->inputBufferAdcTime << endl;
            // cout << "  Time of first DAC sample: " << time_info->outputBufferDacTime << endl;

            // cout << "  Status flags:" << endl;
            // cout << "    underflow: ";
            // if (paInputUnderflow & status_flags) {
            //     cout << "input data all zeros, no available data in buffer";
            // } else {
            //     cout << "there is some available data in buffer";
            // }
            // cout << endl;

            // cout << "    overflow: ";
            // if (paInputOverflow & status_flags) {
            //     cout << "some data prior to the first sample was discarded (possibly callback using too much CPU time)";
            // } else {
            //     cout << "data prior to the first sample was not discard (looks good)";
            // }
            // cout << endl;

            // cout << "  User_data: " << user_data << endl;

            // paAbort - stream will finish asap
            // paComplete - stream will finish once all buffers generated are played
            // these are not necessary tho, as Pa_StopStream(), Pa_AbortStream() or Pa_CloseStream() can also be used

            return paContinue;
        },
        (void*) &input_callback_data
    );
    if (err != paNoError) {
        goto error;
    }
    input_callback_data.stream = input_stream;

    LOG("opening output stream");
    output_stream_parameters = {
        .device = host_api_info->defaultOutputDevice,
        .channelCount = output_device_info->maxOutputChannels,
        .sampleFormat = paFloat32,
        .suggestedLatency = output_device_info->defaultLowOutputLatency,
        .hostApiSpecificStreamInfo = 0
    };
    output_callback_data.device_info = output_device_info;
    err = Pa_OpenStream(
        &output_stream,
        0,
        &output_stream_parameters,
        output_device_info->defaultSampleRate,
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
            assert(user_data);
            callback_data_t* callback_data = (callback_data_t*) user_data;

            // cout << "Sup from output stream callback" << endl;

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
                size_t write_p = callback_data->output_mixed_buffer_write;
                size_t read_p = callback_data->output_mixed_buffer_read;
                if (read_p <= write_p) {
                    available_to_read = write_p - read_p;
                } else {
                    available_to_read = (sizeof_output_buffer + write_p - read_p) % sizeof_output_buffer;
                }
                // LOG("[CALLBACK] before, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
                bool write_zeros = available_to_read < frame_count;

                assert(output);
                float* buffer = (float*) output;
                // interleaved data: 1st sample: [channel0 channel1 ..], 2nd sample: [channel0 channel1 ..], ...
                for (size_t i = 0; i < frame_count; ++i) {
                    float read_val = 0.0f;
                    if (!write_zeros) {
                        read_val = callback_data->output_mixed_buffer[callback_data->output_mixed_buffer_read++];
                    }
                    if (sizeof_output_buffer <= callback_data->output_mixed_buffer_read) {
                        callback_data->output_mixed_buffer_read = 0;
                    }
                    for (size_t channel_index = 0; channel_index < callback_data->device_info->maxOutputChannels; ++channel_index) {
                        *buffer++ = read_val;
                    }
                }

                // write_p = callback_data->output_mixed_buffer_write;
                // read_p = callback_data->output_mixed_buffer_read;
                // if (read_p <= write_p) {
                //     available_to_read = write_p - read_p;
                // } else {
                //     available_to_read = (sizeof_output_buffer + write_p - read_p) % sizeof_output_buffer;
                // }
                // LOG("[CALLBACK] after, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
            }

            return paContinue;
        },
        (void*) &output_callback_data
    );
    if (err != paNoError) {
        goto error;
    }
    output_callback_data.stream = output_stream;

    // LOG("Starting input stream");
    // if ((err = Pa_StartStream(input_stream)) != paNoError) {
    //     goto error;
    // }

    mixer_thread_is_running = true;
    {
        scoped_lock<mutex> lock(sine_waves_mutex);

        for (int i = 0; i < 10; ++i) {
            sine_waves.push_back({ 50 * ((double) i * 2.0 + 1.0), 1.0 / ((double) i * 2.0 + 1.0), 0.0 });
        }
    }
    mixer_thread = thread([&]() {
        double t = 0.0;
        const double time_step = 1.0f / output_callback_data.device_info->defaultSampleRate;

        while (mixer_thread_is_running) {
            size_t available_to_write = 0;
            size_t write_p = output_callback_data.output_mixed_buffer_write;
            size_t read_p = output_callback_data.output_mixed_buffer_read;
            if (read_p <= write_p) {
                available_to_write = (sizeof_output_buffer - (write_p - read_p)) - 1;
            } else {
                available_to_write = (read_p - write_p) - 1;
            }
            // LOG("[MIXER] before, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);

            if (!available_to_write) {
                Pa_Sleep(1);
                continue ;
            }

            {
                scoped_lock<mutex> sine_waves_mutex_lock_guard(sine_waves_mutex);

                // first pass to get informations on the resulting wave
                constexpr float desired_min_mixed_value = -1.0f;
                constexpr float desired_max_mixed_value = 1.0;
                float min_mixed_value = FLT_MAX;
                float max_mixed_value = -FLT_MAX;
                double tmp_t = t;
                for (size_t i = 0; i < available_to_write; ++i) {
                    float mixed_value = 0.0f;
                    for (size_t sine_wave_index = 0; sine_wave_index < sine_waves.size(); ++sine_wave_index) {
                        mixed_value += (float) (sine_waves[sine_wave_index].amplitude * sin(2.0 * PI * sine_waves[sine_wave_index].frequency * tmp_t + sine_waves[sine_wave_index].phase));
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
                    for (size_t sine_wave_index = 0; sine_wave_index < sine_waves.size(); ++sine_wave_index) {
                        mixed_value += (float) (sine_waves[sine_wave_index].amplitude * sin(2.0 * PI * sine_waves[sine_wave_index].frequency * t + sine_waves[sine_wave_index].phase));
                    }
                    output_callback_data.output_mixed_buffer[output_callback_data.output_mixed_buffer_write++] = mixed_value * mixed_value_multiplier;
                    if (sizeof_output_buffer <= output_callback_data.output_mixed_buffer_write) {
                        output_callback_data.output_mixed_buffer_write = 0;
                    }
                    t += time_step;
                }
            }

            write_p = output_callback_data.output_mixed_buffer_write;
            read_p = output_callback_data.output_mixed_buffer_read;
            if (read_p <= write_p) {
                available_to_write = (sizeof_output_buffer - (write_p - read_p)) - 1;
            } else {
                available_to_write = (read_p - write_p) - 1;
            }
            // LOG("[MIXER] after, acquire, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);
        }
    });

    LOG("Starting output stream");
    if ((err = Pa_StartStream(output_stream)) != paNoError) {
        goto error;
    }

    mixed_wave_render_texture = LoadRenderTexture(w_width, w_height);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        double output_stream_cpu_load = Pa_GetStreamCpuLoad(output_callback_data.stream);
        if (0.1 < output_stream_cpu_load) {
            LOG("high output stream CPU load [%]: " << output_stream_cpu_load * 100.0);
        }
        average_output_stream_cpu_load = ((average_output_stream_cpu_load * (double) (n_frames % 45)) + output_stream_cpu_load) / (double) ((n_frames % 45) + 1);

        if (IsFileDropped()) {
            FilePathList file_path_list = LoadDroppedFiles();

            UnloadDroppedFiles(file_path_list);
        }

        if (IsKeyDown(KEY_LEFT)) {
            sine_waves[0].phase -= 0.01f;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            sine_waves[0].phase += 0.01f;
        }
        if (IsKeyDown(KEY_UP)) {
            sine_waves[0].frequency += 1.0f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            sine_waves[0].frequency -= 1.0f;
        }

        if (n_frames % 1 == 0) {
            BeginTextureMode(mixed_wave_render_texture);
            ClearBackground(LIGHTGRAY);

            if (output_callback_data.stream) {
                LOG("average output stream CPU load [%]: " << average_output_stream_cpu_load * 100.0);
                average_output_stream_cpu_load = 0.0;
            }

            // Vector2 points[sizeof_output_buffer];
            // for (size_t i = 0; i < sizeof_output_buffer; ++i) {
            //     points[i].x = (float) i / (float) sizeof_output_buffer * (float) w_width + phase_offset;
            //     if ((float) w_width <= points[i].x) {
            //         points[i].x -= w_width;
            //     }
            //     float min_y = -1.0f;
            //     float max_y = 1.0f;
            //     float y = output_callback_data.output_mixed_buffer[i];
            //     points[i].y = (y - min_y) / (max_y - min_y) * w_height;
            // }
            // DrawLines(points, sizeof_output_buffer, RED);

            Vector2 points[1000];
            size_t to_advance = (size_t) (output_device_info->defaultSampleRate / 60.0f);
            assert(to_advance < 1000);
            static size_t read_p = 0;
            for (size_t i = 0; i < to_advance; ++i) {
                read_p %= sizeof_output_buffer;

                points[i].x = (float) i / (float) to_advance * (float) w_width;
                if ((float) w_width <= points[i].x) {
                    points[i].x -= w_width;
                }
                float min_y = -1.0f;
                float max_y = 1.0f;
                float y = output_callback_data.output_mixed_buffer[read_p++];
                points[i].y = (y - min_y) / (max_y - min_y) * w_height;
            }
            DrawLines(points, to_advance, RED);

            // for (size_t i = 0; i < sizeof_output_buffer; ++i) {
            //     int screen_x = (int) ((float) i / ((float) sizeof_output_buffer) * (float) w_width);
            //     float min_y = -1.0f;
            //     float max_y = 1.0f;
            //     float y = output_callback_data.output_mixed_buffer[i];
            //     int screen_y = (int) ((y - min_y) / (max_y - min_y) * w_height);

            //     DrawCircle(screen_x, screen_y, 2.0f, RED);
            // }
            // float read_x = (float) output_callback_data.output_mixed_buffer_read / (float) sizeof_output_buffer * (float) w_width;
            // float write_x = (float) output_callback_data.output_mixed_buffer_write / (float) sizeof_output_buffer * (float) w_width;
            // DrawLineEx({ read_x, 0.0f }, { read_x, (float) w_height }, 5.0f, { 0, 255, 0, 100 });
            // DrawLineEx({ write_x, 0.0f }, { write_x, (float) w_height }, 5.0f, { 255, 255, 0, 100 });

            DrawFPS(5, 5);

            EndTextureMode();
        }

        DrawTexturePro(
            mixed_wave_render_texture.texture,
            { 0.0f, 0.0f, (float) mixed_wave_render_texture.texture.width, (float) -mixed_wave_render_texture.texture.height },
            { 0.0f, 0.0f, (float) w_width, (float) w_height },
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );

        EndDrawing();
        ++n_frames;
    }

    goto end;

error:
    if (err == paUnanticipatedHostError) {
        const PaHostErrorInfo* host_error_info = Pa_GetLastHostErrorInfo();
        cerr << "Host error occured:" << endl;
        cerr << "  host API type: " << host_error_info->hostApiType << endl;
        cerr << "  error code: " << host_error_info->errorCode << endl;
        cerr << "  text: " << host_error_info->errorText << endl;
    } else {
        cerr << "Error occured: " << Pa_GetErrorText(err) << " (" << err << ")" << endl;
    }
    goto end;

end:
    if (input_stream) {
        LOG("closing input stream");
        PaStream* tmp_input_stream = input_stream;
        input_stream = 0;
        if ((err = Pa_CloseStream(tmp_input_stream)) != paNoError) {
            goto error;
        }
    }

    if (output_stream) {
        LOG("closing output stream");
        PaStream* tmp_output_stream = output_stream;
        output_stream = 0;
        if ((err = Pa_CloseStream(tmp_output_stream)) != paNoError) {
            goto error;
        }
    }

    LOG("terminating");

    mixer_thread_is_running = false;
    mixer_thread.join();

    CloseWindow();
    Pa_Terminate();

    return 0;
}
