#include "arena.h"
#include "cerver.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

Arena general = {0};
int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		perror("socket error");
		return -1;
	}
	int yes = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	struct addrinfo *addr, *p, hint = {0};
	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	getaddrinfo(NULL, "8666", &hint, &addr);
	for(p = addr; p; p = p->ai_next){
		if(bind(fd, p->ai_addr, p->ai_addrlen) == 0){
			break;
		}
	}
	if(!p){
		perror("bind error");
		return -1;
	}
	freeaddrinfo(addr);
	if(listen(fd, 5)){
		perror("listen error");
		return -1;
	}

	for(;;){
		int clientfd = accept(fd, 0, 0);
		if(clientfd < 0){
			perror("accept error");
			return -1;
		}

		HttpHeader req_header = recv_header(&general, clientfd);
		HttpHeader res_header = write_res_header(&general, &req_header);

		send_header(&general, &res_header, clientfd);
		if(req_header.msg.req_line){
			if(	req_header.msg.req_line->method == GET &&
				res_header.msg.res_line->status == OK){
				send_resource(&req_header, clientfd);
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
		close(clientfd);
		arena_reset(&general);
	}

	close(fd);
	return 0;
}
