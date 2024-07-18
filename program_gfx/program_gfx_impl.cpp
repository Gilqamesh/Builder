#include <glm/glm.hpp>

#include <boost/process.hpp>

static void init(offset_ptr_t<program::base_t> base);
static void update(double time_delta);
static void draw();
static void destroy();

static void camera_reset();

struct {
    offset_ptr_t<program::base_t> base;
    int width;
    int height;
    int target_fps;
    float min_zoom;
    float max_zoom;
    float movement_speed;
    Camera2D camera;
    Font font;
    RenderTexture2D scene_render_texture;
    RenderTexture2D overlay_render_texture;
} _;

static void init(offset_ptr_t<program::base_t> base) {
    _.base = base;
    _.width = 1600;
    _.height = 1200;
    _.target_fps = 60;
    _.min_zoom = 0.1f;
    _.max_zoom = 20.0f;
    _.movement_speed = 100.0f;
    camera_reset();

    InitWindow(_.width, _.height, ("Program Visualizer [" + std::to_string(boost::this_process::get_id()) + "]").c_str());
    SetTargetFPS(_.target_fps);

    _.font = LoadFont("program_gfx/assets/LiberationMono-Regular.ttf");
    _.scene_render_texture = LoadRenderTexture(_.width, _.height);
    _.overlay_render_texture = LoadRenderTexture(_.width, _.height);
}

static void update(double time_delta) {
    float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel > 0.0f) {
        _.camera.zoom = 1.15f * _.camera.zoom;
    }
    else if (mouse_wheel < 0.0f) {
        _.camera.zoom = 0.9f * _.camera.zoom;
    }

    if (_.camera.zoom < _.min_zoom) {
        _.camera.zoom = _.min_zoom;
    }
    if (_.camera.zoom > _.max_zoom) {
        _.camera.zoom = _.max_zoom;
    }

    const float relative_zoom = _.camera.zoom / _.max_zoom;
    const float speed = _.movement_speed * time_delta / relative_zoom;
    if (IsKeyDown(KEY_D)) {
        _.camera.target.x += speed;
    }
    if (IsKeyDown(KEY_A)) {
        _.camera.target.x -= speed;
    }
    if (IsKeyDown(KEY_W)) {
        _.camera.target.y -= speed;
    }
    if (IsKeyDown(KEY_S)) {
        _.camera.target.y += speed;
    }
}

static void draw() {
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();
}

static void destroy() {
    CloseWindow();
}

static void camera_reset() {
    memset(&_.camera, 0, sizeof(_.camera));

    _.camera.zoom = 1.0f;
    _.camera.offset.x = _.width / 2.0f;
    _.camera.offset.y = _.height / 2.0f;
}
