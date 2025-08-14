#include "request.h"
#include <stdint.h>

#define SYM_SP ' '
#define SYM_HTAB '\t'
#define SYM_CR '\r'
#define SYM_LF '\n'
#define CRLF "\r\n"
#define MARK_NONE SIZE_MAX

enum parse_state {
	PS_REQ_LINE_METHOD,
	PS_REQ_LINE_TARGET,
	PS_REQ_LINE_HTTP_NAME,
	PS_REQ_LINE_HTTP_MAJOR,
	PS_REQ_LINE_HTTP_PERIOD,
	PS_REQ_LINE_HTTP_MINOR,
	PS_REQ_LINE_CRLF,

	PS_FIELD_LINE_NAME,
	PS_FIELD_LINE_PRE_OWS,
	PS_FIELD_LINE_VALUE,

	PS_HOST,
	PS_FRAMING,
	PS_CONNECTION,

	PS_DONE,

	PS_ERROR
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
