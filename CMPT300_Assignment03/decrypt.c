// Basic Info
//------------------------------------------------
//           my name : Jingda(Kingston) Chen    
//    student number : 301295759                
//     SFU user name : jca303                   
//   lecture section : CMPT 300 D100, Fall 2015 
// instructor's name : Brian Booth              
//         TA's name : Scott Kristjanson        

#include "decrypt.h"

unsigned long long fastpower(unsigned long long x, unsigned long long y, unsigned long long m) {    // x^y mod m
    unsigned long long res = 1;
    unsigned long long tmp = x;
    while (y > 0) {
        if ((y & 1) == 1)
            res = res * tmp % m;
        tmp = tmp * tmp % m;
        y >>= 1;
    }
    return res;
}

void decrypt(char *str) {
    int i, j, len;     // len: the number of blocks (of 6) in string after step 1
    char *oristr;   // original string
    char *tmpstr;   // temporary string
    unsigned long long code[24];  // used in step 2, decimal version of encrypted 41base code
    const unsigned long long base[6] = {1, 41, 1681, 68921, 2825761, 115856201};  // 41^0, 41^1, ... 41^5
    const int dict1[128] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,31, 0,27,37, 0,39,30,
    33,34, 0, 0,29,35,28,38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,36, 0,
     0, 0, 0,32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,40, 0, 0, 0, 0, 1, 2, 3,
     4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
    24,25,26, 0, 0, 0, 0, 0
    };  // dictionary: char(ascii) --> int coefficient
    const char dict2[41] = {
    ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
    'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', '#', '.', ',',
    '\'', '!', '?', '(', ')', '-', ':', '$', '/', '&',
    '\\'
    };  // dictionary: int code (from base 41) --> char

    // initialization
    oristr = (char*)malloc(sizeof(char) * 200);
    tmpstr = (char*)malloc(sizeof(char) * 8);
    strcpy(oristr, str);
    memset(str, 0, 200);

    // step 1:
    i = 0;
    while (i < strlen(oristr)) {
        if ((i % 8) == 0) { // reset 7-char tmp string
            memset(tmpstr, 0, 8);
        }
        if ((i % 8) == 7) { // ignore every 8th char
            strcat(str, tmpstr);    // concatenate the temp string to str
            i++;
            continue;
        }
        tmpstr[strlen(tmpstr)] = oristr[i];
        i++;
    }
    if (i % 6 > 0)
        strcat(str, tmpstr);    // concatenate the rest of the string

    // step 2:
    memset(code, 0, 24*sizeof(unsigned long long));
    for (i = 0; i < strlen(str); i++) {
        code[i / 6] = (code[i / 6] + dict1[str[i]] * base[5 - (i % 6)]) % 4294434817;
    }
    len = strlen(str) / 6;

    // step 3:
    for (i = 0; i < len; i++) {
        if (code[i] != 0)
            code[i] = fastpower(code[i], 1921821779, 4294434817);
    }

    // step 4:
    memset(str, 0, 140);
    for (i = 0; i < len; i++) {
        for (j = 5; j >= 0; j--) {
            str[strlen(str)] = dict2[code[i] / base[j]];
            code[i] %= base[j];
        }
    }

    free(oristr);
    free(tmpstr);
}
