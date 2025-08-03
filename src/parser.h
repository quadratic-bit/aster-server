#include "request.h"
#include <stdint.h>

#define SYM_SP 0x20
#define MARK_NONE SIZE_MAX

enum parse_state {
	PAR_REQ_LINE_METHOD,

	PAR_DONE,

	PAR_ERROR
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

void feed(struct parse_ctx *ctx, const char *request_bytes, size_t n);
