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
    pid_t c_pid;    // child process ID
    pid_t pid;  // for waiting
    int pCP, i, flag; // pCP stands for the child process number
    int state;  // whether child process run successfully
    int *pids;  // to store all children processes' pid
    int nCore;  // the number of available processors
    int alive;  // the number of alive child processes
    int **p2cFD;   // parent to child pip file discrilter
    int **c2pFD;   // child to parent pip file discrilter
    int nBytes; // number of bytes read from pipe
    int selectRes;  // result of select
    int nfds;   // the max range of readfd for select
    fd_set readfds; // the set of read fd for select
    char *time_str; // to format time
    char *conf_line;    // contains encrpyted file name and decrypted file name
    char *buff;     // for reading pipe

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
            sprintf(conf_line, "who am I?");
            nBytes = write(c2pFD[pCP][1], conf_line, strlen(conf_line) + 1);
            printf("%dth child write %d bytes\n", pCP, nBytes);

            // read from pipe
            nBytes = read(p2cFD[pCP][0], buff, MAX_BUFF);
            if (nBytes > 0) {
                sprintf(conf_line, "my parent said (%s)!", buff);
                write(c2pFD[pCP][1], conf_line, strlen(conf_line) + 1);
            }
            else if (nBytes == 0) {
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

        selectRes = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (selectRes > 0) {    // selected something
            for (i=0; i < nCore; i++) {
                // Check the fd, and process
                if (FD_ISSET(c2pFD[i][0], &readfds)) {
                    read(c2pFD[i][0], buff, MAX_BUFF);
                    if (strcmp(buff, "who am I?") == 0) {
                        sprintf(conf_line, "You are my %dth child!", i);
                        write(p2cFD[i][1], conf_line, strlen(conf_line) + 1);
                    }
                    else {
                        printf("My %dth child said:'%s'!\n", i, buff);
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
        else if (selectRes < 0) {   // error occurred
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
