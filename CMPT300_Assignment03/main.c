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

#define MAX_BUFF 2500

void timeFormat(char *time_str) {
    time_t raw_time;

    memset(time_str, 0, 30);
    time(&raw_time);
    strcpy(time_str, ctime(&raw_time));
    time_str[strlen(time_str) - 1] = '\0';
}

int main(int argc, char **argv) {
    FILE *fp;               // input file
    pid_t c_pid;            // child process ID
    pid_t pid;              // for waiting
    int pCP, i, j, flag, mode; // pCP stands for the child process number
    int nLine;              // the number of lines in the config file
    int state;              // whether child process run successfully
    int *pids;              // to store all children processes' pid
    int *serve;             // the serve order in fcfs scheduling
    int *crtpos;            // the current position of rrQueues
    int nCore;              // the number of available processors
    int alive;              // the number of alive child processes
    int **p2cFD;            // parent to child pip file discrilter
    int **c2pFD;            // child to parent pip file discrilter
    int nBytes;             // number of bytes read from pipe
    int selectRes;          // result of select
    int nfds;               // the max range of readfd for select
    fd_set readfds;         // the set of read fd for select
    char *time_str;         // to format time
    char *conf_line;        // contains encrpyted file name and decrypted file name
    char *buff;             // for reading pipe
    char ***rrQueue;        // the queue for round robin

    // check arguments
    if (argc != 2) {
        printf("input format:\n./lyrebird [config file]\n");
        exit(1);
    }

    // open files
    fp = fopen(argv[1], "r");   // open config file
    if (fp == NULL) {
        printf("file open failed\n");
        exit(1);
    }

    // initialize strings
    time_str = (char*)calloc(30, sizeof(char));
    conf_line = (char*)calloc(MAX_BUFF, sizeof(char));
    buff = (char*)calloc(MAX_BUFF, sizeof(char));

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

    // create pipes
    for (i = 0; i < nCore; i++) {

        // create pipe from parent to children
        if (pipe(p2cFD[i]) == -1) {
            timeFormat(time_str);
            printf("[%s] pipe creating failed.\n", time_str);

            // free resources
            free(time_str);
            free(conf_line);
            free(pids);
            for (i = 0; i < nCore; i++)
                free(p2cFD[i]);
            free(p2cFD);
            for (i = 0; i < nCore; i++)
                free(c2pFD[i]);
            free(c2pFD);
            exit(1);
        }

        // create pipe from parent to children
        if (pipe(c2pFD[i]) == -1) {
            timeFormat(time_str);
            printf("[%s] pipe creating failed.\n", time_str);

            // free resources
            free(time_str);
            free(conf_line);
            free(pids);
            for (i = 0; i < nCore; i++)
                free(p2cFD[i]);
            free(p2cFD);
            for (i = 0; i < nCore; i++)
                free(c2pFD[i]);
            free(c2pFD);
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
            char *encpt, *decpt;    // the name of encrpyted file and decrpyted file

            // free/close unnecessary variable
            free(pids);
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

            // initialize variables
            encpt = (char*)calloc(1200, sizeof(char));
            decpt = (char*)calloc(1200, sizeof(char));

            // write to parent process
            sprintf(buff, "I'm ready!");
            nBytes = write(c2pFD[pCP][1], buff, strlen(buff) + 1);

            // read from pipe
            while ((nBytes = read(p2cFD[pCP][0], buff, MAX_BUFF)) > 0) {
                if (buff[strlen(buff) - 1] == '\n')
                    buff[strlen(buff) - 1] = '\0';
                printf("Child process .%d received (%s)\n", pCP, buff);
                // write to parent process
                sprintf(buff, "I'm ready!");
                nBytes = write(c2pFD[pCP][1], buff, strlen(buff) + 1);
            }
            if (nBytes == 0) {
                // parent stop writing
            }
            else {
                // error here
            }

            // close pipe
            close(p2cFD[pCP][0]);
            close(c2pFD[pCP][1]);

            // free local variables
            free(encpt);
            free(decpt);

            // free inherited variables
            free(buff);
            free(time_str);
            free(conf_line);
            free(pids);
            for (i = 0; i < nCore; i++)
                free(p2cFD[i]);
            free(p2cFD);
            for (i = 0; i < nCore; i++)
                free(c2pFD[i]);
            free(c2pFD);

            // exit with state code
            exit(0);
        }
        else if (c_pid > 0) {   // in parent process
            close(p2cFD[i][0]);
            close(c2pFD[i][1]);
            timeFormat(time_str);
            printf("[%s] Child process ID #%d created.\n", time_str, c_pid);
            pids[i] = c_pid;  // record the pid of the child process
        }
        else {  // fork failed
            timeFormat(time_str);
            printf("[%s] Fork Failed\n", time_str);

            // free all malloced arrays before exiting
            free(time_str);
            free(conf_line);
            free(pids);
            for (i = 0; i < nCore; i++)
                free(p2cFD[i]);
            free(p2cFD);
            for (i = 0; i < nCore; i++)
                free(c2pFD[i]);
            free(c2pFD);
            exit(2);
        }
        // initialize char arrays
    }

    // set mode
    if (fgets(buff, MAX_BUFF, fp)) {
        if (buff[strlen(buff) - 1] == '\n')
            buff[strlen(buff) - 1] = '\0';
        if (strcmp(buff, "fcfs") == 0)  // fcfs
            mode = 0;
        else if (strcmp(buff, "round robin") == 0)  // round robin
            mode = 1;
        else    // mode error
            mode = -1;
    }
    else {
        // config file empty
    }

    // reset file pointer to config lines
    fseek(fp, 0, SEEK_SET);
    fgets(buff, MAX_BUFF, fp);

    // initialize round robin queues
    if (mode == 1) {
        // claim memory
        rrQueue = (char***)malloc(nCore * sizeof(char**));
        for (i = 0; i < nCore; i++) {
            rrQueue[i] = (char**)malloc((nLine / nCore + 1) * sizeof(char*));
            for (j = 0; j < nLine / nCore + 1; j++)
                rrQueue[i][j] = (char*)calloc(MAX_BUFF, sizeof(char));
        }

        // fill queues
        for (i = 0; i < nLine; i++)
            fgets(rrQueue[i % nCore][i / nCore], MAX_BUFF, fp);

        // initialize queue head to 0
        crtpos = (int*)calloc(nCore, sizeof(int));
    }

    alive = nCore;
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
                        close(p2cFD[i][1]);
                        while (read(c2pFD[i][0], buff, MAX_BUFF) > 0);
                        close(c2pFD[i][0]);

                        // wait child process to exit
                        while ((pid = waitpid(pids[i], &state, WNOHANG)) == 0);
                        if (pid == -1) {    // childe process terminates unsuccessfully
                            timeFormat(time_str);
                            printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[i]);
                            pids[i] = -1;
                        }
                        else if (pid == pids[i]) {  // child process terminates successfully
                            timeFormat(time_str);
                            printf("[%s] Child process ID #%d terminates successfully.\n", time_str, pid);
                            pids[i] = -1;
                        }

                        alive--;
                    }
                    else {
                        if (mode == 0) {    // fcfs
                            if (fgets(buff, MAX_BUFF, fp)) {
                                write(p2cFD[i][1], buff, strlen(buff) + 1);
                            }
                            else {
                                close(p2cFD[i][1]);
                                while (read(c2pFD[i][0], buff, MAX_BUFF) > 0);
                                close(c2pFD[i][0]);

                                // wait child process to exit
                                while ((pid = waitpid(pids[i], &state, WNOHANG)) == 0);
                                if (pid == -1) {    // childe process terminates unsuccessfully
                                    timeFormat(time_str);
                                    printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[i]);
                                    pids[i] = -1;
                                }
                                else if (pid == pids[i]) {  // child process terminates successfully
                                    timeFormat(time_str);
                                    printf("[%s] Child process ID #%d terminates successfully.\n", time_str, pid);
                                    pids[i] = -1;
                                }

                                alive--;
                            }
                        }
                        else if (mode == 1) {   // round robin
                            if ((crtpos[i] < nLine / nCore + 1) && (strlen(rrQueue[i][crtpos[i]]) > 0)) {
                                write(p2cFD[i][1], rrQueue[i][crtpos[i]], strlen(rrQueue[i][crtpos[i]]) + 1);
                                crtpos[i]++;
                            }
                            else {
                                close(p2cFD[i][1]);
                                while (read(c2pFD[i][0], buff, MAX_BUFF) > 0);
                                close(c2pFD[i][0]);

                                // wait child process to exit
                                while ((pid = waitpid(pids[i], &state, WNOHANG)) == 0);
                                if (pid == -1) {    // childe process terminates unsuccessfully
                                    timeFormat(time_str);
                                    printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[i]);
                                    pids[i] = -1;
                                }
                                else if (pid == pids[i]) {  // child process terminates successfully
                                    timeFormat(time_str);
                                    printf("[%s] Child process ID #%d terminates successfully.\n", time_str, pid);
                                    pids[i] = -1;
                                }

                                alive--;
                            }
                        }
                    }
                }
            }
        }
        else if (selectRes < 0) {   // select error occurred
            // error in select
        }
        else {  // nothing seleted
            continue;
        }
    }

    // free all malloced arrays before exiting
    free(time_str);
    free(pids);
    for (i = 0; i < nCore; i++)
        free(p2cFD[i]);
    free(p2cFD);
    for (i = 0; i < nCore; i++)
        free(c2pFD[i]);
    free(c2pFD);
    return(0);
}
