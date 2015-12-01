// Basic Info
//------------------------------------------------
//           my name : Jingda(Kingston) Chen    
//    student number : 301295759                
//     SFU user name : jca303                   
//   lecture section : CMPT 300 D100, Fall 2015 
// instructor's name : Brian Booth              
//         TA's name : Scott Kristjanson        

#ifndef LYREEGG_H
#define LYREEGG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "decrypt.h"
#include "gettime.h"
#include "memwatch.h"

/*------------------------------------------------- lyreegg ----------
 *|  Function lyreegg
 *|  Purpose:  decrypt given file and write to the designated file
 *|  Parameters:
 *|         char *infile: the encrypted file name
 *|         char *outfile: the decrypted file name
 *|  Returns:  int standing for state, 0 for success, else for err
 **-------------------------------------------------------------------*/
int lyreegg(char *infile, char *outfile);

#endif
