
#include <stdio.h>
#include <string.h>
#include "udp.h"
#include "chat_parser.h"
#include <pthread.h>

#define CLIENT_PORT 10001

int sd;
struct sockaddr_in server_addr;
char logfile_name[64];

volatile int running = 1; // flag to control the running state of threads

void *sender_thread(void *arg);
void *listener_thread(void *arg);


// client code
int main(int argc, char *argv[])
{

    // checking if admin was requested
    if(argc > 1 && strcmp(argv[1], "admin") == 0)
    {
        sd = udp_socket_open(6666);
        printf("You are ADMIN on port 6666\n");
    } 
    else
    {
        sd = udp_socket_open(0); // open a UDP socket on any available port
    }


    

    // Initializing the server's address.
    // We are currently running the server on localhost (127.0.0.1).
    // You can change this to a different IP address
    // when running the server on a different machine.
    // (See details of the function in udp.h)

    int rc = set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);

    // Get the client's own port number
    struct sockaddr_in me;
    socklen_t len = sizeof(me);
    getsockname(sd, (struct sockaddr *)&me, &len);
    int my_port = ntohs(me.sin_port);

    // build filename e.g. iChat_49123.txt
    sprintf(logfile_name, "iChat_%d.txt", my_port);

    printf("Your client log file: %s\n", logfile_name);

    // Storage for request and response messages
    char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];

    // Creating two threads:
    pthread_t sender_tid, listener_tid;
    pthread_create(&sender_tid, NULL, sender_thread, NULL);
    pthread_create(&listener_tid, NULL, listener_thread, NULL);

    pthread_join(sender_tid, NULL);

    running = 0; // signal the listener thread to stop
    pthread_join(listener_tid, NULL);

    // close the socket
    close(sd);
    return 0;
}

void *listener_thread(void *arg)
{
    char server_response[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    // Listening for incoming messages from the server
    while (running)
    {
        int rc = udp_socket_read(sd, &from_addr, server_response, BUFFER_SIZE);

        if (rc > 0)
        {
            if (strcmp(server_response, "ping$") == 0)
            {
                char client_request[BUFFER_SIZE];
                strcpy(client_request, "ret-ping$");
                udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
                continue;
            }

            // log the message to the logfile
            FILE *fp = fopen(logfile_name, "a");
            if (fp)
            {
                // append the message to the logfile
                fprintf(fp, "%s\n", server_response);
                fclose(fp);
            }

            printf("%s\n", server_response);
        }
    }
    return NULL;
}

void *sender_thread(void *arg)
{
    char client_request[BUFFER_SIZE];
    while (running)
    {

        if (fgets(client_request, BUFFER_SIZE, stdin) == NULL)
            continue;
        client_request[strcspn(client_request, "\n")] = '\0';

        request req;
        parse_input(client_request, &req);

        // send the request to the server
        udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
        if (req.type == REQ_DISCONN)
        {
            running = 0; // signal to stop the sender thread
        }
    }
    return NULL;
}
