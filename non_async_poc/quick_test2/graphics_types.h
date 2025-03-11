#ifndef GRAPHICS_TYPES_H
# define GRAPHICS_TYPES_H

# include "instance.h"

# include <cassert>

void init_graphics_module();
void deinit_graphics_module();

struct framebuffer_t {
    uint32_t* data = 0;
    size_t width = 0;
    size_t height = 0;
    inline void put_pixel(size_t x, size_t y, int r, int g, int b, int a) {
        assert(x < width && y < height);
        data[y * width + x] = ((r << 24) & 0xff000000) | ((g << 16) & 0x00ff0000) | ((b << 8) & 0x0000ff00) | (a & 0x000000ff);
    }
};

// inherit from this if the program wants to be drawn
extern type_t* drawable_type;

struct graphics_engine_t : public instance_t {
    graphics_engine_t();
};

#endif // GRAPHICS_TYPES_H
