#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * printf output sits in a user-space buffer until it is flushed, and fork
 * copies that buffer into the child. When stdout is a pipe or a file the
 * buffer is only flushed when full or at exit, so anything printed before
 * fork would come out twice. Every fork below is preceded by fflush(stdout).
 */

static pid_t checked_fork(void) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    return pid;
}

static void task1(void) {
    printf("TASK 1\n");
    int x = 25;
    pid_t pid = checked_fork();
    if (pid == 0) {
        x += 10;
        printf("child pid: %d, x: %d\n", getpid(), x);
        exit(0);
    }
    x -= 5;
    printf("parent pid: %d, x: %d\n", getpid(), x);
    if (waitpid(pid, NULL, 0) < 0)
        perror("waitpid");
}

static void task2(void) {
    printf("TASK 2\n");
    printf("parent pid: %d (compare with newfile.txt)\n", getpid());
    pid_t pid = checked_fork();
    if (pid == 0) {
        char *args[] = {"./writer", NULL};
        execvp(args[0], args);
        /* exec only returns if it failed */
        perror("execvp ./writer");
        _exit(1);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0)
        perror("waitpid");
    else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        printf("writer failed\n");
}

static void task3(void) {
    printf("TASK 3\n");
    pid_t pid = checked_fork();
    if (pid == 0) {
        sleep(2); /* long enough for the parent to have exited */
        printf("child pid: %d, parent pid: %d\n", getpid(), getppid());
        exit(0);
    }
    printf("parent pid: %d\n", getpid());
    /* no wait: main returns and the parent exits before the child */
}

int main(void) {
    task1();
    task2();
    task3();
    return 0;
}
