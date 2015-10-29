#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readline.h"
#include "decrypt.h"
#include "memwatch.h"

int main(int argc, char **argv) {
    int i;
    FILE *enfp; // input file
    FILE *defp; // output file
    char *str;  // each line of encrypted file

    // check arguments
    if (argc != 3) {
        printf("invalid number of arguments\n");
        exit(1);
    }

    // open files
    enfp = fopen(argv[1], "r");   // open encrypted file
    defp = fopen(argv[2], "w");   // open decrypted file
    if ((enfp == NULL) || (defp == NULL)) {
        printf("file open failed\n");
        exit(1);
    }

    str = (char*)malloc(sizeof(char) * 140);
    while(1) {
        memset(str, 0, 140);
        readline(enfp, str);
        if(feof(enfp))
            break;
        decrypt(str);
        fprintf(defp, "%s\n", str);
    }

    free(str);
    fclose(enfp);
    fclose(defp);
}
