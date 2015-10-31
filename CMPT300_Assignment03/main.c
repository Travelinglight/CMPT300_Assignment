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
#include "lyreegg.h"
#include "memwatch.h"

int main(int argc, char **argv) {
    pid_t c_pid;    // child process ID
    pid_t pid;  // for waiting
    int i, flag;
    int state;  // whether child process run successfully
    int *pids;  // to store all children processes' pid
    FILE *fp; // input file
    char *conf_str;  // each line of configure file
    char *time_str; // to format time
    char mode[1024];    // schedule mode
    time_t raw_time;
    int nCore;  // the number of available processors

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

    // get the number of available processors
    nCore = sysconf(_SC_NPROCESSORS_ONLN);
    // initialize the pid array
    pids = (int*)calloc(nCore, sizeof(int));


    // initialize strings
    time_str = (char*)calloc(30, sizeof(char));
    conf_str = (char*)calloc(2050, sizeof(char));

    // find out how many lines in configure file
    nLine = 0;
    while (fgets(conf_str, 2048, fp)) {
        nLine++;
    }

    nLine = 0;
    fseek(fp, 0, SEEK_SET);
    while (fgets(conf_str, 2048, fp)) {
        // get input/output file name
        conf_str[strlen(conf_str) - 1] = '\0';

        // fork child process
        c_pid = fork();
        if (c_pid == 0) {   // in child process
            if (state == 0) {   // if decrypted successfully, print log
                memset(time_str, 0, 30);
                time(&raw_time);
                strcpy(time_str, ctime(&raw_time));
                time_str[strlen(time_str) - 1] = '\0';
            }

            // free all char arrays copied from parent process
            free(time_str);
            free(conf_str);
            free(pids);

            // exit with state code
            exit(state);
        }
        else if (c_pid > 0) {   // in parent process
            // log process create time
            memset(time_str, 0, 30);
            time(&raw_time);
            strcpy(time_str, ctime(&raw_time));
            time_str[strlen(time_str) - 1] = '\0';
            pids[nLine++] = c_pid;  // record the pid of the child process
        }
        else {
            printf("Fork Failed\n");
            // free all malloced arrays before exiting
            free(time_str);
            free(conf_str);
            free(pids);
            exit(2);
        }
        // initialize char arrays
        memset(conf_str, 0, 2050);
    }
    fclose(fp);

    // wait for child processes to exit
    flag = 1;
    while (flag > 0) {
        flag = 0;
        for (i = 0; i < nLine; i++)
        if (pids[i] > 0) {
            pid = waitpid(pids[i], &state, WNOHANG);
            if (pid == -1) {
                memset(time_str, 0, 30);
                time(&raw_time);
                strcpy(time_str, ctime(&raw_time));
                time_str[strlen(time_str) - 1] = '\0';
                printf("[%s] Child process ID #%d did not terminate successfully.\n", time_str, pids[i]);
                pids[i] = -1;
            }
            else if (pid == pids[i]) {
                pids[i] = -1;
            }
            else if (pid == 0) {
                flag = 1;
            }
        }
    }

    // free all malloced arrays before exiting
    free(time_str);
    free(conf_str);
    free(pids);
}
