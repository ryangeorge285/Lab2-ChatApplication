#include <stdio.h>
#include "client_type.h"
#include <netinet/in.h>

// Based on the implementation from https://www.geeksforgeeks.org/c/c-program-to-implement-min-heap/

#define MAX_CLIENTS 200

// Using a static array base heap for simplicity
client_connection_status heap[MAX_CLIENTS];
int num_clients = 0;

// Swap helper
void swap(client_connection_status *a, client_connection_status *b)
{
    client_connection_status temp = *a;
    *a = *b;
    *b = temp;
}

// Inserts a new client to monitor into the min heap
void connections_status_insert(client_connection_status val)
{
    if (num_clients >= MAX_CLIENTS)
    {
        return;
    }

    heap[num_clients] = val;
    int index = num_clients;
    num_clients++;

    while (index > 0 && heap[(index - 1) / 2].last_ping > heap[index].last_ping)
    {
        swap(&heap[index], &heap[(index - 1) / 2]);
        index = (index - 1) / 2;
    }
}

// Deletes a kicked or disconnected client from the linked list
void connection_status_delete(struct sockaddr_in *address)
{
    int index = -1;

    for (int i = 0; i < num_clients; i++)
    {
        if (heap[i].address == address)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
        return;

    heap[index] = heap[num_clients - 1];
    num_clients--;

    if (index > 0 && heap[index].last_ping < heap[(index - 1) / 2].last_ping)
    {
        while (index > 0 && heap[(index - 1) / 2].last_ping > heap[index].last_ping)
        {
            swap(&heap[index], &heap[(index - 1) / 2]);
            index = (index - 1) / 2;
        }
    }
    else
    {
        while (1)
        {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;

            if (left < num_clients && heap[left].last_ping < heap[smallest].last_ping)
                smallest = left;
            if (right < num_clients && heap[right].last_ping < heap[smallest].last_ping)
                smallest = right;

            if (smallest != index)
            {
                swap(&heap[index], &heap[smallest]);
                index = smallest;
            }
            else
            {
                break;
            }
        }
    }
}

// Finds the client that was heard from the longest time ago
client_connection_status *top()
{
    if (num_clients > 0)
        return &heap[0];
    return NULL;
}

// Returns the connection status based on an address
client_connection_status *connection_status_find(struct sockaddr_in *address)
{
    for (int i = 0; i < num_clients; i++)
    {
        if (heap[i].address == address)
            return &heap[i];
    }
    return NULL;
}

// When a successful ping has been made, update the ping
void connection_status_update(struct sockaddr_in *address, time_t last_ping)
{
    connection_status_delete(address);

    client_connection_status status;
    status.address = address;
    status.last_ping = last_ping;

    connections_status_insert(status);
}
