//
// Created by liuluhao on 2022/09/26.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    char buf[512];
    memset(buf, '\0', sizeof(buf));
    int t = 0;
    while (1) {
        int tmpt = read(0, buf + t, sizeof(buf) - t);
        t += tmpt;
        if (tmpt == 0 || t >= sizeof(buf)) break;
//        printf("%d ", t);
    }
    char *nargv[MAXARG];
    int i;
    nargv[0] = argv[1];
    for (i = 2; i < argc; i++) {
        nargv[i - 1] = argv[i];
    }
    char *l = buf, *r = buf;
    int initi = i;
    while (r - buf <= sizeof(buf) && *r != '\0') {
        while (r - buf <= sizeof(buf) && *r != '\n') {
            if (*r == ' ') {
                nargv[i - 1] = malloc(r - l + 1);
                memcpy(nargv[i - 1], l, r - l);
//            printf("%s\n", nargv[i - 1]);
                i++;
                l = r + 1;
            }
            do {
                r++;
            } while (*r != ' ' && *r != '\n' && *r != '\0');
        }
        nargv[i - 1] = malloc(r - l + 1);
        memcpy(nargv[i - 1], l, r - l);
        nargv[i] = 0;
        if (fork() == 0) {
//        printf("%s\n", nargv[2]);
            exec(nargv[0], nargv);
        } else {
            wait((int *) 0);
//            for(; i > initi; i--) free(nargv[i - 1]); // not working
            i = initi;
        }
        r++;
        l = r;
    }
    exit(0);
}