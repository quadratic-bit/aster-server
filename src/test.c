#include <assert.h>
#include "test.h"
#include "request.h"

/* return 0 if no error emitted during parsing,
   and buffer consumption resulted in PR_COMPLETE */
int parse_ok(const char *raw, struct http_request *req, struct parse_ctx *ctx) {
	enum parse_result res;

	*req = new_request();
	*ctx = parse_ctx_init(req);
	res = feed(ctx, raw, strlen(raw));
	return res == PR_COMPLETE && ctx->state == PS_DONE ? 0 : -1;
}

/* return 0 if an error was emitted during parsing,
   and buffer consumption resulted in PR_COMPLETE */
int parse_err(const char *raw, struct http_request *req, struct parse_ctx *ctx) {
	enum parse_result res;

	*req = new_request();
	*ctx = parse_ctx_init(req);
	res = feed(ctx, raw, strlen(raw));
	return res == PR_COMPLETE && ctx->state == PS_ERROR ? 0 : -1;
}

void assert_req_line(
		const struct http_request *req,
		enum http_method hm,
		int http_major_ver,
		int http_minor_ver,
		int keepalive
) {
	ASSERT_EQ_INT(req->method, hm);
	ASSERT_EQ_INT(req->http_major, http_major_ver);
	ASSERT_EQ_INT(req->http_minor, http_minor_ver);
	ASSERT_EQ_INT(req->keep_alive, keepalive);
}

void assert_target_origin(
		const struct http_request *req,
		const char *raw,
		const char *path,
		const char *query
) {
	ASSERT_EQ_INT(req->target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req->raw_target, raw);
	ASSERT_EQ_SLICE(req->path, path);
	ASSERT_EQ_SLICE(req->query, query);
	ASSERT_SLICE_UNSET(req->scheme);
	ASSERT_SLICE_UNSET(req->authority);
	ASSERT_EQ_INT(req->port, 0);
}

void assert_target_asterisk(const struct http_request *req) {
	ASSERT_EQ_INT(req->target_form, TF_ASTERISK);
	ASSERT_EQ_SLICE(req->raw_target, "*");
	ASSERT_SLICE_UNSET(req->path);
	ASSERT_SLICE_UNSET(req->query);
	ASSERT_SLICE_UNSET(req->scheme);
	ASSERT_SLICE_UNSET(req->authority);
	ASSERT_EQ_INT(req->port, 0);
}

void assert_target_absolute(
		const struct http_request *req,
		const char *raw,
		const char *scheme,
		const char *host,
		int port,
		const char *authority,
		const char *path,
		const char *query
) {
	ASSERT_EQ_INT(req->target_form, TF_ABSOLUTE);
	ASSERT_EQ_SLICE(req->raw_target, raw);
	ASSERT_EQ_SLICE(req->scheme, scheme);
	ASSERT_EQ_SLICE(req->host, host);
	ASSERT_EQ_SLICE(req->authority, authority);
	ASSERT_EQ_SLICE(req->path, path);
	ASSERT_EQ_SLICE(req->query, query);
	ASSERT_EQ_INT(req->port, port);
}

void assert_list_eq(
		const struct http_request *req,
		enum http_header_type type,
		const char *items[],
		size_t n_items
) {
	struct header_item_iter it = header_items_init(req, type);
	struct slice value_slice;
	int it_ret;
	size_t counter;

	for (it_ret = header_items_next(req, &it), counter = 0;
			it.header_item.ptr != NULL && !it_ret && counter < n_items;
			it_ret = header_items_next(req, &it), counter++) {
		ASSERT_EQ_SLICE(it.header_item, items[counter]);
	}
	if (counter < n_items) {
		fprintf(stderr,
			"assert_list_eq failed (expected %ld values, got %ld)\n",
			n_items,
			counter);
		exit(1);
	}
	if (it.header_item.ptr != NULL) {
		fprintf(stderr,
			"assert_list_eq failed (expected %ld values, got more)\n",
			n_items);
		exit(1);
	}
	if (it_ret == -1) {
		fprintf(stderr, "assert_list_eq failed (malformed syntax)\n");
		exit(1);
	}
}
