#ifndef CERVER_H
#define CERVER_H

#define _GNU_SOURCE

#include <stdint.h>
#include <stddef.h>
#include <snorkel.h>

typedef enum {
	CUSTOM_ROOT
} Options;

typedef enum {
	HEAD,
	GET,
	POST,
	PUT,
	DELETE,
	BAD
} Method;

typedef enum {
	OK,
	BAD_REQUEST,
	NOT_FOUND,
	INTERNAL_ERROR
} Status;


typedef struct {
	Status status;
	string *http_v;
	string *status_str;
} ResponseLine;

typedef struct {
	Method method;
	string *path;
	string *http_v;
} RequestLine;

typedef union {
	RequestLine *req_line;
	ResponseLine *res_line;
} Message;

typedef struct {
	Message msg;
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
	string *root;
} HttpServer;

string* read_line(Arena*, int);
string* recv_line(Arena*, int);
HttpHeader read_header(Arena*, int);
HttpHeader recv_header(Arena*, int);
HttpHeader write_res_header(Arena*, HttpHeader*, string*);
void send_header(Arena*, HttpHeader*, int);
void send_resource(HttpHeader*, int);
#endif // CERVER_H
