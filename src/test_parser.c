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
	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len, "/path?q=1", 9);
	ASSERT_EQ_MEM(req.path.ptr, req.path.len, "/path", 5);
	ASSERT_EQ_MEM(req.query.ptr, req.query.len, "q=1", 3);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

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
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

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
	ASSERT_EQ_INT(req.target_form, TF_ABSOLUTE);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_MEM(req.raw_target.ptr, req.raw_target.len,
			"http://ex.com:80/path?q=1", 25);
	ASSERT_EQ_MEM(req.scheme.ptr, req.scheme.len, "http", 4);
	ASSERT_EQ_MEM(req.host.ptr, req.host.len, "ex.com", 6);
	ASSERT_EQ_MEM(req.authority.ptr, req.authority.len, "ex.com:80", 9);
	ASSERT_EQ_INT(req.port, 80);
	ASSERT_EQ_MEM(req.path.ptr, req.path.len, "/path", 5);
	ASSERT_EQ_MEM(req.query.ptr, req.query.len, "q=1", 3);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_MEM(
		req.headers[0].name.ptr,
		req.headers[0].name.len,
		"host", 4
	);
	ASSERT_EQ_INT(req.headers[0].type, HH_HOST);
	ASSERT_EQ_MEM(
		req.headers[0].value.ptr,
		req.headers[0].value.len,
		"ex.com", 6
	);

	parse_ctx_free(&ctx);
	http_request_free(&req);
}

int main(void) {
	RUN_TEST(test_get_origin);
	RUN_TEST(test_get_asterisk);
	RUN_TEST(test_get_absolute);
	return 0;
}
