#include "cerver.h"
#include "client.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>

#define SNORKEL_IMPLEMENTATION
#include "snorkel/snorkel_arena.h"
#include "snorkel/snorkel_pool.h"

Arena serv_arena = {0};

void* tpool_alloc(size_t size) {
	arena_flag(&serv_arena);
	return arena_alloc(&serv_arena, size);
}

void* tpool_realloc(void *ptr, size_t size) {
	(void)ptr;
	arena_restore(&serv_arena);
	return tpool_alloc(size);
}

void tpool_free(void *ptr) {
	(void)ptr;
	return;
}

__attribute__((noreturn)) void http_loop(HttpServer *cerver) {
	snorkel_pool_inject_allocators(tpool_alloc, tpool_realloc, tpool_free);

	const int max_count = 1<<10;
	int count = 0;

	Arena client_arena = {0};
	Pool *t_pool = create_pool(8, max_count);
	for(;;) {
		if(count == max_count) {
			wait_pool(t_pool);
			fprintf(stderr, "resetting client arena\n");
			arena_reset(&client_arena);
			count = 0;
		}
		int clientfd = accept(cerver->fd, 0, 0);
		if(clientfd < 0){
			perror("accept error");
			continue;
		}
		HttpClient *c = arena_alloc(&client_arena, sizeof(*c));
		c->fd = clientfd;
		c->cerver = cerver;
		register_task(t_pool, 1, http_reply, c);
		count++;
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
	fprintf(stdout, "Server is running and listening on port 8666\n");

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
