#include "test.h"
#include "parser.h"

static void test_get_origin(void) {
	const char *raw_req = RL11("GET", "/path?q=1") HOST("ex.com") END;
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/path?q=1");
	ASSERT_EQ_SLICE(req.path, "/path");
	ASSERT_EQ_SLICE(req.query, "q=1");

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	parse_ctx_free(&ctx);
	http_request_free(&req);
}

static void test_get_asterisk(void) {
	const char *raw_req = RL11("OPTIONS", "*") HOST("ex.com") END;
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(req.method, HM_OPTIONS);
	ASSERT_EQ_INT(req.target_form, TF_ASTERISK);
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "*", 1);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	parse_ctx_free(&ctx);
	http_request_free(&req);
}

static void test_get_absolute(void) {
	const char *raw_req = RL11("GET", "http://ex.com:80/path?q=1") \
				HOST("ex.com") END;
	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);

	enum parse_result res = feed(&ctx, raw_req, strlen(raw_req));

	ASSERT_EQ_INT(res, PR_COMPLETE);
	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.target_form, TF_ABSOLUTE);
	ASSERT_EQ_SLICE(req.raw_target, "http://ex.com:80/path?q=1");
	ASSERT_EQ_SLICE(req.scheme, "http");
	ASSERT_EQ_SLICE(req.host, "ex.com");
	ASSERT_EQ_SLICE(req.authority, "ex.com:80");
	ASSERT_EQ_INT(req.port, 80);
	ASSERT_EQ_SLICE(req.path, "/path");
	ASSERT_EQ_SLICE(req.query, "q=1");

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	parse_ctx_free(&ctx);
	http_request_free(&req);
}

int main(void) {
	RUN_TEST(test_get_origin);
	RUN_TEST(test_get_asterisk);
	RUN_TEST(test_get_absolute);
	return 0;
}
