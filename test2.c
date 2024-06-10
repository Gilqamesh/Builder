#include "raylib.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

typedef enum orientation {
    VERTICAL,
    HORIZONTAL
} orientation_t;

typedef struct node {
    orientation_t orientation;
    Rectangle rec;
    struct node* left; //
    struct node* right;
} node_t;

static node_t* node;
static int n = 1;
static int width = 1600;
static int height = 1000;

static void update();
static void draw();

void node__destroy(node_t* self) {
    if (self->left) {
        node__destroy(self->left);
    }
    if (self->right) {
        node__destroy(self->right);
    }
    free(self);
}

node_t* node__create_internal(Rectangle rec, int n, orientation_t orientation) {
    node_t* result = calloc(1, sizeof(*result));

    result->rec = rec;
    result->orientation = orientation;
    orientation_t orientation_next = orientation == HORIZONTAL ? VERTICAL : HORIZONTAL;

    if (1 < n) {
        int right = n >> 1;
        int left = n - right;

        result->left = node__create_internal((Rectangle) {
            .x = rec.x,
            .y = rec.y,
            .width = orientation == HORIZONTAL ? rec.width / 2 : rec.width,
            .height = orientation == VERTICAL ? rec.height / 2 : rec.height
        }, left, orientation_next);

        result->right = node__create_internal((Rectangle) {
            .x = rec.x + (orientation == HORIZONTAL ? rec.width / 2 : 0),
            .y = rec.y + (orientation == VERTICAL ? rec.height / 2 : 0),
            .width = orientation == HORIZONTAL ? rec.width / 2 : rec.width,
            .height = orientation == VERTICAL ? rec.height / 2 : rec.height
        }, right, orientation_next);
    }

    return result;
}

node_t* node__create(Rectangle rec, int n) {
    return node__create_internal(rec, n, VERTICAL);
}

void draw_node_internal(node_t* self, int depth) {
    if (self->left) {
        assert(self->right);
        draw_node_internal(self->left, depth);
        draw_node_internal(self->right, depth);
    } else {
        DrawRectangleRec(
            (Rectangle) {
                .x = self->rec.x + self->rec.width * 0.2,
                .y = self->rec.y + self->rec.height * 0.2,
                .width = self->rec.width * 0.6,
                .height = self->rec.height * 0.6
            },
            (Color) {
                .a = 160,
                .r = 100,
                .g = 60,
                .b = 10
            }
        );
    }
}

void draw__node(node_t* self) {
    Color node_color = {
        .a = 200,
        .r = 0,
        .g = 255,
        .b = 0
    };
    DrawRectangleRec((Rectangle) {
        self->rec.x,
        height * 0.1,
        self->rec.width,
        height * 0.2
    }, node_color);
    DrawText("Node", self->rec.x + self->rec.width / 2 - 20, height * 0.2, 60.0f, RED);
    DrawRectangleLinesEx(self->rec, 1.0f, node_color);
    draw_node_internal(self, 0);
}

static void update() {
    int n_prev = n;
    if (IsKeyDown(KEY_A)) {
        if (1 < n) {
            --n;
        }
    }
    if (IsKeyDown(KEY_D)) {
        ++n;
    }

    if (!node || n_prev != n) {
        if (node) {
            node__destroy(node);
        }
        node = node__create((Rectangle) {
            .x = width * 0.1,
            .y = height * 0.3,
            .width = width * 0.8,
            .height = height * 0.6
        }, n);
    }
}

static void draw() {
    draw__node(node);
}

int main() {
    InitWindow(width, height, "Rectangles");

    SetTargetFPS(30);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        update();
        draw();

        EndDrawing();
    }

    CloseWindow();
}
