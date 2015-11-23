// Basic Info
//////////////////////////////////////////////////
//           my name : Jingda(Kingston) Chen    
//    student number : 301295759                
//     SFU user name : jca303                   
//   lecture section : CMPT 300 D100, Fall 2015 
// instructor's name : Brian Booth              
//         TA's name : Scott Kristjanson        

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "gettime.h"

#define MAX_HOST_NAME 80
#define MAX_IP_ADDR 16
#define MAX_TIME_STR 30
#define MAX_CONNECT 1000

int callhome(char *hostaddr, unsigned short port) {
    char time_str[MAX_TIME_STR];
    struct sockaddr_in sa;
    int skt;

    // specify server
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(hostaddr);   // set ip address
    sa.sin_port = htons(port);

    // create a socket
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    // connect to server
    if (connect(skt, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) < 0) {
        close(skt);
        return -1;
    }

    // print info
    gettime(time_str);
    printf("[%s] lyrebird client: PID %d connected to server %s on port %d.\n", time_str, getpid(), hostaddr, port);
    return skt;
}

void speak2mom(int skt, char *content) {
    int bytes = 0;
    int i;

    for (i = 0; (bytes <= 0) && (i < 30); i++) {
        bytes = send(skt, content, strlen(content), 0);
    }
    if (i == 30) {
        printf("message sent failed\n");
        close(skt);
        exit(1);
    }
}

int main(int argc, char **argv) {
    int skt;

    // check for the arguments
    if (argc != 3) {
        printf("input format:  ./lyrebird hostIP portnumber\n");
        exit(1);
    }

    // make connection to server
    skt = callhome(argv[1], (unsigned short)atoi(argv[2]));
    speak2mom(skt, "Mom, how are you?$\n");
    close(skt);
    return 0;
}
