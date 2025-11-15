#include <string.h> 

// declaring the different requests

typedef enum {
    REQ_CONN,
    REQ_SAY,
    REQ_SAYTO,
    REQ_MUTE,
    REQ_UNMUTE,
    REQ_RENAME,
    REQ_DISCONN,
    REQ_KICK,
    REQ_INVALID
}request_type;


// storing the name and the msg 
// picked the max length of the msg randomly
typedef struct {
    request_type type;
    char arg1[256];
    char arg2[512];
} request;

void parse_input(const char *input, request *r);