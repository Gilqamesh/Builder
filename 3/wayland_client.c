#include "wayland-client.h"

#include <stdio.h>

int main(int argc, char** argv) {
    struct wl_display* display = 0;

    if (argc != 2) {
        printf("Usage: <%s> <display_name>\n", argv[0]);
        goto fatal;
    }

    const char* display_name = argv[1];
    printf("Connecting to wayland display '%s'..\n", display_name);
    display = wl_display_connect(display_name);
    if (!display) {
        printf("wl_display_connect failed to connect to '%s'!\n", display_name);
        goto fatal;
    }
    printf("Connected to wayland display '%s'! FD: %d\n", display_name, wl_display_get_fd(display));

    printf("Dispatching display..\n");
    if (wl_display_dispatch(display)) {
        printf("wl_display_dispatch failed!\n");
        goto fatal;
    }
    printf("Dispatched display!\n");

    printf("Disconnecting from wayland display '%s'..\n", display_name);
    wl_display_disconnect(display);
    printf("Disconnected from wayland display '%s'!\n", display_name);

    return 0;

fatal:
    if (display) {
        wl_display_disconnect(display);
    }
    return 1;
}
