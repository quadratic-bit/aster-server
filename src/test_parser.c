#include "test.h"
#include "parser.h"

static void test_get_origin(void) {
	const char *raw_req = "GET /path?q=1 HTTP/1.1\r\nHost: ex.com\r\n\r\n";
	struct http_request req = {0};
	struct parse_ctx ctx = parse_ctx_init(&req);

	feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "/path?q=1", 9);

	parse_ctx_free(&ctx);
}

static void test_get_asterisk(void) {
	const char *raw_req = "OPTIONS * HTTP/1.1\r\nHost: ex.com\r\n\r\n";
	struct http_request req = {0};
	struct parse_ctx ctx = parse_ctx_init(&req);

	feed(&ctx, raw_req, strlen(raw_req));
	ASSERT_EQ_INT(req.method, HM_OPTIONS);
	ASSERT_EQ_INT(req.target_form, TF_ASTERISK);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "*", 1);

	parse_ctx_free(&ctx);
}

int main(void) {
	RUN_TEST(test_get_origin);
	RUN_TEST(test_get_asterisk);
	return 0;
}
