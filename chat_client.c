
#include <stdio.h>
#include "udp.h"
#include "chat_parser.h"
#include <pthread.h>

#define CLIENT_PORT 10001

int sd;
struct sockaddr_in server_addr;


void *sender_thread(void *arg);
void *listener_thread(void *arg);


// client code
int main(int argc, char *argv[])
{
    sd = udp_socket_open(CLIENT_PORT);


    // Initializing the server's address.
    // We are currently running the server on localhost (127.0.0.1).
    // You can change this to a different IP address
    // when running the server on a different machine.
    // (See details of the function in udp.h)


    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);

    // Storage for request and response messages
    char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];

    // Creating two threads:
    pthread_t sender_tid, listener_tid;
    pthread_create(&sender_tid, NULL, sender_thread, NULL);
    pthread_create(&listener_tid, NULL, listener_thread, NULL);


    pthread_join(sender_tid, NULL);
    pthread_join(listener_tid, NULL);

    

    return 0;
}