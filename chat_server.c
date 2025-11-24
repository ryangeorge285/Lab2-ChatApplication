
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "udp.h"
#include "chat_parser.h"
#include "client_linked_list.h"
#include "recent_message_buffer.h"

#define RMB_BUFFER_SIZE 512

client_node *head = NULL;
pthread_rwlock_t clients_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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
void *handle_kick(void *arg);

int main(int argc, char *argv[])
{
    pthread_rwlock_init(&clients_rwlock, NULL);
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
            else if (current_request.type == REQ_KICK)
            {
                pthread_create(&tid, NULL, handle_kick, (void *)args);
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

int addr_equal(struct sockaddr_in *a, struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

client_node *get_client_by_name(char *name)
{
    client_node *client = head;
    while (client)
    {
        if (strcmp(client->name, name) == 0)
            return client;
        client = client->next;
    }
    return NULL;
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

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *new_client = client_connect(&head, &args->client_address, args->req.name);
    snprintf(server_response, BUFFER_SIZE, "Hi %s, you have been connected!", new_client->name);
    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

    char msgs[MAX_RECENT_MESSAGES][RMB_BUFFER_SIZE];
    int count = rmb_get_messages(msgs);
    if (count == 0)
        strcpy(server_response, "There are no previous messages available");
    else
        snprintf(server_response, BUFFER_SIZE, "Previous %i messages:\n", count);

    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);
    for (int i = 0; i < count; ++i)
    {
        udp_socket_write(args->sd, &args->client_address, msgs[i], RMB_BUFFER_SIZE);
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_say(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_rdlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);

    client_node *connected_client = head;
    while (connected_client != NULL)
    {
        int is_muted = 0;
        for (int i = 0; i < connected_client->muted_count; i++)
        {
            if (addr_equal(&connected_client->muted[i], &args->client_address))
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

        connected_client = connected_client->next;
    }

    char buf[RMB_BUFFER_SIZE];
    snprintf(buf, RMB_BUFFER_SIZE, "%s: %s", sender->name, args->req.msg);
    rmb_add_message(buf);
    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_sayto(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_rdlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);

    int is_muted = 0;
    for (int i = 0; i < target->muted_count; i++)
    {
        if (addr_equal(&target->muted[i], &sender->address))
        {
            is_muted = 1;
            break;
        }
    }

    if (!is_muted)
    {
        printf("Saying %s to %s\n", args->req.msg, target->name);
        snprintf(server_response, BUFFER_SIZE, "%s: %s", sender->name, args->req.msg);
        udp_socket_write(args->sd, &target->address, server_response, BUFFER_SIZE);
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_disconnect(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);

    client_node *client = head;
    while (client != NULL)
    {
        for (int i = 0; i < client->muted_count; i++)
        {
            if (addr_equal(&client->muted[i], &sender->address))
            {
                for (int j = i; j < client->muted_count - 1; j++)
                    client->muted[j] = client->muted[j + 1];

                client->muted_count--;
                i--;
            }
        }
        client = client->next;
    }

    strcpy(server_response, "Disconnected. Bye!");
    udp_socket_write(args->sd, &sender->address, server_response, BUFFER_SIZE);

    client_delete(&head, sender->name);

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_mute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);

    printf("Muting %s for %s\n", target->name, sender->name);
    for (int i = 0; i < sender->muted_count; i++)
    {
        if (addr_equal(&sender->muted[i], &target->address))
        {
            pthread_rwlock_unlock(&clients_rwlock);
            free(args);
            return NULL;
        }
    }

    sender->muted[sender->muted_count] = target->address;
    sender->muted_count++;

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_unmute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);

    printf("Unmuting %s for %s\n", target->name, sender->name);
    for (int i = 0; i < sender->muted_count; i++)
    {
        if (addr_equal(&sender->muted[i], &target->address))
        {
            for (int j = i; j < sender->muted_count - 1; j++)
                sender->muted[j] = sender->muted[j + 1];

            sender->muted_count--;
            i--;
        }
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_rename(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    if (get_client_by_name(args->req.name) != NULL)
    {
        snprintf(server_response, BUFFER_SIZE, "Server: The name %s is already taken.", args->req.name);
        udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    strcpy(sender->name, args->req.name);

    snprintf(server_response, BUFFER_SIZE, "Server: You are now renamed as %s", sender->name);
    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

void *handle_kick(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    if (ntohs(args->client_address.sin_port) != 6666)
    {
        char server_response[BUFFER_SIZE];
        snprintf(server_response, BUFFER_SIZE, "Server: You are not allowed to use /kick.");
        udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    client_node *target = get_client_by_name(args->req.name);

    client_node *client = head;
    while (client != NULL)
    {
        for (int i = 0; i < client->muted_count; i++)
        {
            if (addr_equal(&client->muted[i], &target->address))
            {
                for (int j = i; j < client->muted_count - 1; j++)
                    client->muted[j] = client->muted[j + 1];

                client->muted_count--;
                i--;
            }
        }
        client = client->next;
    }

    strcpy(server_response, "You have been kicked out of the server!!!");
    udp_socket_write(args->sd, &target->address, server_response, BUFFER_SIZE);

    snprintf(server_response, BUFFER_SIZE, "Kicked %s from the server!", target->name);
    client_delete(&head, target->name);

    client = head;
    while (client != NULL)
    {
        udp_socket_write(args->sd, &client->address, server_response, BUFFER_SIZE);
        client = client->next;
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}