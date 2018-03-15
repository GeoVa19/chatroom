#ifndef _CLIENT_H
#define _CLIENT_H

#include <stdio.h>
#include <netdb.h>
#include <string.h> 
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define FALSE 0
#define TRUE 1

#define BUFFER_SIZE 256
#define NAMESIZE 15
#define MESSAGE_SIZE BUFFER_SIZE + NAMESIZE + 2 //": "

#define _NAME_SIZE(size) #size
#define NAME_SIZE(size) _NAME_SIZE(size)

void error(const char *);

#endif
