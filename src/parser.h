#include "request.h"
#include <stdint.h>

#define SYM_SP ' '
#define SYM_CR '\r'
#define SYM_LF '\n'
#define TOK_CRLF "\r\n"
#define MARK_NONE SIZE_MAX

enum parse_state {
	PAR_REQ_LINE_METHOD,
	PAR_REQ_LINE_TARGET,
	PAR_REQ_LINE_HTTP_NAME,
	PAR_REQ_LINE_HTTP_MAJOR,
	PAR_REQ_LINE_HTTP_PERIOD,
	PAR_REQ_LINE_HTTP_MINOR,
	PAR_REQ_LINE_CRLF,

	PAR_DONE,

	PAR_ERROR
};

enum parse_result {
	PR_COMPLETE,
	PR_NEED_MORE
};

struct parse_ctx {
	enum parse_state state;

	char *buf;
	size_t len;
	size_t cap;

	size_t pos;
	size_t mark;

	struct http_request *req;
};

struct parse_ctx parse_ctx_init(struct http_request *req);
void parse_ctx_free(struct parse_ctx *ctx);

enum parse_result feed(struct parse_ctx *ctx, const char *req_bytes, size_t n);
