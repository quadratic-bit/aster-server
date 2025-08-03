#include "test.h"
#include "parser.h"

static void test_simple_get(void) {
	const char *raw_req = "GET /path?q=1 HTTP/1.1\r\nHost: ex.com\r\n\r\n";
	struct http_request req = {0};
	struct parse_ctx ctx = parse_ctx_init(&req);
	feed(&ctx, raw_req, strlen(raw_req));
	ASSERT_EQ_INT(req.method, HM_GET);
}

int main(void) {
	test_simple_get();
	fprintf(stderr, "OK\n");
	return 0;
}
