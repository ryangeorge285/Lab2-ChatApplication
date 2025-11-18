
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
void *handle_disconnect(void *arg);
void *handle_mute(void *arg);
void *handle_unmute(void *arg);
void *handle_rename(void *arg);

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
            printf("Received request %s from %s:%d\n", client_request, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

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
            else if (current_request.type == REQ_DISCONN)
            {
                pthread_create(&tid, NULL, handle_disconnect, (void *)args);
                pthread_detach(tid);
            }
            else if (current_request.type == REQ_MUTE)
            {
                pthread_create(&tid, NULL, handle_mute, (void *)args);
                pthread_detach(tid);
            }
            else if (current_request.type == REQ_UNMUTE)
            {
                pthread_create(&tid, NULL, handle_unmute, (void *)args);
                pthread_detach(tid);
            }
            else if (current_request.type == REQ_RENAME)
            {
                pthread_create(&tid, NULL, handle_rename, (void *)args);
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

client_node *get_client_from_address(struct sockaddr_in *addr)
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

    client_node *sender = get_client_from_address(&args->client_address);
    const char *sender_name = sender ? sender->name : "UNKNOWN";

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        if (connected_client->address.sin_addr.s_addr == args->client_address.sin_addr.s_addr &&
            connected_client->address.sin_port == args->client_address.sin_port)
        {
            connected_client = connected_client->next;
            continue;
        }

        int is_muted = 0;
        for (int i = 0; i < connected_client->muted_count; i++)
        {
            if (strcmp(connected_client->muted[i], sender_name) == 0)
            {
                is_muted = 1;
                break;
            }
        }

        if (!is_muted)
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

    client_node *sender = get_client_from_address(&args->client_address);

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        if (strcmp(connected_client->name, args->req.name) == 0)
        {
            int is_muted = 0;
            for (int i = 0; i < connected_client->muted_count; i++)
            {
                if (strcmp(connected_client->muted[i], sender->name) == 0)
                {
                    is_muted = 1;
                    break;
                }
            }

            if (!is_muted)
            {
                printf("Saying %s to %s\n", args->req.msg, connected_client->name);
                snprintf(server_response, BUFFER_SIZE, "%s: %s", sender->name, args->req.msg);
                udp_socket_write(args->sd, &connected_client->address, server_response, BUFFER_SIZE);
            }
            break;
        }
        connected_client = connected_client->next;
    }

    free(args);
    return NULL;
}

void *handle_disconnect(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = get_client_from_address(&args->client_address);

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        for (int i = 0; i < connected_client->muted_count; i++)
        {
            if (strcmp(connected_client->muted[i], sender->name) == 0)
            {
                for (int j = i; j < connected_client->muted_count - 1; j++)
                {
                    strcpy(connected_client->muted[j], connected_client->muted[j + 1]);
                }
                connected_client->muted_count--;
                i--;
            }
        }
        connected_client = connected_client->next;
    }

    client_delete(&head, sender->name);
    free(args);

    strcpy(server_response, "Disconnected. Bye!");
    udp_socket_write(args->sd, &connected_client->address, server_response, BUFFER_SIZE);
    return NULL;
}

void *handle_mute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = get_client_from_address(&args->client_address);

    printf("Muting %s for %s\n", args->req.name, sender->name);
    strcpy(sender->muted[sender->muted_count], args->req.name);
    sender->muted_count++;

    free(args);
    return NULL;
}

void *handle_unmute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = get_client_from_address(&args->client_address);
    const char *sender_name = sender ? sender->name : "UNKNOWN";

    char muted[256][30];
    printf("Unmuting %s for %s\n", args->req.name, sender->name);

    for (int i = 0; i < sender->muted_count; i++)
    {
        if (strcmp(sender->muted[i], args->req.name) == 0)
        {
            for (int j = i; j < sender->muted_count - 1; j++)
            {
                strcpy(sender->muted[j], sender->muted[j + 1]);
            }
            sender->muted_count--;
            i--;
        }
    }

    free(args);
    return NULL;
}

void *handle_rename(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    client_node *sender = get_client_from_address(&args->client_address);

    client_node *client = head;
    while (client != NULL)
    {
        for (int muted_client = 0; muted_client < client->muted_count; muted_client++)
        {
            if (strcmp(client->muted[muted_client], sender->name) == 0)
                strcpy(client->muted[muted_client], args->req.name);
        }
        client = client->next;
    }

    strcpy(sender->name, args->req.name);
    free(args);
    return NULL;
}