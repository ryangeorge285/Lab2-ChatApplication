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
    char arg1[256];
    char arg2[512];
} request;

void parse_input(const char *input, request *r)
{
    r->type = REQ_INVALID;
    r->arg1[0] = '\0';
    r->arg2[0] = '\0';

    char line[1024];
    strcpy(line, input);

    char *cmd = strtok(line, "$");
    printf("Parsed request: \n");
    printf(" %s\n", cmd);
    char *remaining = strtok(NULL, "");
    char *recipient = strtok(remaining, " ");
    printf(" %s\n", recipient);

    if (strcmp(cmd, "conn") == 0)
    {
        r->type = REQ_CONN;
        if (recipient != NULL)
        {
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "say") == 0)
    {
        r->type = REQ_SAY;
        if (recipient != NULL)
        {
            strcpy(r->arg2, recipient);
        }
    }
    else if (strcmp(cmd, "sayto") == 0)
    {
        r->type = REQ_SAYTO;

        if (recipient != NULL)
        {
            strcpy(r->arg1, recipient);
        }

        char *msg = strtok(NULL, "");
        printf(" %s\n", msg);
        if (msg != NULL)
        {
            strcpy(r->arg2, msg);
        }
    }
    else if (strcmp(cmd, "mute") == 0)
    {
        r->type = REQ_MUTE;
        if (recipient != NULL)
        {
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "unmute") == 0)
    {
        r->type = REQ_UNMUTE;
        if (recipient != NULL)
        {
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "rename") == 0)
    {
        r->type = REQ_RENAME;
        if (recipient != NULL)
        {
            strcpy(r->arg1, recipient);
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
            strcpy(r->arg1, recipient);
        }
    }
}