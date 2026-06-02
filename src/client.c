#include <unistd.h>
#include <stdio.h>
#include "snorkel/snorkel_arena.h"
#include "cerver.h"
#include "client.h"

void* http_reply(void *client) {
	Arena reply_arena = {0};

	HttpClient *c = client;
	HttpHeader req_header = recv_header(&reply_arena, c->fd);
	HttpHeader res_header = write_res_header(&reply_arena,
			&req_header, c->cerver->root);

	send_header(&reply_arena, &res_header, c->fd);
	if(req_header.msg.req_line){
		if(	req_header.msg.req_line->method == GET &&
				res_header.msg.res_line->status == OK){
			send_resource(&req_header, c->fd);
		}
		printf("%d ", req_header.msg.req_line->method);
		printf("%.*s ", (int)req_header.msg.req_line->path->len,
				req_header.msg.req_line->path->bytes);
		printf("%.*s\n", 
				(int)req_header.msg.req_line->http_v->len,
				req_header.msg.req_line->http_v->bytes);

		if(req_header.host){
			printf("Host: %s", req_header.host->bytes);
		}
		if(req_header.user_agent){
			printf("User-Agent: ");
			printf("%.*s", (int)req_header.user_agent->len,
					req_header.user_agent->bytes);
		}
		printf("\r\n");
	}
	close(c->fd);

	arena_free(&reply_arena);
	return client;
}
