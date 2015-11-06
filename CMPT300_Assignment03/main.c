// Basic Info
//------------------------------------------------
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
#include <sys/select.h>
#include <fcntl.h>
#include "lyreegg.h"
#include "memwatch.h"

#define MAX_BUFF 2500
#define FNAMELEN 1200

void freemem(int nLine, int nCore, int *pids, int *crtpos, int **p2cFD, int **c2pFD, char *buff, char *encpt, char *decpt, char ***rrQueue) {
    int i, j;

    if (pids) free(pids);
    if (crtpos) free(crtpos);
    if (buff) free(buff);
    if (encpt) free(encpt);
    if (decpt) free(decpt);
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
    if (rrQueue) {
        for (i = 0; i < nCore; i++) {
            for (j = 0; j < nLine / nCore + 1; j++)
                free(rrQueue[i][j]);
            free(rrQueue[i]);
        }
        free(rrQueue);
    }
}

void killson(int pCP, int *alive, int **p2cFD, int **c2pFD, int *pids, char *buff, char *time_str) {
    int state;
    pid_t pid;              // for waiting

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

int main(int argc, char **argv) {
    FILE *fp = NULL;        // input file
    pid_t c_pid;            // child process ID
    pid_t pid;              // for waiting
    int pCP, i, j, flag;    // pCP stands for the child process number
    int mode = 0;           // scheduling mode
    int nLine = 0;          // the number of lines in the config file
    int state;              // whether child process run successfully
    int *pids = NULL;       // to store all children processes' pid
    int *crtpos = NULL;     // the current position of rrQueues
    int nCore = 0;          // the number of available processors
    int alive;              // the number of alive child processes
    int **p2cFD = NULL;     // parent to child pip file discrilter
    int **c2pFD = NULL;     // child to parent pip file discrilter
    int nBytes;             // number of bytes read from pipe
    int selectRes;          // result of select
    int nfds;               // the max range of readfd for select
    fd_set readfds;         // the set of read fd for select
    char time_str[30];
    char *buff = NULL;      // for reading pipe
    char *encpt = NULL;     // file name of encrypted tweets
    char *decpt = NULL;     // file name of decrypted tweets
    char ***rrQueue = NULL; // the queue for round robin

    // check arguments
    if (argc != 2) {
        printf("input format:\n./lyrebird [config file]\n");
        exit(1);
    }

    // open files
    fp = fopen(argv[1], "r");   // open config file
    if (fp == NULL) {
        gettime(time_str);
        printf("[%s] Parent process ID #%d error: file %s open failed\n", time_str, getpid(), argv[1]);
        exit(1);
    }

    // initialize strings
    buff = (char*)calloc(MAX_BUFF, sizeof(char));
    encpt = (char*)calloc(FNAMELEN, sizeof(char));
    decpt = (char*)calloc(FNAMELEN, sizeof(char));

    // get the number of available processors
    nCore = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    // initialize the pid array
    pids = (int*)calloc(nCore, sizeof(int));
    // initialize file descriptors;
    p2cFD = (int**)calloc(nCore, sizeof(int*));
    for (i = 0; i < nCore; i++)
        p2cFD[i] = (int*)calloc(2, sizeof(int));
    c2pFD = (int**)calloc(nCore, sizeof(int*));
    for (i = 0; i < nCore; i++)
        c2pFD[i] = (int*)calloc(2, sizeof(int));

    // check memory allocation error
    flag = 1;
    if (!(time_str && buff && encpt && decpt && pids && p2cFD && c2pFD))
        flag = 0;
    for (i = 0; i < nCore; i++) {
        if (!(p2cFD[i] && c2pFD[i]))
            flag = 0;
    }
    if (!flag) {
        gettime(time_str);
        printf("[%s] Parent process ID%d error: memory allocation failed!\n", time_str, getpid());
        freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
        exit(1);
    }

    // set scheduling mode
    if (fgets(buff, MAX_BUFF, fp)) {
        if (buff[strlen(buff) - 1] == '\n')
            buff[strlen(buff) - 1] = '\0';
        if (strcmp(buff, "fcfs") == 0)  // fcfs
            mode = 0;
        else if (strcmp(buff, "round robin") == 0)  // round robin
            mode = 1;
        else {  // mode error
            gettime(time_str);
            printf("[%s] Parent process ID#%d error: scheduling mode not right!\n", time_str, getpid());

            // free resources
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            exit(1);
        }
    }
    else {
        gettime(time_str);
        printf("[%s] Parent process ID#%d error: no scheduling mode selected!\n", time_str, getpid());

        // free resources
        freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
        exit(1);
    }

    // get the number of configure lines
    while (fgets(buff, MAX_BUFF, fp))
        nLine++;

    // create pipes
    for (i = 0; i < nCore; i++) {
        // create pipe from parent to children
        if (pipe(p2cFD[i]) == -1) {
            gettime(time_str);
            printf("[%s] pipe creating failed.\n", time_str);

            // free resources
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            exit(1);
        }

        // create pipe from parent to children
        if (pipe(c2pFD[i]) == -1) {
            gettime(time_str);
            printf("[%s] pipe creating failed.\n", time_str);

            // free resources
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            exit(1);
        }
    }

    // create all child processes
    for (i = 0; i < nCore; i++) {

        // indicate it is the ith child process now
        pCP = i;

        // fork child process
        c_pid = fork();
        if (c_pid == 0) {   // in child process

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
            sprintf(buff, "I'm ready!");
            nBytes = write(c2pFD[pCP][1], buff, strlen(buff) + 1);

            // read from pipe
            while ((nBytes = read(p2cFD[pCP][0], buff, MAX_BUFF)) != 0) {
                if (buff[strlen(buff) - 1] == '\n')
                    buff[strlen(buff) - 1] = '\0';

                // get input/output file name
                memset(encpt, 0, FNAMELEN);
                memset(decpt, 0, FNAMELEN);
                strncpy(encpt, buff, strchr(buff, ' ') - buff);
                strcpy(decpt, strchr(buff, ' ') + 1);
                state = lyreegg(encpt, decpt);

                // print status
                if (state == 0) {
                    gettime(time_str);
                    printf("[%s] Process ID #%d decrypted %s successfully.\n", time_str, getpid(), encpt);
                }

                // write to parent process
                sprintf(buff, "I'm ready!");
                nBytes = write(c2pFD[pCP][1], buff, strlen(buff) + 1);
            }

            // close pipe
            close(p2cFD[pCP][0]);
            close(c2pFD[pCP][1]);

            // free resources
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            // exit with state code
            exit(0);
        }
        else if (c_pid > 0) {   // in parent process
            close(p2cFD[i][0]);
            close(c2pFD[i][1]);
            gettime(time_str);
            printf("[%s] Child process ID #%d created.\n", time_str, c_pid);
            pids[i] = c_pid;  // record the pid of the child process
        }
        else {  // fork failed
            gettime(time_str);
            printf("[%s] Fork Failed\n", time_str);

            // free all malloced arrays before exiting
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            exit(1);
        }
        // initialize char arrays
    }
    alive = nCore;

    // initialize round robin queues
    if (mode == 1) {
        // allocate memory
        rrQueue = (char***)malloc(nCore * sizeof(char**));
        for (i = 0; i < nCore; i++) {
            rrQueue[i] = (char**)malloc((nLine / nCore + 1) * sizeof(char*));
            for (j = 0; j < nLine / nCore + 1; j++)
                rrQueue[i][j] = (char*)calloc(MAX_BUFF, sizeof(char));
        }

        // initialize queue head to 0
        crtpos = (int*)calloc(nCore, sizeof(int));
        
        // check memory allocation error
        flag = 1;
        if (!crtpos)
            flag = 0;
        for (i = 0; i < nCore; i++) {
            if (!rrQueue[i])
                flag = 0;
            for (j = 0; j < nCore / nLine + 1; j++)
                if (!rrQueue[i][j])
                    flag = 0;
        }
        if (!flag) {
            gettime(time_str);
            printf("[%s] Parent process ID%d error: memory allocation failed!\n", time_str, getpid());
            for (i = 0; i < nCore; i++)
                killson(i, &alive, p2cFD, c2pFD, pids, buff, time_str);
            freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
            exit(1);
        }

        // reset file pointer to config lines
        fseek(fp, 0, SEEK_SET);
        fgets(buff, MAX_BUFF, fp);

        // fill queues
        for (i = 0; i < nLine; i++)
            fgets(rrQueue[i % nCore][i / nCore], MAX_BUFF, fp);
    }

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
                    if (strcmp(buff, "kill me!") == 0) {
                        killson(i, &alive, p2cFD, c2pFD, pids, buff, time_str);
                    }
                    else {
                        if (mode == 0) {    // fcfs
                            if (fgets(buff, MAX_BUFF, fp)) {
                                memset(encpt, 0, FNAMELEN);
                                strncpy(encpt, buff, strchr(buff, ' ') - buff);
                                gettime(time_str);
                                printf("[%s] Child process ID #%d will decrypt %s.\n", time_str, pids[i], encpt);
                                write(p2cFD[i][1], buff, strlen(buff) + 1);
                            }
                            else {
                                killson(i, &alive, p2cFD, c2pFD, pids, buff, time_str);
                            }
                        }
                        else if (mode == 1) {   // round robin
                            if ((crtpos[i] < nLine / nCore + 1) && (strlen(rrQueue[i][crtpos[i]]) > 0)) {
                                memset(encpt, 0, FNAMELEN);
                                strncpy(encpt, rrQueue[i][crtpos[i]], strchr(rrQueue[i][crtpos[i]], ' ') - rrQueue[i][crtpos[i]]);
                                gettime(time_str);
                                printf("[%s] Child process ID #%d will decrypt %s.\n", time_str, pids[i], encpt);
                                write(p2cFD[i][1], rrQueue[i][crtpos[i]], strlen(rrQueue[i][crtpos[i]]) + 1);
                                crtpos[i]++;
                            }
                            else {
                                killson(i, &alive, p2cFD, c2pFD, pids, buff, time_str);
                            }
                        }
                    }
                }
            }
        }
        else if (selectRes < 0) {   // select error occurred
            continue;
        }
        else {  // nothing seleted
            continue;
        }
    }

    // free all malloced arrays before exiting
    freemem(nLine, nCore, pids, crtpos, p2cFD, c2pFD, buff, encpt, decpt, rrQueue);
    return(0);
}
