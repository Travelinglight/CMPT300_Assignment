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

#define MAX_HOST_NAME 80
#define MAX_HOST_ADDR 16
#define MAX_CONNECT 1000

int serverInit() {
    char lhostname[MAX_HOST_NAME + 1];   // local host name
    int skt;                            // the socket
    struct sockaddr_in sa;              // socket address
    struct hostent *hentry;             // host entry

    memset(&sa, 0, sizeof(struct sockaddr_in)); // clear memory
    gethostname(lhostname, MAX_HOST_NAME);       // get local host name
    hentry = gethostbyname(lhostname);          // get host entry from name
    if (hentry == NULL) {
        return -1;
    }
    sa.sin_family = hentry->h_addrtype;         // set socket family
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  // start a socket
        return -1;
    }
    if (bind(skt, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) < 0) { // bind address to the socket
        close(skt);
        return -1;
    }
    if (listen(skt, MAX_CONNECT)) {     // listen for connections
        close(skt);
        return -1;
    }
    return skt;
}

void showServer(int skt) {
    struct ifaddrs *ifaddr, *p;
    char *hostaddr;
    int flg;    // indicating whether getnameinfo succeed/fail
    struct sockaddr_in sa;              // socket address
    int len = sizeof(struct sockaddr);

    hostaddr = (char*)calloc(MAX_HOST_ADDR + 1, sizeof(char));

    // get ip address
    if (getifaddrs(&ifaddr) == -1) {
        printf("getifaddrs failed\n");
        exit(1);
    }

    for (p = ifaddr; p != NULL; p = p->ifa_next) {
        if (p->ifa_addr == NULL)
            continue;
        flg = getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), hostaddr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (((strcmp(p->ifa_name, "wlan0") == 0) || (strcmp(p->ifa_name, "eth0") == 0)) && (p->ifa_addr->sa_family == AF_INET)) {
            if (flg) {
                printf("getnameinfo failed\n");
                exit(1);
            }
            break;
        }
    }

    // get port number
    getsockname(skt, (struct sockaddr*)&sa, &len);
    printf("listening on %s:%d\n", hostaddr, ntohs(sa.sin_port));

    free(hostaddr);
}

int constructServer() {

}

int main(int argc, char **argv) {
    int skt;

    skt = serverInit();
    showServer(skt);
    close(skt);

    return 0;
}
