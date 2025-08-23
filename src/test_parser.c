#include "test.h"

/* NOTE: some (most) tests ported form the llhttp test suite */

static void test_get_origin(void) {
	const char *raw_req = RL11("GET", "/path?q=1") HOST("ex.com") END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_ok(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_origin(&req, "/path?q=1", "/path", "q=1");

	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	END_TEST(ctx, req);
}

static void test_get_asterisk(void) {
	const char *raw_req = RL11("OPTIONS", "*") HOST("ex.com") END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_ok(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_OPTIONS, 1, 1, 1);
	assert_target_asterisk(&req);

	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	END_TEST(ctx, req);
}

static void test_get_absolute(void) {
	const char *raw_req = RL11("GET", "http://ex.com:80/path?q=1") \
				HOST("ex.com") END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_ok(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_absolute(&req, "http://ex.com:80/path?q=1", "http", "ex.com", 80, "ex.com:80", "/path", "q=1");

	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	END_TEST(ctx, req);
}

static void test_curl_get(void) {
	const char *raw_req =
		"GET / HTTP/1.1" CRLF
		"Host: 127.0.0.1" CRLF
		"User-Agent: curl/8.15.0" CRLF
		"Accept: */*" CRLF END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_ok(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_origin(&req, "/", "/", "");

	ASSERT_EQ_HEADER(&req, HH_HOST, "127.0.0.1");
	ASSERT_EQ_HEADER(&req, HH_USER_AGENT, "curl/8.15.0");
	ASSERT_EQ_HEADER(&req, HH_ACCEPT, "*/*");

	END_TEST(ctx, req);
}

static void test_firefox_get(void) {
	const char *raw_req =
		"GET / HTTP/1.1" CRLF
		"Host: 127.0.0.1" CRLF
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:141.0) "
			"Gecko/20100101 Firefox/141.0" CRLF
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,"
			"*/*;q=0.8" CRLF
		"Accept-Language: en-US,en;q=0.5" CRLF
		"Accept-Encoding: gzip, deflate, br, zstd" CRLF
		"Connection: keep-alive" CRLF
		"Upgrade-Insecure-Requests: 1" CRLF
		"Sec-Fetch-Dest: document" CRLF
		"Sec-Fetch-Mode: navigate" CRLF
		"Sec-Fetch-Site: none" CRLF
		"Sec-Fetch-User: ?1" CRLF
		"Priority: u=0, i" CRLF END;

	struct http_request req;
	struct parse_ctx ctx;
	const char *accept_encodings[] = {"gzip", "deflate", "br", "zstd"};

	ASSERT_TRUE(parse_ok(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_origin(&req, "/", "/", "");

	ASSERT_EQ_HEADER(&req, HH_HOST, "127.0.0.1");
	ASSERT_EQ_HEADER(&req, HH_CONNECTION, "keep-alive");
	assert_list_eq(&req, HH_ACCEPT_ENCODING, accept_encodings, 4);

	END_TEST(ctx, req);
}

static void test_get_no_headers_no_body(void) {
	const char *raw_req = RL11("GET", "/get_no_headers_no_body/world") END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_err(raw_req, &req, &ctx) == 0);

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_origin(&req, "/get_no_headers_no_body/world", "/get_no_headers_no_body/world", "");

	END_TEST(ctx, req);
}

static void test_get_one_header_no_body(void) {
	const char *raw_req = RL11("GET", "/get_one_header_no_body") \
				H("Accept", "*/*") END;
	struct http_request req;
	struct parse_ctx ctx;

	ASSERT_TRUE(parse_err(raw_req, &req, &ctx) == 0); /* no host is ambiguous */

	assert_req_line(&req, HM_GET, 1, 1, 1);
	assert_target_origin(&req, "/get_one_header_no_body", "/get_one_header_no_body", "");

	END_TEST(ctx, req);
}

int main(void) {
	RUN_TEST(test_get_origin);
	RUN_TEST(test_get_asterisk);
	RUN_TEST(test_get_absolute);
	RUN_TEST(test_curl_get);
	RUN_TEST(test_firefox_get);
	RUN_TEST(test_get_no_headers_no_body);
	RUN_TEST(test_get_one_header_no_body);
	return 0;
}
