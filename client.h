#ifndef CLIENT_H
#define CLIENT_H

#include "cerver.h"

typedef struct {
	HttpServer *cerver;
	int fd;
} HttpClient;

void* http_reply(void*);
#endif // CLIENT_H
