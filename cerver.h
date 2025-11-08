#ifndef CERVER_H
#define CERVER_H

#define _GNU_SOURCE

#include <stdint.h>
#include <stddef.h>
#include "arena.h"

typedef enum {
	HEAD,
	GET,
	POST,
	PUT,
	DELETE,
	BAD
} Method;

typedef struct {
	string *http_version;
	string *status;
} ResponseLine;

typedef struct {
	Method method;
	string *path;
	string *http_version;
} RequestLine;

typedef union {
	RequestLine *request_line;
	ResponseLine *response_line;
} Message;

typedef struct {
	Message message;
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
HttpHeader write_response_header(Arena*, HttpHeader*);
void send_header(Arena*, HttpHeader*, int);
void send_resource(HttpHeader*, int);
#endif // CERVER_H
