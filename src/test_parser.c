#include "test.h"
#include "parser.h"
#include <stdio.h>

/* NOTE: some (most) tests ported form the llhttp test suite */

static void test_get_origin(void) {
	const char *raw_req = RL11("GET", "/path?q=1") HOST("ex.com") END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_DONE(ctx, res);

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);
	ASSERT_EQ_INT(req.keep_alive, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/path?q=1");
	ASSERT_SLICE_UNSET(req.scheme);
	ASSERT_SLICE_UNSET(req.authority);
	ASSERT_EQ_INT(req.port, 0);
	ASSERT_EQ_SLICE(req.path, "/path");
	ASSERT_EQ_SLICE(req.query, "q=1");

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	END_TEST(ctx, req);
}

static void test_get_asterisk(void) {
	const char *raw_req = RL11("OPTIONS", "*") HOST("ex.com") END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_DONE(ctx, res);

	ASSERT_EQ_INT(req.method, HM_OPTIONS);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);
	ASSERT_EQ_INT(req.keep_alive, 1);

	ASSERT_EQ_INT(req.target_form, TF_ASTERISK);
	ASSERT_EQ_SLICE(req.raw_target, "*");
	ASSERT_SLICE_UNSET(req.scheme);
	ASSERT_SLICE_UNSET(req.authority);
	ASSERT_EQ_INT(req.port, 0);
	ASSERT_SLICE_UNSET(req.path);
	ASSERT_SLICE_UNSET(req.query);

	ASSERT_EQ_INT(req.num_headers, 1);
	ASSERT_EQ_HEADER(&req, HH_HOST, "ex.com");

	END_TEST(ctx, req);
}

static void test_get_absolute(void) {
	const char *raw_req = RL11("GET", "http://ex.com:80/path?q=1") \
				HOST("ex.com") END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_DONE(ctx, res);

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);
	ASSERT_EQ_INT(req.keep_alive, 1);

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

	END_TEST(ctx, req);
}

static void test_curl_get(void) {
	const char *raw_req =
		"GET / HTTP/1.1" CRLF
		"Host: 127.0.0.1" CRLF
		"User-Agent: curl/8.15.0" CRLF
		"Accept: */*" CRLF END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_DONE(ctx, res);

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/");
	ASSERT_EQ_SLICE(req.path, "/");

	ASSERT_EQ_INT(req.num_headers, 3);
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

	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_DONE(ctx, res);

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);
	ASSERT_EQ_INT(req.keep_alive, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/");
	ASSERT_EQ_SLICE(req.path, "/");

	ASSERT_EQ_INT(req.num_headers, 12);
	ASSERT_EQ_HEADER(&req, HH_HOST, "127.0.0.1");
	ASSERT_EQ_HEADER(&req, HH_CONNECTION, "keep-alive");

	END_TEST(ctx, req);
}

static void test_get_no_headers_no_body(void) {
	const char *raw_req = RL11("GET", "/get_no_headers_no_body/world") END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_ERROR(res); /* no host is ambiguous */

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/get_no_headers_no_body/world");
	ASSERT_EQ_SLICE(req.path, "/get_no_headers_no_body/world");
	ASSERT_SLICE_UNSET(req.query);

	ASSERT_EQ_INT(req.num_headers, 0);

	END_TEST(ctx, req);
}

static void test_get_one_header_no_body(void) {
	const char *raw_req = RL11("GET", "/get_one_header_no_body") \
				H("Accept", "*/*") END;
	SETUP_TEST(raw_req, req, ctx, res);
	ASSERT_ERROR(res); /* no host is ambiguous */

	ASSERT_EQ_INT(req.method, HM_GET);
	ASSERT_EQ_INT(req.http_major, 1);
	ASSERT_EQ_INT(req.http_minor, 1);

	ASSERT_EQ_INT(req.target_form, TF_ORIGIN);
	ASSERT_EQ_SLICE(req.raw_target, "/get_one_header_no_body");
	ASSERT_EQ_SLICE(req.path, "/get_one_header_no_body");

	ASSERT_EQ_INT(req.num_headers, 1);

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
