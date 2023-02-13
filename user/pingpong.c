//
// Created by liuluhao on 2022/09/26.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p_0[2], p_1[2];
    pipe(p_0); // from parent to child
    pipe(p_1); // from child to parent

    if (fork() == 0) {
        // if child
        close(p_0[1]);
        close(p_1[0]);

        read(p_0[0], (char *) 0, 1);
        printf("%d: received ping\n", getpid());
        close(p_0[0]);

        write(p_1[1], " ", 1);
        close(p_1[1]);

        exit(0);
    } else {
        // if parent
        close(p_0[0]);
        close(p_1[1]);

        write(p_0[1], " ", 1);
        close(p_0[1]);

        read(p_1[0], (char *) 0, 1);
        printf("%d: received pong\n", getpid());
        close(p_1[0]);

        exit(0);
    }
}