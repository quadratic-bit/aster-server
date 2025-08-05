#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* expect http_request to be zeroed out */
struct parse_ctx parse_ctx_init(struct http_request *req) {
	struct parse_ctx ctx;

	ctx.state = PAR_REQ_LINE_METHOD;

	ctx.cap = 1024;
	ctx.buf = malloc(sizeof(char) * ctx.cap);

	if (ctx.buf == NULL) {
		perror("parse_ctx_init");
		exit(1);
	}

	ctx.len = 0;
	ctx.pos = 0;
	ctx.mark = MARK_NONE;
	ctx.req = req;

	return ctx;
}

void parse_ctx_free(struct parse_ctx *ctx) {
	free(ctx->buf);
}

/* expect data to be allocated up to (data+n) */
static void append_to_buf(struct parse_ctx *ctx, const char* data, size_t n) {
	while (n + ctx->len > ctx->cap) {
		size_t new_cap = ctx->cap * 2;
		ctx->buf = realloc(ctx->buf, new_cap);
		if (ctx->buf == NULL) {
			perror("append_to_buf");
			exit(1);
		}
		ctx->cap = new_cap;
	}
	memcpy(ctx->buf + ctx->len, data, n);
	ctx->len += n;
}

/* request-line = method SP request-target SP HTTP-version
   method = token
   token = 1*tchar
   tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" x21-2A except x22", x28(, x29)
          / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~" x2B, x2D, x2E, x5E-60, x7C, x7E
          / DIGIT (x30-x39) / ALPHA (x41-5A / x61-7A)
          ; any VCHAR (x21-7E), except delimiters (hex: 22, 28, 29, 2C, 2F, 5B, 5C, 5D) */

static int is_vchar(char ch) {
	unsigned char c = (unsigned char)ch;
	return c >= 0x21 && c <= 0x7E;
}

/* return 1 if tchar, 0 otherwise */
static int is_tchar(char ch) {
	unsigned char c = (unsigned char)ch;
	if (!is_vchar(ch)) return 0;
	if (c == 0x22 || c == 0x28 || c == 0x29 || c == 0x2C || c == 0x2F
			|| c == 0x5B || c == 0x5C || c == 0x5D) {
		return 0;
	}
	return 1;
}

/* return 1 if finished, 0 if need more bytes */
static int parse_req_line_method(struct parse_ctx *ctx) {
	enum http_method parsed_method;
	char ch;

	if (ctx->mark == MARK_NONE) ctx->mark = ctx->pos;
	ch = ctx->buf[ctx->pos];
	while (is_tchar(ch)) {
		ctx->pos++;
		if (ctx->pos >= ctx->len) return 0;
		ch = ctx->buf[ctx->pos];
	}
	if (ch == SYM_SP) {
		struct slice method = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parsed_method = HM_UNK;
		switch (method.len) {
		case 3:
			if (!slice_str_cmp(&method, "GET"))
				parsed_method = HM_GET;
			else if (!slice_str_cmp(&method, "PUT"))
				parsed_method = HM_PUT;
			break;
		case 4:
			if (!slice_str_cmp(&method, "POST"))
				parsed_method = HM_POST;
			else if (!slice_str_cmp(&method, "HEAD"))
				parsed_method = HM_HEAD;
			break;
		case 5:
			if (!slice_str_cmp(&method, "TRACE"))
				parsed_method = HM_TRACE;
			break;
		case 6:
			if (!slice_str_cmp(&method, "DELETE"))
				parsed_method = HM_DELETE;
			break;
		case 7:
			if (!slice_str_cmp(&method, "CONNECT"))
				parsed_method = HM_CONNECT;
			else if (!slice_str_cmp(&method, "OPTIONS"))
				parsed_method = HM_OPTIONS;
			break;
		}
		ctx->req->method = parsed_method;
		ctx->state = PAR_REQ_LINE_TARGET;
		return 1;
	}
	ctx->state = PAR_ERROR;
	ctx->mark = MARK_NONE;
	return 1;
}

/* return 1 if ALPHA, 0 otherwise */
static int is_alpha(char ch) {
	unsigned char c = (unsigned char)ch;
	return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
}

/* return 1 if DIGIT, 0 otherwise */
static int is_digit(char ch) {
	unsigned char c = (unsigned char)ch;
	return c >= 0x30 && c <= 0x39;
}

/* return 1 if pchar, 0 otherwise */
static int is_pchar(char ch) {
	unsigned char c = (unsigned char)ch;
	if (is_alpha(ch) || is_digit(ch)) {
		return 1;
	}
	if (c >= 0x24 && c <= 0x2E) {
		return 1;
	}
	return ch == '_' || ch == '~' || ch == ':' || ch == '@' || ch == '!' ||
		ch == ';' || ch == '=';
}

static void parse_asterisk_form(struct parse_ctx *ctx) {
	assert(ctx->req->target_form == TF_ASTERISK);
	assert(ctx->req->raw_target.len == 1);
	assert(ctx->req->raw_target.ptr[0] == '*');

	ctx->state = PAR_DONE;
}

static void parse_origin_form(struct parse_ctx *ctx) {
	const struct slice target = ctx->req->raw_target;
	const char *buf = target.ptr;
	size_t pos = 1;

	assert(ctx->req->target_form == TF_ORIGIN);
	assert(ctx->req->raw_target.len > 0);
	assert(ctx->req->raw_target.ptr[0] == '/');

	/* origin-form   = absolute-path [ "?" query ] */
	/* absolute-path = 1*( "/" segment ) */
	/* segment       = *pchar */
	/* query         = *( pchar / "/" / "?" ) */

	/* absolute-path */
	while (pos < target.len) {
		if (buf[pos] == '/') {
			pos++;
			continue;
		}
		if (buf[pos] == '?') {
			pos++;
			break;
		}
		if (!is_pchar(buf[pos])) {
			ctx->state = PAR_ERROR;
			return;
		}
		pos++;
	}

	/* query */
	while (pos < target.len) {
		if (buf[pos] == '/' || buf[pos] == '?' || is_pchar(buf[pos])) {
			pos++;
			continue;
		}
		ctx->state = PAR_ERROR;
		return;
	}

	ctx->state = PAR_DONE;
}

static void parse_authority_form(struct parse_ctx *ctx) {
	assert(ctx->req->target_form == TF_AUTHORITY ||
			ctx->req->target_form == TF_ABSOLUTE);

	/* TODO: implement absolute-form and authority-form */
	assert(0);

	ctx->state = PAR_DONE;
}

static void parse_absolute_form(struct parse_ctx *ctx) {
	/* TODO: implement absolute-form and authority-form */
	assert(ctx->req->target_form == TF_AUTHORITY ||
			ctx->req->target_form == TF_ABSOLUTE);

	/* TODO: implement absolute-form and authority-form */
	assert(0);

	ctx->state = PAR_DONE;
}

/* return 1 if finished, 0 if need more bytes */
static int parse_req_line_target(struct parse_ctx *ctx) {
	char ch;

	ch = ctx->buf[ctx->pos];

	if (ctx->mark == MARK_NONE) { /* First time into the function */
		if (ch == SYM_SP) {
			/* Two SP, reject for now, TODO: handle LWSP */
			ctx->state = PAR_ERROR;
			return 1;
		}
		ctx->mark = ctx->pos;
	} else if (ch == SYM_SP) {
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		switch (ctx->req->target_form) {
		case TF_ASTERISK:
			parse_asterisk_form(ctx);
			break;
		case TF_ORIGIN:
			parse_origin_form(ctx);
			break;
		case TF_AUTHORITY:
			parse_authority_form(ctx);
			break;
		case TF_ABSOLUTE:
			parse_absolute_form(ctx);
			break;
		case TF_UNK: assert(0);
		}
		return 1;
	}

pre_line_target_switch:
	switch (ctx->req->target_form) {
	case TF_ASTERISK: /* More than two characters in asterisk-form */
		ctx->state = PAR_ERROR;
		ctx->mark = MARK_NONE;
		return 1;

	case TF_ORIGIN:
		while (ch == '/' || ch == '?' || is_pchar(ch)) {
			ctx->pos++;
			if (ctx->pos >= ctx->len) return 0;
			ch = ctx->buf[ctx->pos];
		}
		if (ch != SYM_SP) { /* Invalid character for origin-form */
			ctx->state = PAR_ERROR;
			ctx->mark = MARK_NONE;
			return 1;
		}
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parse_origin_form(ctx);
		return 1;

	case TF_UNK:
		if (ch == '*') { /* asterisk-form or invalid */
			ctx->req->target_form = TF_ASTERISK;
			ctx->pos++;
			if (ctx->pos >= ctx->len) return 0;
			ch = ctx->buf[ctx->pos];
			if (ch != SYM_SP) { /* More symbols after initial '*' */
				ctx->state = PAR_ERROR;
				ctx->mark = MARK_NONE;
				return 1;
			}
			ctx->req->raw_target = get_slice(
				ctx->buf + ctx->mark,
				ctx->pos - ctx->mark
			);
			ctx->mark = MARK_NONE;
			ctx->pos++;
			parse_asterisk_form(ctx);
			return 1;
		}
		if (ch == '/') { /* origin-form or invalid */
			ctx->req->target_form = TF_ORIGIN;
			ctx->pos++;
			if (ctx->pos >= ctx->len) return 0;
			ch = ctx->buf[ctx->pos];
			goto pre_line_target_switch;
		}

		/* if (!is_alpha(ch)) ctx->req->target_form = TF_AUTHORITY; */

		while (ch == '/' || ch == '?' || is_pchar(ch)) {
			ctx->pos++;
			if (ctx->pos >= ctx->len) return 0;
			ch = ctx->buf[ctx->pos];
		}
		if (ch != SYM_SP) { /* Invalid character */
			ctx->state = PAR_ERROR;
			ctx->mark = MARK_NONE;
			return 1;
		}
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parse_absolute_form(ctx);
		return 1;

	case TF_AUTHORITY:
	case TF_ABSOLUTE:
		break;
	}

	return 1;
}

/* expect request_bytes to be allocated up to (request_bytes+n) */
void feed(struct parse_ctx *ctx, const char *request_bytes, size_t n) {
	append_to_buf(ctx, request_bytes, n);
	while (ctx->pos < ctx->len && ctx->state != PAR_DONE && ctx->state != PAR_ERROR) {
		switch (ctx->state) {
		case PAR_REQ_LINE_METHOD:
			parse_req_line_method(ctx);
			break;
		case PAR_REQ_LINE_TARGET:
			parse_req_line_target(ctx);
			break;
		case PAR_ERROR:
		case PAR_DONE:
			break;
		}
	}
}
