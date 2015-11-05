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
#include "decrypt.h"
#include "memwatch.h"

int lyreegg(char *infile, char *outfile) {
    char *str;
    FILE *enfp; // input file
    FILE *defp; // output file
    int i;

    // open files
    enfp = fopen(infile, "r");   // open encrypted file
    defp = fopen(outfile, "w");   // open decrypted file
    if (enfp == NULL) {
        printf("file %s open failed\n", infile);
        return 1;
    }
    if (defp == NULL) {
        printf("file %s open failed\n", outfile);
        return 1;
    }

    str = (char*)calloc(200, sizeof(char));
    while(fgets(str, 200, enfp)) {
        str[strlen(str) - 1] = '\0';
        decrypt(str);
        fprintf(defp, "%s\n", str);
        memset(str, 0, 200);
    }

    fclose(enfp);
    fclose(defp);
    free(str);
    return 0;
}
