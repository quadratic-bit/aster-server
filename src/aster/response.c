#include "response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct http_response new_response(void) {
	struct http_response new_resp = {0};

	new_resp.len = 0;
	new_resp.cap = 512;

	new_resp.buf = malloc(new_resp.cap);
	if (new_resp.buf == NULL) {
		perror("new_response");
		exit(1);
	}
	new_resp.buf[0] = '\0';

	return new_resp;
}

void append_to_response(struct http_response *resp, const char *str) {
	size_t to_append = strlen(str);
	if (resp->len + to_append + 1 > resp->cap) {
		size_t new_cap = resp->cap * 2;
		resp->buf = realloc(resp->buf, new_cap);
		if (resp->buf == NULL) {
			perror("append_to_response");
			exit(1);
		}
		resp->cap = new_cap;
	}
	strcat(resp->buf, str);
	resp->len += to_append;
}

void http_response_free(struct http_response *resp) {
	free(resp->buf);
}
