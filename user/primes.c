//
// Created by liuluhao on 2022/09/26.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p_0[2];
    pipe(p_0);
    if (fork() == 0) {
        // if child
        close(p_0[1]);

        int inpipe, outpipe;
        inpipe = p_0[0];
        while (1) {
            char numc[4];
            if (read(inpipe, numc, 4) == 0) {
                break;
            }
//            printf("%d was read.\n", *((int *)numc));
            int tmp = *((int *) numc);
            printf("prime %d\n", tmp);
            int p_1[2];
            pipe(p_1);
            if (fork() == 0) {
                close(p_1[1]);
                close(inpipe);
                inpipe = p_1[0];
            } else {
                close(p_1[0]);
                outpipe = p_1[1];
                while (read(inpipe, numc, 4) != 0) {
                    if ((*((int *) numc)) % tmp != 0) {
                        write(outpipe, numc, 4);
                    }
                }
                close(p_0[0]);
                close(p_1[1]);
                wait((int *) 0);
                exit(0);
            }
        }
        exit(0);
    } else {
        // if parent
        close(p_0[0]);

        int i;
        for (i = 2; i <= 35; i++) {
//            printf("%d from Ancestor.\n", i);
            write(p_0[1], (char *) (&i), 4);
        }
        close(p_0[1]);

        wait((int *) 0);
        exit(0);
    }
}