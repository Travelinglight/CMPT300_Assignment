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
#include <signal.h>
#include "gettime.h"
#include "lyretalk.h"

#define MAX_HOST_NAME 80
#define MAX_IP_ADDR 16
#define MAX_TIME_STR 30
#define MAX_CONNECT 1000
#define MAX_BUFF 2500

typedef struct clientinfo {
    int sktfd;
    char ipaddr[MAX_IP_ADDR];
    struct clientinfo* next;
    struct clientinfo* prev;
} client;

/*------------------------------------------------- serverInit -------
 *|  Function serverInit
 *|  Purpose:  Set up a socket to listen on a port
 *|  Parameters: None
 *|  Returns:  The socket used to listen on a port
 **-------------------------------------------------------------------*/
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

    // start a socket
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    // set file discriptor nonblock
    fcntl(skt, F_SETFL, O_NONBLOCK);
    // bind address to the socket
    if (bind(skt, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) < 0) {
        close(skt);
        return -1;
    }
    // listen for connections
    if (listen(skt, MAX_CONNECT)) {
        close(skt);
        return -1;
    }

    free(hostaddr);
    return skt;
}

/*------------------------------------------------- showServer -------
 *|  Function showServer
 *|  Purpose:  Print the server PID, IP address, and port number
 *|  Parameters:
 *|         int skt: the server socket that listens on a port;
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
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

/*------------------------------------------------- destruct ---------
 *|  Function destruct
 *|  Purpose: free client list and close server socket
 *|  Parameters:
 *|         int serverSKt: the server listening socket;
 *|         client *clients: the head of client list;
 *|         FILE *fp: the config file pointer;
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void destruct(int serverSkt, client *clients, FILE *fp) {
    client *tmp;
    while (clients->next != NULL) {
        tmp = clients->next;
        clients->next = tmp->next;
        close(tmp->sktfd);
        free(tmp);
    }
    close(serverSkt);
    if (fp != NULL) {
        fclose(fp);
    }
}

/*------------------------------------------------- welcome ----------
 *|  Function welcome
 *|  Purpose: Accept a client request, print connection info, 
 *|             and add the client into list.
 *|  Parameters:
 *|         int serverSkt: the server socket that listens on a port;
 *|         client *clients: the head of the client list;
.*|
 *|  Returns:  int status, 0 for success, else for error
 **-------------------------------------------------------------------*/
int welcome(int serverSkt, client *clients) {
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
        return 1;
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
    newNode->prev = clients;
    if (clients->next != NULL)
        clients->next->prev = newNode;
    clients->next = newNode;

    return 0;
}

/*------------------------------------------------- dropClient -------
 *|  Function dropClient
 *|  Purpose: remove a client from the client list, and update the
 *|             pClient pointer from outside.
 *|  Parameters:
 *|         client **pClient: the 2nd order pointer to the client that
.*|                 is to be deleted.
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void dropClient(client **pClient) {
    client *tmp;

    close((*pClient)->sktfd);
    tmp = (*pClient)->prev->next = (*pClient)->next;
    if ((*pClient)->next != NULL)
        (*pClient)->next->prev = (*pClient)->prev;
    free((*pClient));
    (*pClient) = tmp;
}

void safeCtrlC(int signum) {
}

/*------------------------------------------------- main -------------
 *|  Function main
 *|  Purpose: Main function of server
 *|  Parameters: none
 *|  Returns:  execution status, 0 for normal, else for error
 **-------------------------------------------------------------------*/
int main(int argc, char **argv) {
    int serverSkt;          // the listening socket
    int nfds;               // the max range of readfd for select
    fd_set readfds;         // the set of read fd for select
    int selectRes;          // result of select
    client clients;         // a list of clients
    client *pClient;        // for iterating through client list
    client *tmpClient;      // temporary client pointer for deleting
    char buff[MAX_BUFF];    // buff 2500
    char time_str[MAX_TIME_STR];
    FILE *fp;               // file pointer to config file

/*    // check arguments
    if (argc != 2) {
        printf("input format:\n./lyrebird [config file]\n");
        exit(1);
    }

    // open config file
    fp = fopen(argv[1], "r");   // open config file
    if (fp == NULL) {
        gettime(time_str);
        printf("[%s] Server ID #%d error: file %s open failed\n", time_str, getpid(), argv[1]);
        exit(1);
    }*/

    // establishment
    serverSkt = serverInit();
    showServer(serverSkt);

    // client list initialization
    clients.sktfd = -1;
    clients.next = NULL;
    clients.prev = NULL;
    strcpy(clients.ipaddr, "");

    // do something
    while (1) {
        // setup readfds
        nfds = serverSkt;
        FD_ZERO(&readfds);
        FD_SET(serverSkt, &readfds);
        for (pClient = clients.next; pClient != NULL; pClient = pClient->next) {
            FD_SET(pClient->sktfd, &readfds);
            nfds = nfds < pClient->sktfd ? pClient->sktfd : nfds;
        }

        // check child process message
        selectRes = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (selectRes > 0) {    // selected something
            for (pClient = clients.next; pClient != NULL; pClient = pClient->next) {
                if (FD_ISSET(pClient->sktfd, &readfds)) {
                    if (lyrelisten(pClient->sktfd, buff, MAX_BUFF)) {
                        destruct(serverSkt, &clients, fp);
                        exit(1);
                    }
                    if (strcmp(buff, "bye") == 0)
                        dropClient(&pClient);
                    else {
                        if (lyrespeak(pClient->sktfd, buff)) {
                            destruct(serverSkt, &clients, fp);
                            exit(1);
                        }
                    }
                }
                if (pClient == NULL) break;
            }
            if (FD_ISSET(serverSkt, &readfds)) {
                if (welcome(serverSkt, &clients))
                    destruct(serverSkt, &clients, fp);
            }
        }
    }

    // exiting
    destruct(serverSkt, &clients, fp);
    return 0;
}
