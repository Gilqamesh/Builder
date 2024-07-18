#ifndef PROGRAM_GFX_H
# define PROGRAM_GFX_H

# include "program.h"

namespace program {

struct program_visualizer_t : public base_initializer_t<program_visualizer_t> {
    program_visualizer_t();
};
offset_ptr_t<program_visualizer_t> program_visualizer();

} // namespace program

#endif // PROGRAM_GFX_H
