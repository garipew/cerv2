#ifndef CERVER_H
#define CERVER_H

#include <stdint.h>
#include <stddef.h>
#include "arena.h"

typedef struct {
	string *method;
	string *path;
	string *http_version;
} RequestLine;

typedef struct {
	RequestLine *request_line;
	string *user_agent;
	string *host;
	string *server;
	size_t content_length;
} HttpHeader;

typedef struct {
	HttpHeader *header;
	u8 *body;
} HttpRequest;

typedef struct {
	int fd;
} HttpServer;

string* read_line(Arena*, int);
string* recv_line(Arena*, int);
HttpHeader read_header(Arena*, int);
HttpHeader recv_header(Arena*, int);
#endif // CERVER_H
