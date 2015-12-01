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
#include "lyreegg.h"

#define MAX_HOST_NAME 80
#define MAX_IP_ADDR 16
#define MAX_TIME_STR 30
#define MAX_CONNECT 1000
#define MAX_BUFF 2500
#define MAX_FNAME 1250



/*------------------------------------------------- callhome ---------
 *|  Function callhome
 *|  Purpose: connect to server
 *|  Parameters:
 *|         char *hostaddr: the server ip address
 *|         unsigned short port: the port the server is listening on
.*|
 *|  Returns:  The socket used to take to the server
 **-------------------------------------------------------------------*/
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

/*------------------------------------------------- freemem ----------
 *|  Function freemem
 *|  Purpose: free memory before exiting
 *|  Parameters:
 *|         int nCore: the number of child processes
 *|         int *pids: the array of child processes' pid
 *|         int **p2cFD: the 2nd order array of pipe fd from parent
 *|                     to children
 *|         int **c2pFD: the 2nd order array of pipe fd from children
 *|                     to parent
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void freemem(int nCore, int *pids, int **p2cFD, int **c2pFD) {
    int i, j;

    if (pids) free(pids);
    if (p2cFD) {
        for (i = 0; i < nCore; i++) {
            free(p2cFD[i]);
        }
        free(p2cFD);
    }
    if (c2pFD) {
        for (i = 0; i < nCore; i++) {
            free(c2pFD[i]);
        }
        free(c2pFD);
    }
}

void allocmem(int nCore, int **pids, int ***p2cFD, int ***c2pFD) {
    int i;
    int flag;
    char time_str[MAX_TIME_STR];

    // initialize the pid array
    *pids = (int*)calloc(nCore, sizeof(int));
    // initialize file descriptors;
    *p2cFD = (int**)calloc(nCore, sizeof(int*));
    for (i = 0; i < nCore; i++)
        (*p2cFD)[i] = (int*)calloc(2, sizeof(int));
    *c2pFD = (int**)calloc(nCore, sizeof(int*));
    for (i = 0; i < nCore; i++)
        (*c2pFD)[i] = (int*)calloc(2, sizeof(int));

    // check memory allocation error
    flag = 1;
    if (!(pids && p2cFD && c2pFD))
        flag = 0;
    for (i = 0; i < nCore; i++) {
        if (!(p2cFD[i] && c2pFD[i]))
            flag = 0;
    }
    if (!flag) {
        gettime(time_str);
        printf("[%s] Parent process ID%d error: memory allocation failed!\n", time_str, getpid());
        freemem(nCore, *pids, *p2cFD, *c2pFD);
        exit(1);
    }
}

void createPipes(int nCore, int *pids, int **p2cFD, int **c2pFD) {
    int i;
    char time_str[MAX_TIME_STR];

    for (i = 0; i < nCore; i++) {
        // create pipe from parent to children
        if (pipe(p2cFD[i]) == -1) {
            gettime(time_str);
            printf("[%s] pipe creating failed.\n", time_str);
            freemem(nCore, pids, p2cFD, c2pFD);
            exit(1);
        }

        // create pipe from parent to children
        if (pipe(c2pFD[i]) == -1) {
            gettime(time_str);
            printf("[%s] pipe creating failed.\n", time_str);
            freemem(nCore, pids, p2cFD, c2pFD);
            exit(1);
        }
    }
}


void childRoutine(int nCore, int pCP, int **p2cFD, int **c2pFD, char *buff, char *encpt, char *decpt) {
    int i;
    int nBytes;
    int state = -1;
    char time_str[MAX_TIME_STR];

    // close unnecessary pipes
    for (i = 0; i < nCore; i++) {
        close(p2cFD[i][1]);
        if (i != pCP)
            close(p2cFD[i][0]);
    }
    for (i = 0; i < nCore; i++) {
        close(c2pFD[i][0]);
        if (i != pCP)
            close(c2pFD[i][1]);
    }

    // write to parent process
    sprintf(buff, "ready");
    write(c2pFD[pCP][1], buff, strlen(buff) + 1);

    // read from pipe
    while ((nBytes = read(p2cFD[pCP][0], buff, MAX_BUFF)) != 0) {
        if (buff[strlen(buff) - 1] == '\n')
            buff[strlen(buff) - 1] = '\0';

        // get input/output file name
        memset(encpt, 0, MAX_FNAME);
        memset(decpt, 0, MAX_FNAME);
        strncpy(encpt, buff, strchr(buff, ' ') - buff);
        strcpy(decpt, strchr(buff, ' ') + 1);
        state = lyreegg(encpt, decpt);

        // print status
        if (state == 0) {
            gettime(time_str);
            sprintf(buff, "successfully decrypted %s in process %d.", encpt, getpid());
        }
        else if (state == 1) {
            sprintf(buff, "encountered an error: Unable to open file %s in process %d.", encpt, getpid());
        }
        else if (state == 2) {
            sprintf(buff, "encountered an error: Unable to open file %s in process %d.", decpt, getpid());
        }

        // write to parent process
        write(c2pFD[pCP][1], buff, strlen(buff) + 1);
    }
}

/*------------------------------------------------- killson ---------
 *|  Function killson
 *|  Purpose: make a child process exit
 *|  Parameters:
 *|         int pCP: which child to exit
 *|         int *alive: the number of alive children
 *|         int **p2cFD: the 2nd order array of pipe fd from parent
 *|                     to children
 *|         int **c2pFD: the 2nd order array of pipe fd from children
 *|                     to parent
 *|         int *pids: the array of child processes' pid
 *|         char *buff: buffer used for communication
.*|
 *|  Returns:  void
 **-------------------------------------------------------------------*/
void killson(int pCP, int *alive, int **p2cFD, int **c2pFD, int *pids, char *buff) {
    int state;
    pid_t pid;              // for waiting
    char time_str[MAX_TIME_STR];

    // close pipes
    close(p2cFD[pCP][1]);
    while (read(c2pFD[pCP][0], buff, MAX_BUFF) > 0);
    close(c2pFD[pCP][0]);

    // wait child process to exit
    while ((pid = waitpid(pids[pCP], &state, WNOHANG)) == 0);
    if (pid == -1) {    // childe process terminates unsuccessfully
        gettime(time_str);
        printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[pCP]);
        pids[pCP] = -1;
    }
    else if (pid == pids[pCP]) {  // child process terminates successfully
        pids[pCP] = -1;
    }

    (*alive)--;
}

/*------------------------------------------------- main -------------
 *|  Function main
 *|  Purpose: Main function of client
 *|  Parameters: 
 *|         int argc: the number of arguments
 *|         char ** argv: the arguments list
 *|
 *|  Returns:  execution state, 0 for normal, else for error
 **-------------------------------------------------------------------*/
int main(int argc, char **argv) {
    char encpt[MAX_FNAME];  // file name of encrypted tweets
    char decpt[MAX_FNAME];  // file name of decrypted tweets
    char time_str[MAX_TIME_STR];
    char buff[MAX_BUFF];    // buffer
    pid_t c_pid;            // child process ID
    pid_t pid;              // for waiting
    int pCP, i, j, flag;    // pCP stands for the child process number
    int nCore = 0;          // the number of available processors
    int alive;              // the number of alive child processes
    int *pids = NULL;       // to store all children processes' pid
    int **p2cFD = NULL;     // parent to child pip file discrilter
    int **c2pFD = NULL;     // child to parent pip file discrilter
    int selectRes;          // result of select
    int nfds;               // the max range of readfd for select
    fd_set readfds;         // the set of read fd for select
    int skt;                // socket fd to server

    // check for the arguments
    if (argc != 3) {
        printf("input format:  ./lyrebird hostIP portnumber\n");
        exit(1);
    }

    // get the number of available processors
    nCore = sysconf(_SC_NPROCESSORS_ONLN) - 1;

    // allocate memory
    allocmem(nCore, &pids, &p2cFD, &c2pFD);

    // create pipes
    createPipes(nCore, pids, p2cFD, c2pFD);

    // create all child processes
    for (i = 0; i < nCore; i++) {

        pCP = i;        // indicate it is the ith child process now

        // fork child process
        c_pid = fork();
        if (c_pid == 0) {   // in child process
            // do things
            childRoutine(nCore, pCP, p2cFD, c2pFD, buff, encpt, decpt);

            // close pipe
            close(p2cFD[pCP][0]);
            close(c2pFD[pCP][1]);

            // free resources
            freemem(nCore, pids, p2cFD, c2pFD);
            exit(0);
        }
        else if (c_pid > 0) {   // in parent process
            close(p2cFD[i][0]);
            close(c2pFD[i][1]);
            pids[i] = c_pid;  // record the pid of the child process
        }
        else {  // fork failed
            gettime(time_str);
            printf("[%s] Parent process ID #%d error: Fork Failed\n", time_str, getpid());
            freemem(nCore, pids, p2cFD, c2pFD);
            exit(1);
        }
    }
    alive = nCore;

    // make connection to server
    skt = callhome(argv[1], (unsigned short)atoi(argv[2]));
    flag = 1;   // flag=1 means tasks available

    // start dispatching jobs
    while (alive > 0) {
        // setup readfds
        nfds = -1;
        FD_ZERO(&readfds);
        for (i=0; i < nCore; i++) {
            if (fcntl(c2pFD[i][0], F_GETFD) != -1) {
                FD_SET(c2pFD[i][0], &readfds);
                // find max
                nfds = nfds < c2pFD[i][0] ? c2pFD[i][0]: nfds;
            }
        }

        // check child process message
        selectRes = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (selectRes > 0) {    // selected something
            for (i=0; i < nCore; i++) {
                // Check the fd, and process
                if (FD_ISSET(c2pFD[i][0], &readfds)) {
                    read(c2pFD[i][0], buff, MAX_BUFF);
                    if (strcmp(buff, "kill me!") == 0) {    // child process requires to be killed
                        killson(i, &alive, p2cFD, c2pFD, pids, buff);
                    }
                    else {
                        // ask for task
                        lyrespeak(skt, buff);   // ready or success or failure

                        // receive task and dispatch it
                        lyrelisten(skt, buff, MAX_BUFF);
                        if (strcmp(buff, "goodjob") == 0)
                            killson(i, &alive, p2cFD, c2pFD, pids, buff);
                        else
                            write(p2cFD[i][1], buff, strlen(buff) + 1);
                    }
                }
            }
        }
    }

    lyrespeak(skt, "bye");   // ready or success or failure
    close(skt);
    freemem(nCore, pids, p2cFD, c2pFD);
    gettime(time_str);
    printf("[%s] lyrebird client: PID %d completed its tasks and is exiting successfully.\n", time_str, getpid());
    return 0;
}
