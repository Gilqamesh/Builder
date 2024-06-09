#include "sem.h"
#include "msg.h"

#include <stdio.h>
#include <unistd.h>

struct sem sem;
struct msg_queue msg;

static void fn();
static void fn2();
static void fn3();

static void fn() {
    printf("fn: hello\n");
    sem__lock(&sem);
    printf("fn: sem's value: %d\n", sem__val(&sem));
    printf("fn: fn2()\n");
    fn2();
    sem__unlock(&sem);
}

static void fn2() {
    printf("fn2: hello\n");
    sem__lock(&sem);
    printf("fn2: sem's value: %d\n", sem__val(&sem));
    printf("fn2: fn3()\n");
    fn3();
    sem__unlock(&sem);
}

static void fn3() {
    printf("fn3: hello\n");
    sem__lock(&sem);
    printf("fn3: sem's value: %d\n", sem__val(&sem));
    printf("fn3: doing stuff...\n");
    sem__unlock(&sem);
}

int main() {
    if (sem__create(&sem, 1)) {
        return 1;
    }
    if (msg_queue__create(&msg, 2)) {
        sem__destroy(&sem);
        return 1;
    }

    printf("main: fn()\n");
    fn();
    printf("main: sem's value: %d\n", sem__val(&sem));

    printf("main: fn2()\n");
    fn2();
    printf("main: sem's value: %d\n", sem__val(&sem));

    printf("main: fn3()\n");
    fn3();
    printf("main: sem's value: %d\n", sem__val(&sem));

    sem__destroy(&sem);

    msg_queue__print(&msg);

    msg_queue__write(&msg, "sup", 2);
    msg_queue__write(&msg, "sup", 3);
    msg_queue__write(&msg, "sup", 4);
    msg_queue__write(&msg, "sup", 5);
    msg_queue__write(&msg, "sup", 6);
    msg_queue__write(&msg, "sup", 2000);
    msg_queue__write(&msg, "sup", 8);

    msg_queue__print(&msg);

    char buffer[1024];
    while (msg_queue__read(&msg, buffer, sizeof(buffer))) {
        printf("%s\n", buffer);
    }

    msg_queue__print(&msg);

    msg_queue__destroy(&msg);

    return 0;
}
