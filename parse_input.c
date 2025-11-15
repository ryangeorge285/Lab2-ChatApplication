#include "chat.h"
#include <string.h>
#include <stdio.h>

void parse_input(const char *input, request *r){
    r->type = REQ_INVALID;
    r->arg1[0] = '\0';
    r->arg2[0] = '\0';

    char line[1024];
    strcpy(line, input);

    char *cmd = strtok(line, "$");
    printf("Parsed request: \n");
    printf(" %s\n", cmd);
    char *remaining = strtok(NULL, "");
    char *recipient= strtok(remaining, " ");
    printf(" %s\n", recipient);



    if(strcmp(cmd, "conn") == 0){
        r->type = REQ_CONN;
        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }
        
    }
    else if (strcmp(cmd, "say") == 0){
        r->type = REQ_SAY;
        if (recipient != NULL){
            strcpy(r->arg2, recipient);
        }
        
    }
    else if (strcmp(cmd, "sayto") == 0){
        r->type = REQ_SAYTO;

        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }

        char *msg = strtok(NULL, "");
        printf(" %s\n", msg);
        if (msg != NULL){
            strcpy(r->arg2, msg);
        }
        
    }
    else if (strcmp(cmd, "mute") == 0){
        r->type = REQ_MUTE;
        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "unmute") == 0){
        r->type = REQ_UNMUTE;
        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "rename") == 0){
        r->type = REQ_RENAME;
        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }
    }
    else if (strcmp(cmd, "disconn") == 0){
        r->type = REQ_DISCONN;
    }
    else if (strcmp(cmd, "kick") == 0){
        r->type = REQ_KICK;
        if (recipient != NULL){
            strcpy(r->arg1, recipient);
        }
    }


}