#include "readline.h"
void readline(FILE *fp, char *str) {
    char c;

    // argument check
    if (fp == NULL) {
        printf("readline FILE not open\n");
        exit(1);
    }
    if (str == NULL) {
        printf("readline str memory not allocated\n");
        exit(1);
    }

    // readline
    while (!feof(fp)) {
        c = fgetc(fp);
        if (c == '\n')  // line break
            break;
        str[strlen(str)] = c;
    }
    return;
}
