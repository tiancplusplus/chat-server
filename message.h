#ifndef __MESSAGE__
#define __MESSAGE__

#define MAX_MESSAGE_TOTAL 100 
#define MAX_CONNECTION 100
struct message{
	char name[256];
	char msg[1024];
};

typedef struct message message_t;

#endif
