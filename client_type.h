#ifndef CLIENT_TYPE_H
#define CLIENT_TYPE_H

#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

typedef struct client_node
{
    struct sockaddr_in address;
    char address_string[30];
    char name[256];

    struct sockaddr_in muted[256];
    int muted_count;

    struct client_node *next;
} client_node;

typedef struct client_connection_status
{
    struct sockaddr_in *address;
    time_t last_ping;
} client_connection_status;

#endif
