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

typedef struct clientinfo {
    int sktfd;
    char ipaddr[MAX_IP_ADDR];
    struct clientinfo* next;
} client;

int serverInit() {
    char *hostaddr;
    int err;
    struct ifaddrs *ifaddr, *p;

    char lhostname[MAX_HOST_NAME + 1];  // local host name
    int skt;                            // the socket
    struct sockaddr_in sa;              // socket address
    struct hostent *hentry;             // host entry

    // get ip address
    hostaddr = (char*)calloc(MAX_IP_ADDR + 1, sizeof(char));
    if (getifaddrs(&ifaddr) == -1) {
        printf("getifaddrs failed\n");
        exit(1);
    }
    for (p = ifaddr; p != NULL; p = p->ifa_next) {  // search the chain to find the ip address
        if (p->ifa_addr == NULL)
            continue;
        err = getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), hostaddr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (((strcmp(p->ifa_name, "wlan0") == 0) || (strcmp(p->ifa_name, "eth0") == 0)) && (p->ifa_addr->sa_family == AF_INET)) {
            if (err) {
                printf("getnameinfo failed\n");
                exit(1);
            }
            break;
        }
    }
    if (strlen(hostaddr) == 0) {
        printf("get ip address failed\n");
        exit(1);
    }

    memset(&sa, 0, sizeof(struct sockaddr_in)); // clear memory
    gethostname(lhostname, MAX_HOST_NAME);       // get local host name
    hentry = gethostbyname(lhostname);          // get host entry from name
    if (hentry == NULL) {
        return -1;
    }
    sa.sin_family = hentry->h_addrtype;         // set address family
    sa.sin_addr.s_addr = inet_addr(hostaddr);   // set ip address
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  // start a socket
        return -1;
    }
    fcntl(skt, F_SETFL, O_NONBLOCK);
    if (bind(skt, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) < 0) { // bind address to the socket
        close(skt);
        return -1;
    }
    if (listen(skt, MAX_CONNECT)) {     // listen for connections
        close(skt);
        return -1;
    }

    free(hostaddr);
    return skt;
}

void showServer(int skt) {
    struct sockaddr_in sa;              // socket address
    int len = sizeof(struct sockaddr);
    char time_str[MAX_TIME_STR];
    char hostaddr[MAX_IP_ADDR];

    getsockname(skt, (struct sockaddr*)&sa, &len);
    inet_ntop(AF_INET, &(sa.sin_addr), hostaddr, MAX_IP_ADDR);
    gettime(time_str);
    printf("[%s] lyrebird.server: PID %d on host %s, port %d\n", time_str, getpid(), hostaddr, ntohs(sa.sin_port));
}

void welcome(int serverSkt, client *clients) {
    int t;
    struct sockaddr_in sa;              // socket address
    int len = sizeof(struct sockaddr);
    char time_str[MAX_TIME_STR];
    char clientaddr[MAX_IP_ADDR];
    client *newNode;

    // accept connection
    t = accept(serverSkt, (struct sockaddr*)&sa, &len);
    if (t < 0) {
        printf("Accept error\n");
        exit(1);
    }

    // print client info
    inet_ntop(AF_INET, &(sa.sin_addr), clientaddr, MAX_IP_ADDR);
    gettime(time_str);
    printf("[%s] Successfully connected to lyrebird client %s.\n", time_str, clientaddr);

    // add to linked list
    newNode = (client*)malloc(sizeof(client));
    newNode->sktfd = t;
    strcpy(newNode->ipaddr, clientaddr);
    newNode->next = clients->next;
    clients->next = newNode;
}

int main(int argc, char **argv) {
    int serverSkt;          // the listening socket
    int nfds;               // the max range of readfd for select
    fd_set readfds;         // the set of read fd for select
    int selectRes;          // result of select
    client clients;         // a list of clients
    client *pClient;        // for iterating through client list
    client *tmpClient;      // temporary client pointer for deleting

    // establishment
    serverSkt = serverInit();
    showServer(serverSkt);

    // client list initialization
    clients.sktfd = -1;
    clients.next = NULL;
    strcpy(clients.ipaddr, "");

    // do something
    while (1) {
        // setup readfds
        nfds = serverSkt;
        FD_ZERO(&readfds);
        FD_SET(serverSkt, &readfds);
        for (pClient = clients->next; pClient != NULL; pClient = pClient->next) {
            FD_SET(pClient->sktfd, &readfds);
            nfds = nfds < pClient->sktfd ? pClient->sktfd : nfds;
        }

        // check child process message
        selectRes = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (selectRes > 0) {    // selected something
            if (FD_ISSET(serverSkt, &readfds)) {
                welcome(serverSkt, &clients);
            }
        }
    }

    // exiting
    close(serverSkt);

    return 0;
}
