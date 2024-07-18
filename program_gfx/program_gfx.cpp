#include "program_gfx.h"

#include "raylib.h"
#include "program_gfx_impl.cpp"

namespace program {

program_visualizer_t::program_visualizer_t():
    base_initializer_t<program_visualizer_t>(
        {
            process({
                process({
                    file_modified(
                        { g_oscillator_200ms },
                        "libglm.a"
                    )
                }),
                lambda(
                    {
                        file_modified(
                            { g_oscillator_200ms },
                            "libraylib.a"
                        )
                    },
                    [](offset_ptr_t<base_t> base) {
                        // base->set_start(get_time());

                        std::cout << "Sup from lambda yoo" << std::endl;
                        while (1) {
                        }

                        base->set_success(get_time());
                        // base->set_finish(get_time());
                    }
                )
            })
            // file_modified(
            //     { g_oscillator_200ms },
            //     "libraylib.a"
            // ),
            // file_modified(
            //     { g_oscillator_200ms },
            //     "raylib.h"
            // ),
            // file_modified(
            //     { g_oscillator_200ms },
            //     "program_gfx/program_gfx.cpp"
            // ),
            // todo: add gfx backend as input
        },
        [](offset_ptr_t<base_t> base, time_type_t time_propagate) {
            (void) time_propagate;
            base->set_start(get_time());

            ::init(base);

            double t_prev = GetTime();
            while (!WindowShouldClose()) {
                double t_cur = GetTime();
                double t_delta = t_cur - t_prev;
                ::update(t_delta);
                ::draw();
            }

            ::destroy();

            base->set_success(get_time());
            base->set_finish(get_time());

            return 0;
        },
        [](offset_ptr_t<base_t> base) -> std::string {
            (void) base;
            return "program visualizer";
        }        
    ) {
}
offset_ptr_t<program_visualizer_t> program_visualizer() {
    /*
        describe dependencies for this program
    */
    return malloc_program<program_visualizer_t>();
}

} // namespace program
