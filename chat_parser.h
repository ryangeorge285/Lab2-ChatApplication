#include <string.h>
#include <stdio.h>

// declaring the different requests

typedef enum
{
    REQ_CONN,
    REQ_SAY,
    REQ_SAYTO,
    REQ_MUTE,
    REQ_UNMUTE,
    REQ_RENAME,
    REQ_DISCONN,
    REQ_KICK,
    REQ_INVALID
} request_type;

// storing the name and the msg
// picked the max length of the msg randomly
typedef struct
{
    request_type type;
    char name[256];
    char msg[512];
} request;

void parse_input(const char *input, request *r)
{
    r->type = REQ_INVALID;
    r->name[0] = '\0';
    r->msg[0] = '\0';

    char line[1024];
    char message[1024];
    strcpy(line, input);

    char *cmd = strtok(line, "$");
    printf("Parsed request: \n");
    printf(" %s\n", cmd);
    char *remaining = strtok(NULL, "");
    strcpy(message, remaining);
    char *recipient = strtok(remaining, " ");

    if (strcmp(cmd, "conn") == 0)
    {
        r->type = REQ_CONN;
        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }
    }
    else if (strcmp(cmd, "say") == 0)
    {
        r->type = REQ_SAY;
        char *msg_ptr = message;
        if (*msg_ptr== ' ') msg_ptr++;
        if (msg_ptr != NULL)
        {
            strcpy(r->msg, msg_ptr);
        }
    }
    else if (strcmp(cmd, "sayto") == 0)
    {
        r->type = REQ_SAYTO;

        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }

        char *message = strtok(NULL, "");
        printf(" %s\n", message);
        if (message != NULL)
        {
            strcpy(r->msg, message);
        }
    }
    else if (strcmp(cmd, "mute") == 0)
    {
        r->type = REQ_MUTE;
        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }
    }
    else if (strcmp(cmd, "unmute") == 0)
    {
        r->type = REQ_UNMUTE;
        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }
    }
    else if (strcmp(cmd, "rename") == 0)
    {
        r->type = REQ_RENAME;
        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }
    }
    else if (strcmp(cmd, "disconn") == 0)
    {
        r->type = REQ_DISCONN;
    }
    else if (strcmp(cmd, "kick") == 0)
    {
        r->type = REQ_KICK;
        if (recipient != NULL)
        {
            strcpy(r->name, recipient);
        }
    }
}