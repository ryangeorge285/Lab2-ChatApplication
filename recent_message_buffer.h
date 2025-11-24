#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECENT_MESSAGES 15
#ifndef RMB_BUFFER_SIZE
#define RMB_BUFFER_SIZE 512
#endif

char recent_message_buffer[MAX_RECENT_MESSAGES][RMB_BUFFER_SIZE];
int recent_message_buffer_head = 0;
int messages_present = 0;
pthread_rwlock_t recent_message_buffer_lock;

void rmb_add_message(char *message)
{
    strcpy(recent_message_buffer[recent_message_buffer_head], message);
    recent_message_buffer_head = (recent_message_buffer_head + 1) % MAX_RECENT_MESSAGES;
    if (messages_present < MAX_RECENT_MESSAGES)
        messages_present++;
}

int rmb_get_messages(char arr[][RMB_BUFFER_SIZE])
{
    int start = (recent_message_buffer_head - messages_present + MAX_RECENT_MESSAGES) % MAX_RECENT_MESSAGES;
    for (int i = 0; i < messages_present; ++i)
    {
        int idx = (start + i) % MAX_RECENT_MESSAGES;
        strcpy(arr[i], recent_message_buffer[idx]);
    }
    return messages_present;
}