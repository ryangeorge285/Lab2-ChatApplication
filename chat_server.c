
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "udp.h"
#include "chat_parser.h"
#include "client_linked_list.h"

client_node *head = NULL;

typedef struct
{
    int sd;
    struct sockaddr_in client_address;
    request req;
} thread_args;

void *handle_connection(void *arg);
void *handle_say(void *arg);
void *handle_sayto(void *arg);

int main(int argc, char *argv[])
{
    int sd = udp_socket_open(SERVER_PORT);

    assert(sd > -1);

    // Server main loop
    while (1)
    {
        char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];
        struct sockaddr_in client_address;

        int rc = udp_socket_read(sd, &client_address, client_request, BUFFER_SIZE);

        // Successfully received an incoming request
        if (rc > 0)
        {
            printf("Received request %s @ %s:%d\n", client_request, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            request current_request;
            parse_input(client_request, &current_request);

            pthread_t tid;
            thread_args *args = malloc(sizeof(thread_args));
            args->sd = sd;
            args->client_address = client_address;
            args->req = current_request;

            if (current_request.type == REQ_CONN)
            {
                pthread_create(&tid, NULL, handle_connection, (void *)args);
                pthread_detach(tid);
            }
            else if (current_request.type == REQ_SAY)
            {
                pthread_create(&tid, NULL, handle_say, (void *)args);
                pthread_detach(tid);
            }
            else if (current_request.type == REQ_SAYTO)
            {
                pthread_create(&tid, NULL, handle_sayto, (void *)args);
                pthread_detach(tid);
            }
            else
            {
                printf("An unknown request was sent");
            }
        }
    }

    return 0;
}

client_node *find_client_by_addr(struct sockaddr_in *addr)
{
    client_node *connected_client = head;
    while (connected_client)
    {
        if (connected_client->address.sin_addr.s_addr == addr->sin_addr.s_addr && connected_client->address.sin_port == addr->sin_port)
        {
            return connected_client;
        }
        connected_client = connected_client->next;
    }
    return NULL;
}

void *handle_connection(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *new_client = client_connect(&head, &args->client_address, args->req.name);
    snprintf(server_response, BUFFER_SIZE, "Hi %s, you have been connected!", new_client->name);
    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

    free(args);
    return NULL;
}

void *handle_say(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = find_client_by_addr(&args->client_address);
    const char *sender_name = sender ? sender->name : "UNKNOWN";

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        if (connected_client->address.sin_addr.s_addr != args->client_address.sin_addr.s_addr || connected_client->address.sin_port != args->client_address.sin_port)
        {
            printf("Saying %s to %s\n", args->req.msg, connected_client->name);
            snprintf(server_response, BUFFER_SIZE, "%s: %s", sender_name, args->req.msg);
            udp_socket_write(args->sd, &connected_client->address, server_response, BUFFER_SIZE);
        }
        connected_client = connected_client->next;
    }

    free(args);
    return NULL;
}

void *handle_sayto(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = find_client_by_addr(&args->client_address);
    const char *sender_name = sender ? sender->name : "UNKNOWN";

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        if (strcmp(connected_client->name, args->req.name) == 0)
        {
            printf("Saying %s to %s\n", args->req.msg, connected_client->name);
            snprintf(server_response, BUFFER_SIZE, "%s: %s", sender_name, args->req.msg);
            udp_socket_write(args->sd, &connected_client->address, server_response, BUFFER_SIZE);
        }
        connected_client = connected_client->next;
    }

    free(args);
    return NULL;
}