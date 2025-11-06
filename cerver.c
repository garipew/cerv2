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
		if((at = string_find(line, (u8*)"\r\n", 2)) != -1){
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

string* read_line(Arena* arena, int fd){
	static u8 remaining[CHUNK_SIZE];
	u8 chunk[CHUNK_SIZE];
	string* line = string_concat(arena, NULL, remaining, CHUNK_SIZE);
	memset(remaining, 0, CHUNK_SIZE);
	int at;
	for(ssize_t bytes_recvd = 0; ;){
		if((at = string_find(line, (u8*)"\r\n", 2)) != -1){
			line->len = at+strlen("\r\n");
			string_to_bytes(line, remaining, line->len, CHUNK_SIZE);
			break;
		}
		if((bytes_recvd = read(fd, chunk, CHUNK_SIZE)) <= 0){
			break;
		}
		line = string_concat(arena, line, chunk, bytes_recvd); 
	}
	return line;
}

HttpHeader recv_header(Arena *arena, int fd){
	HttpHeader header = {0};
	string *str = recv_line(arena, fd);
	int at;
	for(;;){
		if(	string_find(str, (u8*) "GET", 3) == 0 ||
			string_find(str, (u8*) "HEAD", 4) == 0){
			header.method = string_concat(arena, NULL, str->bytes, str->len);
		}else if(string_find(str, (u8*) "Host: ", 6) == 0){
			header.host = string_concat(arena, NULL, str->bytes+6, str->len-6);
		}else if(string_find(str, (u8*) "User-Agent: ", 12) == 0){
			header.user_agent = string_concat(arena, NULL, str->bytes+12, str->len-12);
		}

		if((at = string_find(str, (u8*)"\r\n", 2)) == 0){
			break;
		}
		str = recv_line(arena, fd);
	} 
	return header;
}

HttpHeader read_header(Arena *arena, int fd){
	HttpHeader header = {0};
	string *str = read_line(arena, fd);
	int at;
	for(;;){
		if(	string_find(str, (u8*) "GET", 3) == 0 ||
			string_find(str, (u8*) "HEAD", 4) == 0){
			header.method = string_concat(arena, NULL, str->bytes, str->len);
		}else if(string_find(str, (u8*) "Host: ", 6) == 0){
			header.host = string_concat(arena, NULL, str->bytes+6, str->len-6);
		}else if(string_find(str, (u8*) "User-Agent: ", 12) == 0){
			header.user_agent = string_concat(arena, NULL, str->bytes+12, str->len-12);
		}

		if((at = string_find(str, (u8*)"\r\n", 2)) == 0){
			break;
		}
		str = read_line(arena, fd);
	} 
	return header;
}
