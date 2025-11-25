#include "cerver.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <snorkel.h>

Arena serv_arena = {0};
Arena msg_arena = {0};

__attribute__((noreturn)) void http_loop(HttpServer *cerver){
	for(;;){
		int clientfd = accept(cerver->fd, 0, 0);
		if(clientfd < 0){
			perror("accept error");
			continue;
		}

		HttpHeader req_header = recv_header(&msg_arena, clientfd);
		HttpHeader res_header = write_res_header(&msg_arena,
				&req_header, cerver->root);

		send_header(&msg_arena, &res_header, clientfd);
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
		arena_reset(&msg_arena);
	}
}

int setup_server(HttpServer *server){
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server->fd == -1){
		perror("socket error");
		return 0;
	}
	int yes = 1;
	setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	struct addrinfo *addr, *p, hint = {0};
	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	getaddrinfo(NULL, "8666", &hint, &addr);
	for(p = addr; p; p = p->ai_next){
		if(bind(server->fd, p->ai_addr, p->ai_addrlen) == 0){
			break;
		}
	}
	if(!p){
		perror("bind error");
		return 0;
	}
	freeaddrinfo(addr);
	if(listen(server->fd, 5)){
		perror("listen error");
		return 0;
	}
	return 1;
}

int main(int argc, char** argv){
	HttpServer cerver = {0};
	struct option options[2] = {0};
	options[0].name = "root";
	options[0].has_arg = required_argument;
	options[0].val = CUSTOM_ROOT;
	int opt;

	struct stat st = {0};
	while((opt=getopt_long(argc, argv, "", options, NULL)) != -1){
		switch(opt){
			case CUSTOM_ROOT:
				cerver.root = string_concat_bytes(&serv_arena, cerver.root,
						optarg, strlen(optarg));
				break;
			default:
				return -1;
		}
	}
	if(!cerver.root || cerver.root->bytes[0] != '/'){
		string *fixed_root = arena_create_string(&serv_arena, 255);
		getcwd(fixed_root->bytes, fixed_root->size);
		fixed_root->len = strnlen(fixed_root->bytes, 
				fixed_root->size);
		fixed_root = string_concat_bytes(&serv_arena, fixed_root, "/", 1);
		cerver.root = string_concat(&serv_arena, fixed_root, cerver.root);
	}
	if(cerver.root->bytes[cerver.root->len-1] == '/'){
		cerver.root->len--;
	}
	if(stat(cerver.root->bytes, &st) == -1 ||
		(st.st_mode & S_IFMT) != S_IFDIR){
		fprintf(stderr, "%s: %s is not a directory or does not exist\n",
				argv[0], cerver.root->bytes);
		return -1;
	}
	if(!setup_server(&cerver)){
		return -1;
	}
	http_loop(&cerver);
	return 0;
}
