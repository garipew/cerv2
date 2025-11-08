#include "cerver.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define CHUNK_SIZE 8 

Method get_method_from_bytes(char *b, size_t len){
	if(len >= 3 && !strncmp(b, "GET", 3)){
		return GET;
	}
	if(len >= 4 && !strncmp(b, "HEAD", 4)){
		return HEAD;
	}
	if(len >= 4 && !strncmp(b, "POST", 4)){
		return POST;
	}
	if(len >= 3 && !strncmp(b, "PUT", 3)){
		return PUT;
	}
	if(len >= 6 && !strncmp(b, "DELETE", 6)){
		return DELETE;
	}
	return BAD;
}

string* recv_line(Arena* a, int fd){
	static char remaining[CHUNK_SIZE];
	char chunk[CHUNK_SIZE];
	string* line = string_concat_bytes(a, NULL, remaining, CHUNK_SIZE);
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
		line = string_concat_bytes(a, line, chunk, bytes_recvd); 
	}
	return line;
}

RequestLine* new_req_line(Arena *a, string *header_str){
	RequestLine *rl = arena_alloc(a, sizeof(*rl), ALIGNOF(*rl));

	char *bytes = header_str->bytes;
	int total_offset, rel_offset = total_offset = 0;
	total_offset = string_find(header_str, 0, " ", 1);
	if(total_offset < 0){
		return NULL;
	}
	rel_offset = total_offset - (bytes - header_str->bytes);
	rl->method = get_method_from_bytes(bytes, rel_offset);

	rel_offset++; 
	total_offset++; 
	bytes += rel_offset;

	total_offset = string_find(header_str, total_offset, " ", 1);
	if(total_offset < 0){
		return NULL;
	}
	rel_offset = total_offset - (bytes - header_str->bytes);
	rl->path = string_concat_bytes(a, NULL, bytes, rel_offset);

	rel_offset++;
	total_offset++;
	bytes+=rel_offset;

	total_offset = string_find(header_str, total_offset, "\r\n", 2); 
	rel_offset = total_offset - (bytes - header_str->bytes);
	if(total_offset < 0 || rel_offset <= 0){
		return NULL;
	}
	rl->http_v = string_concat_bytes(a, NULL, bytes, rel_offset);

	return rl;
}

HttpHeader recv_header(Arena *a, int fd){
	HttpHeader header = {0};
	string *str = recv_line(a, fd);
	int at;
	for(;;){
		if(	string_find(str, 0,  "GET", 3) == 0 ||
			string_find(str, 0,  "HEAD", 4) == 0){
			header.msg.req_line = new_req_line(a, str);
			if(!header.msg.req_line){
				fprintf(stderr, "Bad Request\n400?\n\n");
			}
		}else if(string_find(str, 0,  "Host: ", 6) == 0){
			header.host = string_substr(a, str, 6, -1);
		}else if(string_find(str, 0,  "User-Agent: ", 12) == 0){
			header.user_agent = string_substr(a, str, 12, -1);
		}

		if((at = string_find(str, 0, "\r\n", 2)) == 0){
			break;
		}
		str = recv_line(a, fd);
	} 
	return header;
}

string* get_string_from_status(Arena *a, Status status){
	char *msgs[] = {
		"200 Ok",
		"400 Bad Request",
		"404 Not Found",
		"500 Internal Server Error"
	};
	char *msg = msgs[status];
	return string_concat_bytes(a, NULL, msg, strlen(msg));
}

ResponseLine* new_res_line(Arena *a, Status status){
	ResponseLine *rl = arena_alloc(a, sizeof(*rl), ALIGNOF(*rl));
	rl->http_v = string_concat_bytes(a, NULL, "HTTP/1.0", 8);
	rl->status_str = get_string_from_status(a, status);
	rl->status = status;
	return rl;
}

string* gethoststr(Arena *a){
	string *s = arena_create_string(a, 255);
	gethostname(s->bytes, s->size);
	s->len = strnlen(s->bytes, s->size);
	return s;
}

HttpHeader write_res_header(Arena *a, HttpHeader *req_header){
	HttpHeader response = {0};
	struct stat status = {0}; 
	if(!req_header || req_header->msg.req_line->method == BAD){
		response.msg.res_line = new_res_line(a, BAD_REQUEST);
		return response;
	}
	string *path = req_header->msg.req_line->path;

	response.server = gethoststr(a);

	switch(req_header->msg.req_line->method){
		case GET:
		case HEAD:
			path = string_ensure_terminator(a, path);
			req_header->msg.req_line->path = path;
			if(!stat(path->bytes, &status) &&
				(status.st_mode & S_IFMT) == S_IFREG){
				response.msg.res_line = new_res_line(a, OK);
				response.content_length = status.st_size;
				return response;
			}
			perror(path->bytes);
			response.msg.res_line = new_res_line(a, NOT_FOUND);
			return response;
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

void ensure_send(int fd, char *bytes, size_t len){
	for(size_t bytes_sent = 0; bytes_sent < len; ){
		bytes_sent+=send(fd, bytes+bytes_sent, len-bytes_sent, 0);
	}
}

void send_header(Arena *a, HttpHeader *hdr, int fd){
	if(!hdr->msg.res_line){
		hdr->msg.res_line = new_res_line(a, INTERNAL_ERROR);
	}
	int added_len;

	string *hdr_str = string_concat(a, NULL, hdr->msg.res_line->http_v);
	hdr_str = string_concat_bytes(a, hdr_str, " ", 1);
	hdr_str = string_concat(a, hdr_str, hdr->msg.res_line->status_str);
	hdr_str = string_concat_bytes(a, hdr_str, "\r\n", 2);

	if(hdr->content_length > 0){
		// TODO(garipew): This string creation allocates space enough to
		// hold the content length in it's binary representation.
		// Obviously, this is greater than the space needed to hold it's
		// decimal representation.
		// TLDR; Should the context present tight memory constraints,
		// this allocation could be shortened.
		size_t size = (sizeof(size_t)*8)+strlen("Content-Length: ");
		string *conlen_str =  arena_create_string(a, size);
		conlen_str = string_concat_bytes(a, conlen_str,
				"Content-Length: ", strlen("Content-Length: "));

		// TODO(garipew): This here breaks the entire point of the
		// custom API... I shouldn't use stdio functions nor manipulate
		// string->bytes directly... Find a better solution.
		sprintf(conlen_str->bytes+conlen_str->len, "%lu%n",
				hdr->content_length, &added_len);
		conlen_str->len+=added_len;

		conlen_str = string_concat_bytes(a, conlen_str, "\r\n", 2);
		hdr_str = string_concat(a, hdr_str, conlen_str);
	}

	if(hdr->server){
		hdr_str = string_concat_bytes(a, hdr_str, "Server: ", 8); 
		hdr_str = string_concat(a, hdr_str, hdr->server); 
		hdr_str = string_concat_bytes(a, hdr_str, "\r\n", 2);
	}

	hdr_str = string_concat_bytes(a, hdr_str, "\r\n", 2);
	ensure_send(fd, hdr_str->bytes, hdr_str->len);
}

void send_resource(HttpHeader *req_header, int fd){
	FILE *res = fopen(req_header->msg.req_line->path->bytes, "r");
	char chunk[CHUNK_SIZE];
	for(size_t b_read; ; ){
		if((b_read = fread(chunk, sizeof(char), CHUNK_SIZE, res)) <= 0){
			break;
		}
		ensure_send(fd, chunk, b_read);
	}
	fclose(res);
}
