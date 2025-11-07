#include "cerver.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#define CHUNK_SIZE 8 

string* recv_line(Arena* arena, int fd){
	static u8 remaining[CHUNK_SIZE];
	u8 chunk[CHUNK_SIZE];
	string* line = string_concat(arena, NULL, remaining, CHUNK_SIZE);
	memset(remaining, 0, CHUNK_SIZE);
	int at;
	for(ssize_t bytes_recvd = 0; ;){
		if((at = string_find(line, 0, (u8*)"\r\n", 2)) != -1){
			line->len = at+strlen("\r\n");
			string_to_bytes(line, remaining, line->len, CHUNK_SIZE);
			break;
		}
		if((bytes_recvd = recv(fd, chunk, CHUNK_SIZE, 0)) <= 0){
			break;
		}
		line = string_concat(arena, line, chunk, bytes_recvd); 
	}
	return line;
}

RequestLine* new_request_line(Arena *arena, string *rl_str){
	RequestLine *rl = arena_alloc(arena, sizeof(*rl), ALIGNOF(*rl));

	u8 *bytes = rl_str->bytes;
	int total_offset, relative_offset = total_offset = 0;
	total_offset = string_find(rl_str, 0, (u8*)" ", 1);
	if(total_offset < 0){
		return NULL;
	}
	relative_offset = total_offset - (bytes - rl_str->bytes);
	rl->method = string_concat(arena, NULL, bytes, relative_offset);

	relative_offset++; 
	total_offset++; 
	bytes += relative_offset;

	total_offset = string_find(rl_str, total_offset, (u8*)" ", 1);
	if(total_offset < 0){
		return NULL;
	}
	relative_offset = total_offset - (bytes - rl_str->bytes);
	rl->path = string_concat(arena, NULL, bytes, relative_offset);

	relative_offset++;
	total_offset++;
	bytes+=relative_offset;

	total_offset = string_find(rl_str, total_offset, (u8*)"\r\n", 2); 
	relative_offset = total_offset - (bytes - rl_str->bytes);
	if(total_offset < 0 || relative_offset <= 0){
		return NULL;
	}
	rl->http_version = string_concat(arena, NULL, bytes, relative_offset);

	return rl;
}

HttpHeader recv_header(Arena *arena, int fd){
	HttpHeader header = {0};
	string *str = recv_line(arena, fd);
	int at;
	for(;;){
		if(	string_find(str, 0, (u8*) "GET", 3) == 0 ||
			string_find(str, 0, (u8*) "HEAD", 4) == 0){
			header.request_line = new_request_line(arena, str);
			if(!header.request_line){
				fprintf(stderr, "Bad Request\n400?\n\n");
			}
		}else if(string_find(str, 0, (u8*) "Host: ", 6) == 0){
			header.host = string_concat(arena, NULL, str->bytes+6, str->len-6);
		}else if(string_find(str, 0, (u8*) "User-Agent: ", 12) == 0){
			header.user_agent = string_concat(arena, NULL, str->bytes+12, str->len-12);
		}

		if((at = string_find(str, 0, (u8*)"\r\n", 2)) == 0){
			break;
		}
		str = recv_line(arena, fd);
	} 
	return header;
}
