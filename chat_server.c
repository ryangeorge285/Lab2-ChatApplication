
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "udp.h"
#include "chat_parser.h"
#include "client_linked_list.h"
#include "recent_message_buffer.h"
#include "client_heap.h"

#define RMB_BUFFER_SIZE 512
#define INACTIVITY_THRESHOLD_SECONDS 5 * 60
#define PING_RESPONSE_WAIT_SECONDS 5

client_node *head = NULL;
pthread_rwlock_t clients_rwlock = PTHREAD_RWLOCK_INITIALIZER;
int server_socket = -1;

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
void *handle_ret_ping(void *arg);

void *monitor_inactivity(void *arg);
void remove_client_from_muted_lists(struct sockaddr_in *target_addr);
void refresh_client_activity(struct sockaddr_in *address);

int main(int argc, char *argv[])
{
    pthread_rwlock_init(&clients_rwlock, NULL);
    rmb_init();

    int sd = udp_socket_open(SERVER_PORT);

    assert(sd > -1);
    server_socket = sd;

    pthread_t monitor_tid;
    // Starts monitoring the jjjjjnactive clients
    pthread_create(&monitor_tid, NULL, monitor_inactivity, NULL);
    pthread_detach(monitor_tid);

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

            // Now that we have received a request, initialise the thread parameters and start the thread with the correct helper function

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
            else if (current_request.type == REQ_RET_PING)
            {
                pthread_create(&tid, NULL, handle_ret_ping, (void *)args);
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

// Helper function thats compares address
int addr_equal(struct sockaddr_in *a, struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

// Helper function that returns the node of a client from a name
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

// Helper function that returns the node of a client from an address
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

// Helper function which loops through all clients and deletes all references to a certain clients address in the muted list as they are about to be kicked
void remove_client_from_muted_lists(struct sockaddr_in *target_addr)
{
    client_node *client = head;
    while (client != NULL)
    {
        for (int i = 0; i < client->muted_count; i++)
        {
            if (addr_equal(&client->muted[i], target_addr))
            {
                for (int j = i; j < client->muted_count - 1; j++)
                    client->muted[j] = client->muted[j + 1];

                client->muted_count--;
                i--;
            }
        }
        client = client->next;
    }
}

// Inactivity thread monitoring
// Refreshes the stored activity time for a client
void refresh_client_activity(struct sockaddr_in *address)
{
    connection_status_update(address, time(NULL));
}

// Handles a new client connecting to the server
void *handle_connection(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock); // Protect linked list

    client_node *new_client = client_connect(&head, &args->client_address, args->req.name);
    client_connection_status status = {.address = &new_client->address, .last_ping = time(NULL)};
    connections_status_insert(status);
    snprintf(server_response, BUFFER_SIZE, "Hi %s, you have been connected!", new_client->name);
    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);

    char msgs[MAX_RECENT_MESSAGES][RMB_BUFFER_SIZE];
    int count = rmb_get_messages(msgs);
    if (count == 0)
        strcpy(server_response, "There are no previous messages available");
    else
        snprintf(server_response, BUFFER_SIZE, "Previous %i messages:\n", count);

    udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);
    // Sends the stored messages back to the newly connected client
    for (int i = 0; i < count; ++i)
    {
        udp_socket_write(args->sd, &args->client_address, msgs[i], RMB_BUFFER_SIZE);
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

// Handles the client's response to a server ping
void *handle_ret_ping(void *arg)
{
    thread_args *args = (thread_args *)arg;

    pthread_rwlock_rdlock(&clients_rwlock);

    // Update the last ping time when a client answers
    client_node *client = get_client_from_address(&args->client_address);
    if (client != NULL)
    {
        refresh_client_activity(&client->address);
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

// Background loop that disconnects inactive clients
void *monitor_inactivity(void *arg)
{
    while (1)
    {
        // Check the client that has sent nothign for the longest time
        client_connection_status *status = top();
        if (status == NULL)
        {
            sleep(1);
            continue;
        }

        struct sockaddr_in *inactive_addr = status->address;
        time_t now = time(NULL);
        double idle_seconds = difftime(now, status->last_ping);

        if (idle_seconds < INACTIVITY_THRESHOLD_SECONDS)
        {
            sleep(1);
            continue;
        }

        // ping to check if they are inactive or not
        char ping_message[BUFFER_SIZE];
        strcpy(ping_message, "ping$");
        udp_socket_write(server_socket, inactive_addr, ping_message, BUFFER_SIZE);

        sleep(PING_RESPONSE_WAIT_SECONDS);

        client_connection_status *updated_status = connection_status_find(inactive_addr);
        if (updated_status == NULL)
        {
            continue;
        }

        now = time(NULL);
        idle_seconds = difftime(now, updated_status->last_ping);

        if (idle_seconds < INACTIVITY_THRESHOLD_SECONDS)
        {
            continue;
        }

        pthread_rwlock_wrlock(&clients_rwlock);

        client_node *inactive_client = get_client_from_address(inactive_addr);
        if (inactive_client != NULL)
        {
            char server_response[BUFFER_SIZE];
            snprintf(server_response, BUFFER_SIZE, "Server: %s has been disconnected due to inactivity.", inactive_client->name);

            udp_socket_write(server_socket, &inactive_client->address, "Disconnected due to inactivity", BUFFER_SIZE);
            remove_client_from_muted_lists(&inactive_client->address);
            connection_status_delete(inactive_addr);
            client_delete(&head, inactive_client->name);

            // Notifies everyone that the client has been removed
            client_node *client = head;
            while (client != NULL)
            {
                udp_socket_write(server_socket, &client->address, server_response, BUFFER_SIZE);
                client = client->next;
            }
        }

        pthread_rwlock_unlock(&clients_rwlock);
    }

    return NULL;
}

// Broadcasts a message to all clients who have not muted the sender
void *handle_say(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_rdlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    if (sender == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    refresh_client_activity(&sender->address);

    // Loop through all connected clients and send the message if not muted
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

// Handles messages between two specific clients
void *handle_sayto(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_rdlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);
    if (sender == NULL || target == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    refresh_client_activity(&sender->address);

    // Skip sending if the target has muted the sender
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
        udp_socket_write(args->sd, &sender->address, server_response, BUFFER_SIZE);
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

// Disconnects a client from the server
void *handle_disconnect(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    if (sender == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    // Remove any mute references before deleting the user
    remove_client_from_muted_lists(&sender->address);

    // Cache the name before deleting the node
    char disconnected_name[256];
    strcpy(disconnected_name, sender->name);

    strcpy(server_response, "Disconnected. Bye!");
    udp_socket_write(args->sd, &sender->address, server_response, BUFFER_SIZE);

    connection_status_delete(&sender->address);
    client_delete(&head, sender->name);

    // Notify remaining clients that the user left
    snprintf(server_response, BUFFER_SIZE, "Server: %s has disconnected.", disconnected_name);
    client_node *client = head;
    while (client != NULL)
    {
        udp_socket_write(args->sd, &client->address, server_response, BUFFER_SIZE);
        client = client->next;
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}

// Adds a user to the sender's muted list
void *handle_mute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);
    if (sender == NULL || target == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    refresh_client_activity(&sender->address);

    // Avoid duplicating an entry in the mute list
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

// Removes a user from the sender's muted list
void *handle_unmute(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    client_node *target = get_client_by_name(args->req.name);
    if (sender == NULL || target == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    refresh_client_activity(&sender->address);

    // Updatethe muted list to remove all matches
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

// Changes a client's display name, doesnt change much as clients are identified by address
void *handle_rename(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    if (sender == NULL)
    {
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    refresh_client_activity(&sender->address);
    if (get_client_by_name(args->req.name) != NULL)
    {
        // Do not allow two clients to share the same name
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

// Forcefully removes a client from the server (admin only)
void *handle_kick(void *arg)
{
    thread_args *args = (thread_args *)arg;
    char server_response[BUFFER_SIZE];

    pthread_rwlock_wrlock(&clients_rwlock);

    client_node *sender = get_client_from_address(&args->client_address);
    refresh_client_activity(&sender->address);

    // Only the admin port can execute kicks
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
    if (target == NULL)
    {
        snprintf(server_response, BUFFER_SIZE, "Server: User %s was not found.", args->req.name);
        udp_socket_write(args->sd, &args->client_address, server_response, BUFFER_SIZE);
        pthread_rwlock_unlock(&clients_rwlock);
        free(args);
        return NULL;
    }

    remove_client_from_muted_lists(&target->address);

    strcpy(server_response, "You have been kicked out of the server!!!");
    udp_socket_write(args->sd, &target->address, server_response, BUFFER_SIZE);

    connection_status_delete(&target->address);
    snprintf(server_response, BUFFER_SIZE, "Kicked %s from the server!", target->name);
    client_delete(&head, target->name);

    client_node *client = head;
    while (client != NULL)
    {
        udp_socket_write(args->sd, &client->address, server_response, BUFFER_SIZE);
        client = client->next;
    }

    pthread_rwlock_unlock(&clients_rwlock);

    free(args);
    return NULL;
}
