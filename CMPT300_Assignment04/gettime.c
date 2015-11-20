#include "gettime.h"

void gettime(char *time_str) {
    time_t raw_time;

    memset(time_str, 0, 30);
    time(&raw_time);
    strcpy(time_str, ctime(&raw_time));
    time_str[strlen(time_str) - 1] = '\0';
}
