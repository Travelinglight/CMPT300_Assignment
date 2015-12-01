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
#define MAX_IP_ADDR 20
#define MAX_TIME_STR 30
#define MAX_CONNECT 1000
#define MAX_BUFF 2500
#define MAX_FNAME 1250

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
    char hostaddr[MAX_IP_ADDR];         // local ip address
    int err;
    struct ifaddrs *ifaddr, *p;

    char lhostname[MAX_HOST_NAME + 1];  // local host name
    int skt;                            // the socket
    struct sockaddr_in sa;              // socket address
    struct hostent *hentry;             // host entry

    // get ip address
    if (getifaddrs(&ifaddr) == -1)
        return -1;      // get address failed
    for (p = ifaddr; p != NULL; p = p->ifa_next) {  // search the chain to find the ip address
        if ((p->ifa_addr == NULL) || (p->ifa_addr->sa_family != AF_INET))
            continue;
        memset(hostaddr, 0, MAX_IP_ADDR * sizeof(char));
        err = getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), hostaddr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if ((strcmp(hostaddr, "127.0.0.1") != 0) && (strcmp(hostaddr, "0.0.0.0") != 0)) {
            if (err)
                return -2;  // getnameinfo failed
            break;
        }
    }
    if (strlen(hostaddr) == 0)
        return -3;  // get ip address failed

    memset(&sa, 0, sizeof(struct sockaddr_in)); // clear memory
    sa.sin_family = AF_INET;         // set address family
    sa.sin_addr.s_addr = inet_addr(hostaddr);   // set ip address

    // start a socket
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -5;  // socket init failed
    }
    // set file discriptor nonblock
    fcntl(skt, F_SETFL, O_NONBLOCK);
    // bind address to the socket
    if (bind(skt, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) < 0) {
        close(skt);
        return -6;  // bind socket and address failed
    }
    // listen for connections
    if (listen(skt, MAX_CONNECT)) {
        close(skt);
        return -7;  // socket listen error
    }

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

    getsockname(skt, (struct sockaddr*)&sa, (unsigned int*)&len);
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
 *|         FILE *cfp: the config file pointer;
 *|         FILE *lfp: the log file pointer;
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void destruct(int serverSkt, client *clients, FILE *cfp, FILE *lfp) {
    client *tmp;
    while (clients->next != NULL) {
        tmp = clients->next;
        clients->next = tmp->next;
        close(tmp->sktfd);
        free(tmp);
    }
    close(serverSkt);
    if (cfp != NULL) {
        fclose(cfp);
    }
    if (lfp != NULL) {
        fclose(lfp);
    }
}

/*------------------------------------------------- welcome ----------
 *|  Function welcome
 *|  Purpose: Accept a client request, print connection info, 
 *|             and add the client into list.
 *|  Parameters:
 *|         int serverSkt: the server socket that listens on a port;
 *|         client *clients: the head of the client list;
 *|         FILE *lfp: the log file pointer;
.*|
 *|  Returns:  int status, 0 for success, else for error
 **-------------------------------------------------------------------*/
int welcome(int serverSkt, client *clients, FILE *lfp) {
    int t;
    struct sockaddr_in sa;              // socket address
    int len = sizeof(struct sockaddr);
    char time_str[MAX_TIME_STR];
    char clientaddr[MAX_IP_ADDR];
    client *newNode;

    // accept connection
    t = accept(serverSkt, (struct sockaddr*)&sa, (unsigned int*)&len);
    if (t < 0)
        return 1;

    // print client info
    inet_ntop(AF_INET, &(sa.sin_addr), clientaddr, MAX_IP_ADDR);
    gettime(time_str);
    fprintf(lfp, "[%s] Successfully connected to lyrebird client %s.\n", time_str, clientaddr);

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
 *|         FILE *lfp: the log file pointer;
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void dropClient(client **pClient, FILE *lfp) {
    client *tmp;
    char time_str[MAX_TIME_STR];

    close((*pClient)->sktfd);

    // print disconnect info
    gettime(time_str);
    fprintf(lfp, "[%s] lyrebird client %s has disconnected expectedly.\n", time_str, (*pClient)->ipaddr);

    // drop from client list
    tmp = (*pClient)->prev->next = (*pClient)->next;
    if ((*pClient)->next != NULL)
        (*pClient)->next->prev = (*pClient)->prev;
    free((*pClient));
    (*pClient) = tmp;
}

/*------------------------------------------------- main -------------
 *|  Function main
 *|  Purpose: Main function of server
 *|  Parameters:
 *|         int argc: the number of arguments
 *|         char ** argv: the arguments list
 *|
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
    char encpt[MAX_FNAME];  // file name of encrypted tweets
    char time_str[MAX_TIME_STR];
    FILE *cfp;              // file pointer to config file
    FILE *lfp;              // file pointer to log file
    int flag;               // the flag for server to quit

    // check arguments
    if (argc != 3) {
        printf("input format:  ./lyrebird [config file] [log file]\n");
        exit(1);
    }

    // open config file
    cfp = fopen(argv[1], "r");   // open config file
    if (cfp == NULL) {
        gettime(time_str);
        printf("[%s] Server ID #%d error: file %s open failed\n", time_str, getpid(), argv[1]);
        exit(1);
    }
    lfp = fopen(argv[2], "w");   // open log file
    if (lfp == NULL) {
        gettime(time_str);
        printf("[%s] Server ID #%d error: file %s open failed\n", time_str, getpid(), argv[2]);
        exit(1);
    }

    // establishment
    if ((serverSkt = serverInit()) < 0) {
        switch(serverSkt) {
            case -1:
                sprintf(buff, "getifaddrs failed");
                break;
            case -2:
                sprintf(buff, "getnameinfo failed");
                break;
            case -3:
                sprintf(buff, "get ip address failed");
                break;
            case -4:
                sprintf(buff, "gethostbyname failed");
                break;
            case -5:
                sprintf(buff, "socket init failed");
                break;
            case -6:
                sprintf(buff, "bind socket and address failed");
                break;
            case -7:
                sprintf(buff, "socket listen error");
                break;
            default:
                sprintf(buff, "unknown error");
        }
        gettime(time_str);
        printf("[%s] Server ID #%d error: %s.\n", time_str, getpid(), buff);
        exit(1);
    }
    showServer(serverSkt);

    // client list initialization
    clients.sktfd = -1;
    clients.next = NULL;
    clients.prev = NULL;
    strcpy(clients.ipaddr, "");

    // dispatching jobs
    flag = 1;
    while (flag || (clients.next != NULL)) {
        // setup readfds
        nfds = serverSkt;
        FD_ZERO(&readfds);
        FD_SET(serverSkt, &readfds);
        for (pClient = clients.next; pClient != NULL; pClient = pClient->next) {
            FD_SET(pClient->sktfd, &readfds);
            nfds = nfds < pClient->sktfd ? pClient->sktfd : nfds;
        }

        // check sockets message
        selectRes = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (selectRes > 0) {    // selected something
            for (pClient = clients.next; pClient != NULL; pClient = pClient->next) {
                if (FD_ISSET(pClient->sktfd, &readfds)) {

                    // receive from client
                    if (lyrelisten(pClient->sktfd, buff, MAX_BUFF)) {
                        lyrespeak(pClient->sktfd, "bye");
                        dropClient(&pClient, lfp);
                        continue;
                    }
                    if (strcmp(buff, "bye") == 0)   // client said good bye
                        dropClient(&pClient, lfp);
                    else {                          // client ask for jobs
                        if (strcmp(buff, "ready") != 0) {  // success / fail
                            gettime(time_str);
                            fprintf(lfp, "[%s] The lyrebird client %s has %s\n", time_str, pClient->ipaddr, buff);
                        }

                        // get task
                        if ((!flag) || (!(fgets(buff, MAX_BUFF, cfp)))) {
                            strcpy(buff, "");
                            flag = 0;
                        }

                        // assign task
                        if (!flag)  // no tasks to be assigned
                            lyrespeak(pClient->sktfd, "goodjob");
                        else {                  // assign the task
                            if (lyrespeak(pClient->sktfd, buff)) {  // speak fail
                                gettime(time_str);
                                fprintf(lfp, "[%s] Server ID #%d error: speak to client %s failed\n", time_str, getpid(), pClient->ipaddr);
                                dropClient(&pClient, lfp);
                            }
                            else {  // speak succeed
                                gettime(time_str);
                                memset(encpt, 0, MAX_FNAME);
                                strncpy(encpt, buff, strchr(buff, ' ') - buff);
                                fprintf(lfp, "[%s] The lyrebird client %s has been given the task of decrypting %s.\n", time_str, pClient->ipaddr, encpt);
                            }
                        }
                    }
                }
                if (pClient == NULL) break;
            }
            if (FD_ISSET(serverSkt, &readfds)) {
                welcome(serverSkt, &clients, lfp);
            }
        }
    }

    // exiting
    destruct(serverSkt, &clients, cfp, lfp);
    gettime(time_str);
    printf("[%s] lyrebird server: PID %d completed its tasks and is exiting successfully.\n", time_str, getpid());
    return 0;
}
