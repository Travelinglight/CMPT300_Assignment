#include "gettime.h"

/*------------------------------------------------- gettime ----------
 *|  Function gettime
 *|  Purpose: get current time and format it into the given string
 *|  Parameters: 
 *|         char *str: the string where the formatted time is to be
 *|                 stored.
 *|  Returns:  none
 **-------------------------------------------------------------------*/
void gettime(char *time_str) {
    time_t raw_time;

    memset(time_str, 0, 30);
    time(&raw_time);
    strcpy(time_str, ctime(&raw_time));
    time_str[strlen(time_str) - 1] = '\0';
}
