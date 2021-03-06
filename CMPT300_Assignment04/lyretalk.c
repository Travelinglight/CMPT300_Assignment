#include "lyretalk.h"

#define MAX_BUFF 2500

/*------------------------------------------------- lyrespeak --------
 *|  Function lyrespeak
 *|  Purpose:  send content through socket
 *|  Parameters:
 *|         int skt: the socket in connection
 *|         char *content: the content one wants to send.
.*|
 *|  Returns:  int: state, 0 for success and else for failure
 **-------------------------------------------------------------------*/
int lyrespeak(int skt, char *content) {
    char buff[MAX_BUFF];
    int bytes = 0;
    int i;

    strcpy(buff, content);
    strcat(buff, "$");
    for (i = 0; (bytes <= 0) && (i < 30); i++) {
        bytes = send(skt, buff, strlen(buff), 0);
    }
    if (i == 30)
        return 1;
    return 0;
}

/*------------------------------------------------- lyrelisten -------
 *|  Function lyrelisten
 *|  Purpose:  receive from a socket
 *|  Parameters:
 *|         int skt: the socket in connection
 *|         char *buff: the buffer used to store received content
 *|         int len: the capacity of the buffer
.*|
 *|  Returns:  int: state, 0 for success and else for failure
 **-------------------------------------------------------------------*/
int lyrelisten(int skt, char *buff, int len) {
    char tmp[MAX_BUFF];
    char *pos = NULL;

    memset(buff, 0, MAX_BUFF * sizeof(char));
    memset(tmp, 0, MAX_BUFF * sizeof(char));
    while (pos == NULL) {
        if (strlen(buff) + strlen(tmp) >= len) {
            return 1;
        }
        strcat(buff, tmp);
        memset(tmp, 0, sizeof(tmp));
        if (recv(skt, tmp, MAX_BUFF, 0) < 0) {
            return 1;
        }
        pos = strchr(tmp, '$');
    }
    *pos = '\0';
    strcat(buff, tmp);
    return 0;
}
