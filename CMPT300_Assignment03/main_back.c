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

#define MAX_BUFF 2500

int main(int argc, char **argv) {
    pid_t c_pid;    // child process ID
    pid_t pid;  // for waiting
    int pCP, i, flag; // pCP stands for the child process number
    int state;  // whether child process run successfully
    int *pids;  // to store all children processes' pid
    char *time_str; // to format time
    char *conf_line;    // contains encrpyted file name and decrypted file name
    time_t raw_time;
    int nCore;  // the number of available processors
    int **p2cFD;   // parent to child pip file discrilter

    // initialize strings
    time_str = (char*)calloc(30, sizeof(char));
    conf_line = (char*)calloc(2400, sizeof(char));

    // get the number of available processors
    nCore = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    // initialize the pid array
    pids = (int*)calloc(nCore, sizeof(int));
    // initialize file descriptors;
    p2cFD = (int**)calloc(nCore, sizeof(int*));
    for (i = 0; i < nCore; i++)
        p2cFD[i] = (int*)calloc(2, sizeof(int));

    for (i = 0; i < nCore; i++) {
        // create pipe from parent to children
        if (pipe(p2cFD[i]) == -1) {
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            printf("[%s] pipe creating failed.\n", time_str);
            exit(1);
        }

        // indicate it is the ith child process now
        pCP = i;

        // fork child process
        c_pid = fork();
        if (c_pid == 0) {   // in child process
            int nBytes; // number of bytes read from pipe
            char *buff;    // read buffer
            char *encpt, *decpt;    // the name of encrpyted file and decrpyted file

            // initialize variables
            buff = (char*)calloc(MAX_BUFF, sizeof(char));
            encpt = (char*)calloc(1200, sizeof(char));
            decpt = (char*)calloc(1200, sizeof(char));

            // free/close unnecessary variable
            free(pids);
            close(p2cFD[pCP][1]);

            do {
                memset(buff, 0, MAX_BUFF);
                nBytes = read(p2cFD[pCP][0], buff, MAX_BUFF);
                if (nBytes >= 0) {
                    printf("p%d received\n", pCP);
                    strcat(conf_line, buff);
                }
                else {
                    memset(time_str, 0, 30);
                    time(&raw_time);
                    strcpy(time_str, ctime(&raw_time));
                    time_str[strlen(time_str) - 1] = '\0';
                    printf("[%s] process #%d read failed\n", time_str, getpid());
                }
            } while (conf_line[strlen(conf_line) - 1] != '\n');

            // get time
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            printf("[%s] process #%d(%d) read(%d): (%s)\n", time_str, getpid(), pCP, nBytes, conf_line);

            // free all char arrays copied from parent process
            free(time_str);
            free(encpt);
            free(decpt);
            free(conf_line);
            free(buff);
            close(p2cFD[pCP][0]);

            // exit with state code
            exit(0);
        }
        else if (c_pid > 0) {   // in parent process
            close(p2cFD[i][0]);
            // log process create time
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            printf("[%s] Child process ID #%d created.\n", time_str, c_pid);
            pids[i] = c_pid;  // record the pid of the child process
        }
        else {
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            printf("[%s] Fork Failed\n", time_str);
            // free all malloced arrays before exiting
            free(time_str);
            free(pids);
            exit(2);
        }
        // initialize char arrays
    }

    for (i = 0; i < nCore; i++) {
        sprintf(conf_line, "Hello my %dth child!\n", i);
        if (write(p2cFD[i][1], conf_line, strlen(conf_line) + 1) != strlen(conf_line)+1) {
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            printf("[%s] process #%d write failed\n", time_str, pCP);
        }
        else {
            printf("process #%d write succeeded\n", pCP);
        }
    }

    // wait for child processes to exit
    flag = 1;
    while (flag > 0) {
        flag = 0;
        for (i = 0; i < nCore; i++)
        if (pids[i] > 0) {
            pid = waitpid(pids[i], &state, WNOHANG);
            if (pid == -1) {
                memset(time_str, 0, 30);
                time(&raw_time);
                strcpy(time_str, ctime(&raw_time));
                time_str[strlen(time_str) - 1] = '\0';
                printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[i]);
                pids[i] = -1;
                close(p2cFD[i][1]);
            }
            else if (pid == pids[i]) {
                pids[i] = -1;
                close(p2cFD[i][1]);
            }
            else if (pid == 0) {
                flag = 1;
            }
        }
    }

    // free all malloced arrays before exiting
    free(time_str);
    free(pids);
}
