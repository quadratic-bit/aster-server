#include "request.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct slice get_slice(const char *ptr, size_t len) {
	struct slice new_slice;
	new_slice.ptr = ptr;
	new_slice.len = len;
	return new_slice;
}

int slice_str_cmp(const struct slice *sl, const char *str) {
	assert(sl->len == strlen(str));
	return memcmp(sl->ptr, str, sl->len);
}

struct http_request new_request(void) {
	struct http_request new_req = {0};

	new_req.num_headers = 0;
	new_req.cap_headers = 4;

	new_req.headers = malloc(
		new_req.cap_headers * sizeof(struct http_header)
	);
	if (new_req.headers == NULL) {
		perror("new_request");
		exit(1);
	}

	return new_req;
}

void append_empty_header(
		struct http_request *req,
		struct slice header_name
) {
	struct http_header new_header;

	if (req->num_headers + 1 > req->cap_headers) {
		size_t new_cap = req->cap_headers * 2;

		req->headers = realloc(
			req->headers,
			new_cap * sizeof(struct http_header)
		);
		if (req->headers == NULL) {
			perror("append_to_buf");
			exit(1);
		}
		req->cap_headers = new_cap;
	}
	new_header.name = header_name;
	new_header.value.len = 0;
	new_header.value.ptr = NULL;
	new_header.type = HH_UNK;
	req->headers[req->num_headers++] = new_header;
}
