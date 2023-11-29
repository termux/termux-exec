#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define TERMUX_APT_PATH "/data/data/com.termux/files/usr/bin/apt"

extern char **environ;

int main() {
    if (fork() == 0) {
        char *const argv[] = { "apt", "--version", NULL };
        printf("# execve\n");
        execve(TERMUX_APT_PATH, argv, environ);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        printf("# execl\n");
        execl(TERMUX_APT_PATH, "apt", "--version", NULL);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        printf("# execlp\n");
        execlp("apt", "apt", "--version", NULL);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        printf("# execle\n");
        execle(TERMUX_APT_PATH, "apt", "--version", NULL, environ);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        char *const argv[] = { "apt", "--version", NULL };
        printf("# execv\n");
        execv(TERMUX_APT_PATH, argv);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        char *const argv[] = { "apt", "--version", NULL };
        printf("# execvp\n");
        execvp("apt", argv);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        char *const argv[] = { "apt", "--version", NULL };
        printf("# execvpe\n");
        execvpe("apt", argv, environ);
        return 0;
    }

    sleep(1);
    if (fork() == 0) {
        char *const argv[] = { "apt", "--version", NULL };
        int fd = open(TERMUX_APT_PATH, 0);
        printf("# fexecve\n");
        fexecve(fd, argv, environ);
        return 0;
    }

    sleep(1);

    return 0;
}
