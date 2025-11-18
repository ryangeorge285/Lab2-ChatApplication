#include <netinet/in.h>
#include <stdio.h>

typedef struct client_node
{
    struct sockaddr_in address;
    char address_string[30];
    char name[256];

    char muted[256][30];
    int muted_count;

    struct client_node *next;
} client_node;

client_node *client_connect(client_node **head, struct sockaddr_in *new_address, char *name)
{
    client_node *new_client = (client_node *)malloc(sizeof(client_node));

    new_client->address = *new_address;
    new_client->muted_count = 0;

    strcpy(new_client->name, name);
    snprintf(new_client->address_string, 30, "%s:%d", inet_ntoa(new_address->sin_addr), ntohs(new_address->sin_port));

    new_client->next = *head;
    *head = new_client;

    printf("Connected %s @ %s\n", new_client->name, new_client->address_string);

    return new_client;
}

int client_delete(client_node **head, char *name)
{
    client_node *current = *head;

    if (current == NULL)
        return 1;

    if (strcmp(current->name, name) == 0)
    {
        *head = current->next;
        free(current);
        return 0;
    }

    while (current->next != NULL && strcmp(current->next->name, name) != 0)
    {
        current = current->next;
    }

    if (current->next != NULL && strcmp(current->next->name, name) == 0)
    {
        client_node *temp = current->next;
        current->next = temp->next;
        free(temp);
        return 0;
    }
    return 1;
}