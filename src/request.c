#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "str.h"

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

int slice_str_cmp_check(const struct slice *sl, const char *str) {
	if (sl->len != strlen(str)) return 1;
	return memcmp(sl->ptr, str, sl->len);
}

int slice_str_cmp_ci(const struct slice *sl, const char *str) {
	size_t pos = 0;

	assert(sl->len == strlen(str));
	for (; pos < sl->len; ++pos) {
		int diff = lower(sl->ptr[pos]) - lower(str[pos]);
		if (diff != 0) return diff;
	}
	return 0;
}

int slice_str_cmp_ci_check(const struct slice *sl, const char *str) {
	size_t pos = 0;

	if (sl->len != strlen(str)) return 1;
	for (; pos < sl->len; ++pos) {
		int diff = lower(sl->ptr[pos]) - lower(str[pos]);
		if (diff != 0) return diff;
	}
	return 0;
}

void http_request_free(struct http_request *req) {
	free(req->headers);
	free(req->h_index);
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

	new_req.h_index = malloc(sizeof(struct field_index));
	if (new_req.h_index == NULL) {
		free(new_req.headers);
		perror("new_request");
		exit(1);
	}

	memset(new_req.h_index->heads, 0xFF, HH__COUNT * sizeof(size_t));
	memset(new_req.h_index->tails, 0xFF, HH__COUNT * sizeof(size_t));
	memset(new_req.h_index->count, 0, HH__COUNT * sizeof(size_t));

	return new_req;
}

void append_empty_header(struct http_request *req, struct slice header_name) {
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
	new_header.next_same_type = SIZE_MAX;
	req->headers[req->num_headers++] = new_header;
}

struct http_header *get_header_by_name(
		struct http_request *req,
		const char *name
) {
	size_t i;
	size_t len;
	const size_t target_len = strlen(name);
	for (i = 0; i < req->num_headers; ++i) {
		len = req->headers[i].name.len;
		if (len == target_len &&
				!memcmp(name, req->headers[i].name.ptr, len)) {
			return req->headers + i;
		}
	}
	return NULL;
}

struct http_header *get_header(
		struct http_request *req,
		enum http_header_type type
) {
	size_t i;
	for (i = 0; i < req->num_headers; ++i) {
		if (req->headers[i].type == type) {
			return req->headers + i;
		}
	}
	return NULL;
}

void strip_postfix_ows(struct slice *header_value) {
	const char *end = header_value->ptr + header_value->len - 1;
	while (*end == ' ' || *end == '\t') end--;
	assert(end >= header_value->ptr);
	header_value->len = (size_t)(end - header_value->ptr + 1);
}

int is_http_ver(struct http_request *req, uint8_t major, uint8_t minor) {
	return req->http_major == major && req->http_minor == minor;
}

size_t headers_count(const struct http_request *req, enum http_header_type htype) {
	return req->h_index->count[htype];
}

size_t headers_first(const struct http_request *req, enum http_header_type htype) {
	return req->h_index->heads[htype];
}

size_t headers_next(const struct http_request *req, size_t idx) {
	return req->headers[idx].next_same_type;
}

static void trim_ows(struct slice *sl) {
	while (sl->len > 0 && (*sl->ptr == SYM_HTAB || *sl->ptr == SYM_SP)) {
		sl->ptr++;
		sl->len--;
	}
	while (sl->len > 0 && (sl->ptr[sl->len - 1] == SYM_HTAB || sl->ptr[sl->len - 1] == SYM_SP)) {
		sl->len--;
	}
}

struct header_item_iter header_items_init(
		const struct http_request *req,
		enum http_header_type htype
) {
	struct header_item_iter it = {0};
	it.header_index = req->h_index->heads[htype];
	it.offset = 0;
	return it;
}

int header_items_next(
		const struct http_request *req,
		struct header_item_iter *it
) {
	size_t pos;
	struct slice hval = req->headers[it->header_index].value;
	int is_quoting = 0;

	if (it->header_index == SIZE_MAX) {
		it->header_item.ptr = NULL;
		return 0;
	}

	if (it->header_item.ptr == NULL) {
		pos = 0;
		it->offset = 0;
		it->header_item.ptr = hval.ptr;
	} else if (it->offset >= hval.len) {
		size_t next = req->headers[it->header_index].next_same_type;
		if (it->last_comma) {
			it->header_item.len = 0;
			it->last_comma = 0;
			return 0;
		}
		if (next == SIZE_MAX) {
			it->header_item.ptr = NULL;
			return 0;
		}

		it->header_index = next;
		it->offset = 0;

		pos = 0;
		hval = req->headers[next].value;
		it->header_item.ptr = hval.ptr;
	} else {
		pos = it->offset;
	}

	while (pos < hval.len) {
		char ch = hval.ptr[pos];
		if (ch == '\"') {
			is_quoting ^= 1;
			pos++;
			continue;
		}
		if (ch == ',' && !is_quoting) {
			size_t off = pos;
			if (pos > it->offset) {
				while (--pos >= it->offset && (hval.ptr[pos] == SYM_HTAB || hval.ptr[pos] == SYM_SP));
				pos++;
			}
			while (++off < hval.len && (hval.ptr[off] == SYM_HTAB || hval.ptr[off] == SYM_SP));

			it->header_item.len = pos - it->offset;
			it->header_item.ptr = hval.ptr + it->offset;
			it->offset = off;
			if (off >= hval.len) it->last_comma = 1;
			trim_ows(&it->header_item);
			return 0;
		}
		if (ch == '\\' && is_quoting) {
			pos++;
			if (pos >= hval.len) return -1;
			ch = hval.ptr[pos];
			if (ch == SYM_HTAB || ch == SYM_SP || is_vchar(ch) || is_obs_text(ch)) {
				pos++;
				continue;
			} else {
				return -1;
			}
		}
		if (!is_qdtext(ch)) return -1;
		pos++;
	}

	if (is_quoting) return -1;

	if (pos == it->offset) {
		it->header_item.ptr = NULL;
		return 0;
	}

	it->header_item.ptr = hval.ptr + it->offset;
	it->header_item.len = pos - it->offset;
	it->offset = pos;
	trim_ows(&it->header_item);

	return 0;
}
