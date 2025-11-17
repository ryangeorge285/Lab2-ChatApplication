#include <netinet/in.h>
#include <stdio.h>

typedef struct client_node
{
    struct sockaddr_in address;
    char address_string[30];
    char name[256];

    struct client_node *next;
} client_node;

client_node *client_connect(client_node **head, struct sockaddr_in *new_address, char *name)
{
    client_node *new_client = (client_node *)malloc(sizeof(client_node));

    new_client->address = *new_address;

    strcpy(new_client->name, name);
    snprintf(new_client->address_string, 30, "%s:%d", inet_ntoa(new_address->sin_addr), ntohs(new_address->sin_port));

    new_client->next = *head;
    *head = new_client;

    printf("Connected %s @ %s\n", new_client->name, new_client->address_string);

    return new_client;
}