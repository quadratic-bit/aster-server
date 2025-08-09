#include "test.h"
#include "parser.h"

static void test_get_origin(void) {
	const char *raw_req = "GET /path?q=1 HTTP/1.1\r\nHost: ex.com\r\n\r\n";
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(ctx.state, PS_DONE);
	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "/path?q=1", 9);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"Host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

	parse_ctx_free(&ctx);
}

static void test_get_asterisk(void) {
	const char *raw_req = "OPTIONS * HTTP/1.1\r\nHost: ex.com\r\n\r\n";
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(ctx.state, PS_DONE);
	ASSERT_EQ_INT(req.method, HM_OPTIONS);
	ASSERT_EQ_INT(req.target_form, TF_ASTERISK);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "*", 1);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"Host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

	parse_ctx_free(&ctx);
}

static void test_get_absolute(void) {
	const char *raw_req = "GET http://ex.com:80/path?q=1 HTTP/1.1\r\n"
		"Host: ex.com\r\n\r\n";
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(ctx.state, PS_DONE);
	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.target_form, TF_ABSOLUTE);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len,
			"http://ex.com:80/path?q=1", 25);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"Host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

	parse_ctx_free(&ctx);
}

int main(void) {
	RUN_TEST(test_get_origin);
	RUN_TEST(test_get_asterisk);
	RUN_TEST(test_get_absolute);
	return 0;
}
