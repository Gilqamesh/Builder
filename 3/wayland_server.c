#include "wayland-server.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

static void wayland_logger(
    void *user_data,
    enum wl_protocol_logger_type direction,
    const struct wl_protocol_logger_message *message
) {
    (void) user_data;
    switch (direction) {
    case WL_PROTOCOL_LOGGER_REQUEST: {
    } break ;
    case WL_PROTOCOL_LOGGER_EVENT: {
    } break;
    default: assert(0);
    }

    assert(message);
    printf("[Logger]");
    printf(" opcode: %d", message->message_opcode);
    printf(" message: %s signature: %s", message->message->name, message->message->signature);
    for (int argument = 0; argument < message->arguments_count; ++argument) {
        printf(" argument %d: %s", argument, message->arguments[argument].s);
    }

    printf("\n");
}

int main(int argc, char** argv) {
    struct wl_display* display = 0;
    struct wl_protocol_logger* logger = 0;

    if (argc != 2) {
        printf("Usage <%s> <display_name>\n", argv[0]);
        goto fatal;
    }

    const char* display_name = argv[1];

    printf("Creating wayland server..\n");
    display = wl_display_create();
    if (!display) {
        printf("wl_display_create failed\n");
        return 1;
    }
    printf("Created wayland server!\n");

    printf("Adding wayland logger..\n");
    logger = wl_display_add_protocol_logger(display, &wayland_logger, 0);
    if (!logger) {
        printf("wl_display_add_protocol_logger failed\n");
        goto fatal;
    }
    printf("Added wayland logger!\n");

    printf("Adding socket '%s'..\n", display_name);
    if (wl_display_add_socket(display, display_name)) {
        printf("wl_display_add_socket failed!\n");
        goto fatal;
    }
    printf("Added socket '%s'", display_name);

    printf("Running wayland server..\n");
    wl_display_run(display);
    printf("Ran wayland server!\n");

    printf("Destroying logger..\n");
    wl_protocol_logger_destroy(logger);
    printf("Destroyed logger!\n");

    printf("Destroying wayland server..\n");
    wl_display_destroy(display);
    printf("Destroyed wayland server!\n");

    return 0;

fatal:
    if (logger) {
        wl_protocol_logger_destroy(logger);
    }
    if (display) {
        wl_display_destroy(display);
    }
    return 1;
}
