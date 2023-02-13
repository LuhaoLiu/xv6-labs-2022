//
// Created by liuluhao on 2022/09/26.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//int contain(const char *str, const char *key) {
//    int i;
//    for (i = 0; i + strlen(key) <= strlen(str); i++) {
//        if (memcmp(str + i, key, strlen(key)) == 0) {
//            return 1;
//        }
//    }
//    return 0;
//}

char* fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p = path;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ) {
        return p;
    }
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void find(char *path, char *key) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
//        printf("debug: %d\n", strlen(path));
        return;
    }

    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
        case T_FILE:
//            if (contain(path, key)) {
//                printf("%s\n", fmtname(path));
//            }
            if (strcmp(path, key) == 0) {
                printf("%s\n", fmtname(path));
            }
            break;

        case T_DIR:
//            if (contain(path, key)) {
//                printf("%s\n", fmtname(path));
//            }
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) {
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0) {
                    printf("find : cannot stat %s\n", buf);
                    continue;
                }
                if (st.type == T_FILE && strcmp(de.name, key) == 0) {
                    printf("%s\n", fmtname(buf));
                }
                if (strcmp(".", de.name) != 0 && strcmp("..", de.name) != 0 && st.type == T_DIR) {
                    find(buf, key);
                }
            }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(2, "find: invalid argument(s)\n");
        exit(1);
    } else {
        find(argv[1], argv[2]);
        exit(0);
    }
}