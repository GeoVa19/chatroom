#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>

#define FALSE 0
#define TRUE 1
#define MAX_CLIENTS 10

#define BUFFER_SIZE 256
#define NAMESIZE 15
#define MESSAGE_SIZE BUFFER_SIZE + NAMESIZE + 2 //": "
#define HISTORY_SIZE 1000 * MESSAGE_SIZE

#define PORT "8085" //port number

void error(const char *);
int is_full(const char *);

