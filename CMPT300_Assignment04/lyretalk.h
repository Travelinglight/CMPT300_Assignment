// Basic Info
//------------------------------------------------
//           my name : Jingda(Kingston) Chen    
//    student number : 301295759                
//     SFU user name : jca303                   
//   lecture section : CMPT 300 D100, Fall 2015 
// instructor's name : Brian Booth              
//         TA's name : Scott Kristjanson        

#ifndef GETTIME_H
#define GETTIME_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "gettime.h"
#include "memwatch.h"

void lyrespeak(int skt, char *buff);
void lyrelisten(int skt, char *buff, int len);

#endif
