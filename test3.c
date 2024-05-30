#include <unistd.h>
#include <time.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdio.h>
#include <wait.h>
#include <assert.h>
#include <stdlib.h>

void* thread_routine(void* data) {
    (void) data;

    printf("[%d] Starting child process\n", getpid());
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        printf("[%d] Starting work...\n", getpid());
        for (int i = 0; i < 10; ++i) {
            printf("[%d] %d\n", getpid(), i);
            sleep(1);
        }
        printf("[%d] Finished work!\n", getpid());
        exit(0);
    }
    pid_t waited_pid = waitpid(pid, 0, 0);
    if (waited_pid == -1) {
        perror("waitpid");
        exit(2);
    }
    if (waited_pid == pid) {
        printf("[%d] Successfully waited child\n", getpid());
    } else {
        assert(0 && "child is still running");
    }

    return 0;
}

int main() {
    printf("Creating worker thread\n");

    pthread_t thread;
    if (pthread_create(&thread, 0, &thread_routine, 0)) {
        perror("pthread_create");
        exit(1);
    }

    printf("Created worker thread, starting work in main thread\n");

    for (int i = 0; i < 4; ++i) {
        printf("Doing other work in main thread: %d..\n", i);
        sleep(1);
    }

    printf("Waiting for working thread\n");
    if (pthread_join(thread, 0)) {
        perror("pthread_join");
        exit(2);
    }
    printf("Finished waiting for working thread\n");

    return 0;
}
