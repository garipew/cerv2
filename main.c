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
	getaddrinfo(NULL, "6666", &hint, &addr);
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

		HttpHeader header = recv_header(&general, clientfd);
		printf("%s", header.method->bytes);
		if(header.host){
			printf("Host: %s", header.host->bytes);
		}
		if(header.user_agent){
			printf("User-Agent: %s", header.user_agent->bytes);
		}
		printf("\r\n");
		close(clientfd);
		arena_reset(&general);
	}

	close(fd);
	return 0;
}
