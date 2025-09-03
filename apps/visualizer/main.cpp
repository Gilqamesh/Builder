#include "builder.h"
#include "raylib.h"

int main() {
    InitWindow(800, 600, "The Window");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
