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

/*------------------------------------------------- lyrespeak --------
 *|  Function lyrespeak
 *|  Purpose:  send content through socket
 *|  Parameters:
 *|         int skt: the socket in connection
 *|         char *content: the content one wants to send.
.*|
 *|  Returns:  int: state, 0 for success and else for failure
 **-------------------------------------------------------------------*/
int lyrespeak(int skt, char *buff);

/*------------------------------------------------- lyrelisten -------
 *|  Function lyrelisten
 *|  Purpose:  receive from a socket
 *|  Parameters:
 *|         int skt: the socket in connection
 *|         char *buff: the buffer used to store received content
 *|         int len: the capacity of the buffer
.*|
 *|  Returns:  int: state, 0 for success and else for failure
 **-------------------------------------------------------------------*/
int lyrelisten(int skt, char *buff, int len);

#endif
