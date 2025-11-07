#include "cerver.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#define CHUNK_SIZE 8 

Method get_method_from_string(string *str){
	if(string_find(str, 0, "GET", 3) == 0){
		return GET;
	}
	if(string_find(str, 0, "HEAD", 3) == 0){
		return HEAD;
	}
	if(string_find(str, 0, "POST", 3) == 0){
		return POST;
	}
	if(string_find(str, 0, "PUT", 3) == 0){
		return PUT;
	}
	if(string_find(str, 0, "DELETE", 3) == 0){
		return DELETE;
	}
	return BAD;
}

string* recv_line(Arena* arena, int fd){
	static char remaining[CHUNK_SIZE];
	char chunk[CHUNK_SIZE];
	string* line = string_concat_bytes(arena, NULL, remaining, CHUNK_SIZE);
	memset(remaining, 0, CHUNK_SIZE);
	int at;
	for(ssize_t bytes_recvd = 0; ;){
		if((at = string_find(line, 0, "\r\n", 2)) != -1){
			line->len = at+strlen("\r\n");
			string_to_bytes(line, remaining, line->len, CHUNK_SIZE);
			break;
		}
		if((bytes_recvd = recv(fd, chunk, CHUNK_SIZE, 0)) <= 0){
			break;
		}
		line = string_concat_bytes(arena, line, chunk, bytes_recvd); 
	}
	return line;
}

RequestLine* new_request_line(Arena *arena, string *header_str){
	RequestLine *rl = arena_alloc(arena, sizeof(*rl), ALIGNOF(*rl));

	char *bytes = header_str->bytes;
	int total_offset, relative_offset = total_offset = 0;
	total_offset = string_find(header_str, 0, " ", 1);
	if(total_offset < 0){
		return NULL;
	}
	relative_offset = total_offset - (bytes - header_str->bytes);
	rl->method = get_method_from_string(string_concat_bytes(arena, NULL, bytes, relative_offset));

	relative_offset++; 
	total_offset++; 
	bytes += relative_offset;

	total_offset = string_find(header_str, total_offset, " ", 1);
	if(total_offset < 0){
		return NULL;
	}
	relative_offset = total_offset - (bytes - header_str->bytes);
	rl->path = string_concat_bytes(arena, NULL, bytes, relative_offset);

	relative_offset++;
	total_offset++;
	bytes+=relative_offset;

	total_offset = string_find(header_str, total_offset, "\r\n", 2); 
	relative_offset = total_offset - (bytes - header_str->bytes);
	if(total_offset < 0 || relative_offset <= 0){
		return NULL;
	}
	rl->http_version = string_concat_bytes(arena, NULL, bytes, relative_offset);

	return rl;
}

HttpHeader recv_header(Arena *arena, int fd){
	HttpHeader header = {0};
	string *str = recv_line(arena, fd);
	int at;
	for(;;){
		if(	string_find(str, 0,  "GET", 3) == 0 ||
			string_find(str, 0,  "HEAD", 4) == 0){
			header.message.request_line = new_request_line(arena, str);
			if(!header.message.request_line){
				fprintf(stderr, "Bad Request\n400?\n\n");
			}
		}else if(string_find(str, 0,  "Host: ", 6) == 0){
			header.host = string_concat_bytes(arena, NULL, str->bytes+6, str->len-6);
		}else if(string_find(str, 0,  "User-Agent: ", 12) == 0){
			header.user_agent = string_concat_bytes(arena, NULL, str->bytes+12, str->len-12);
		}

		if((at = string_find(str, 0, "\r\n", 2)) == 0){
			break;
		}
		str = recv_line(arena, fd);
	} 
	return header;
}

ResponseLine* new_response_line_from_bytes(Arena *arena, char *bytes, size_t len){
	ResponseLine *rl = arena_alloc(arena, sizeof(*rl), ALIGNOF(*rl));
	rl->http_version = string_concat_bytes(arena, NULL, "HTTP/1.1", 8);
	rl->status = string_concat_bytes(arena, NULL, bytes, len);
	return rl;
}

HttpHeader write_response_header(Arena *arena, HttpHeader *request_header){
	HttpHeader response = {0};
	if(!request_header || request_header->message.request_line->method == BAD){
		response.message.response_line =  new_response_line_from_bytes(arena, "400 Bad Request", 16);
		return response;
	}
	struct stat status = {0}; 
	response.server = arena_create_string(arena, 255);
	gethostname(response.server->bytes, response.server->size);
	response.server->len = strnlen(response.server->bytes, response.server->size);
	switch(request_header->message.request_line->method){
		case GET:
		case HEAD:
			if(!stat(request_header->message.request_line->path->bytes, &status)){
				response.message.response_line = new_response_line_from_bytes(arena, "200 Ok", 6);
				response.content_length = status.st_size;
				return response;
			}else if(errno == ENOENT){
				response.message.response_line = new_response_line_from_bytes(arena, "404 Not Found", 13);
				return response;
			}
			break;
		case PUT:
			break;
		case POST:
			break;
		case DELETE:
			break;
		default:
			break;
	}
	return response;
}

void send_header(Arena *arena, HttpHeader *header, int fd){
	if(!header->message.response_line){
		header->message.response_line = new_response_line_from_bytes(arena, "500 Internal Server Error", 25);
	}
	int added_len;

	string *header_str = string_concat_bytes(arena, header->message.response_line->http_version, " ", 1);
	header_str = string_concat(arena, header_str, header->message.response_line->status);
	header_str = string_concat_bytes(arena, header_str, "\r\n", 2);

	if(header->content_length > 0){
		// TODO(garipew): This string creation allocates space enough to hold the content length in it's binary
		// representation. Obviously, this is greater than the space needed to hold it's decimal representation.
		// TLDR; Should the context present tight memory constraints, this allocation could be shortened.
		string *conlen_str =  arena_create_string(arena, (sizeof(size_t)*8)+strlen("Content-Length: "));
		conlen_str = string_concat_bytes(arena, conlen_str, "Content-Length: ", strlen("Content-Length: "));

		// TODO(garipew): This here breaks the entire point of the custom API... I shouldn't use stdio functions
		// nor manipulate string->bytes directly.
		sprintf(conlen_str->bytes+conlen_str->len, "%lu%n", header->content_length, &added_len);
		conlen_str->len+=added_len;

		conlen_str = string_concat_bytes(arena, conlen_str, "\r\n", 2);
		header_str = string_concat(arena, header_str, conlen_str);
	}

	if(header->server){
		header_str = string_concat_bytes(arena, header_str, "Server: ", 8); 
		header_str = string_concat(arena, header_str, header->server); 
		header_str = string_concat_bytes(arena, header_str, "\r\n", 2);
	}

	header_str = string_concat_bytes(arena, header_str, "\r\n", 2);
	for(size_t bytes_sent = 0; bytes_sent < header_str->len; ){
		bytes_sent+=send(fd, header_str->bytes+bytes_sent, header_str->len-bytes_sent, 0);
	}
}
