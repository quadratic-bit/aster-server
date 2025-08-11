#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* expect http_request to be zeroed out */
struct parse_ctx parse_ctx_init(struct http_request *req) {
	struct parse_ctx ctx;

	ctx.state = PS_REQ_LINE_METHOD;

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

/* return 1 if vchar, 0 otherwise */
static int is_vchar(char ch) {
	return ch >= '!' && ch <= '~';
}

/* return 1 if ALPHA, 0 otherwise */
static int is_alpha(char ch) {
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

/* return 1 if DIGIT, 0 otherwise */
static int is_digit(char ch) {
	return ch >= '0' && ch <= '9';
}

/* return 1 if HEXDIG, 0 otherwise */
static int is_hexdig(char ch) {
	return is_digit(ch) || (
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F')
	);
}

/* return 1 if tchar, 0 otherwise */
static int is_tchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) return 1;
	if (!is_vchar(ch)) return 0;
	return ch == '!' || ch == '#' || ch == '$' || ch == '%' ||
		ch == '&' || ch == '\'' || ch == '*' || ch == '+' ||
		ch == '-' || ch == '.' || ch == '^' || ch == '_' || ch == '`' ||
		ch == '|' || ch == '~';
}

/* return 1 if obs-text, 0 otherwise */
static int is_obs_text(char ch) {
	unsigned char uch = (unsigned char)ch;
	return uch >= 0x80;
}

/* return 1 and advance pos by 2 if valid pct-encoding, return 0 otherwise */
static int consume_pct_enc(struct parse_ctx *ctx) {
	assert(ctx->buf[ctx->pos] == '%');
	if (ctx->pos + 2 >= ctx->len) {
		return 0;
	}
	if (!is_hexdig(ctx->buf[ctx->pos + 1]) ||
			!is_hexdig(ctx->buf[ctx->pos + 2])) {
		return 0;
	}
	ctx->pos += 2;
	return 1;
}

/* if is ALPHA, lowercase */
static char lower(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		return ch | 0x20;
	}
	return ch;
}

/* parse DIGIT into uint8_t */
static uint8_t to_digit(char ch) {
	assert(is_digit(ch));
	return (uint8_t)((unsigned char)ch - 0x30);
}

static uint16_t parse_port(const char *buf, size_t len) {
	uint16_t port = 0;
	uint16_t power = 1;
	size_t pos;
	for (pos = 0; pos < len; ++pos, power *= 10) {
		port += (uint16_t)(to_digit(buf[len - pos - 1]) * power);
	}
	return port;
}

static enum parse_result parse_req_line_method(struct parse_ctx *ctx) {
	enum http_method parsed_method;
	struct slice method;
	char ch;

	if (ctx->mark == MARK_NONE) ctx->mark = ctx->pos;
	ch = ctx->buf[ctx->pos];
	while (is_tchar(ch)) {
		ctx->pos++;
		if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		ch = ctx->buf[ctx->pos];
	}
	if (ch != SYM_SP) {
		ctx->state = PS_PARSING_ERROR;
		ctx->mark = MARK_NONE;
		return PR_COMPLETE;
	}

	method = get_slice(
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
		/* if (!slice_str_cmp(&method, "CONNECT"))
			parsed_method = HM_CONNECT; */
		if (!slice_str_cmp(&method, "OPTIONS"))
			parsed_method = HM_OPTIONS;
		break;
	}
	if (parsed_method == HM_UNK) {
		ctx->state = PS_UNSUPPORTED_METHOD;
		return PR_COMPLETE;
	}
	ctx->req->method = parsed_method;
	ctx->state = PS_REQ_LINE_TARGET;
	return PR_COMPLETE;
}

/* return 1 if pchar, 0 otherwise */
static int is_pchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) {
		return 1;
	}
	if (ch >= '$' && ch <= '.') {
		return 1;
	}
	return ch == '_' || ch == '~' || ch == ':' || ch == '@' || ch == '!' ||
		ch == ';' || ch == '=';
}

/* return 1 if allowed reg-name char, 0 otherwise */
static int is_regchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) {
		return 1;
	}
	if (ch >= '$' && ch <= '.') {
		return 1;
	}
	return ch == '_' || ch == '~' || ch == '!' || ch == ';' || ch == '=';
}

static void parse_asterisk_form(struct parse_ctx *ctx) {
	assert(ctx->req->target_form == TF_ASTERISK);
	assert(ctx->req->raw_target.len == 1);
	assert(ctx->req->raw_target.ptr[0] == '*');

	ctx->state = PS_REQ_LINE_HTTP_NAME;
}

static void parse_origin_form(struct parse_ctx *ctx) {
	const struct slice target = ctx->req->raw_target;
	const char *buf = target.ptr;
	size_t pos = 1;
	size_t mark = 0;

	assert(ctx->req->target_form == TF_ORIGIN);
	assert(ctx->req->raw_target.len > 0);
	assert(ctx->req->raw_target.ptr[0] == '/');

	/* absolute-path */
	while (pos < target.len) {
		if (buf[pos] == '/') {
			pos++;
			continue;
		}
		if (buf[pos] == '?') {
			ctx->req->path = get_slice(buf + mark, pos - mark);
			pos++;
			break;
		}
		if (!is_pchar(buf[pos])) {
			ctx->state = PS_PARSING_ERROR;
			return;
		}
		pos++;
	}
	if (ctx->req->path.ptr == NULL) {
		ctx->req->path = get_slice(buf + mark, pos - mark);
	}

	/* query */
	if (pos < target.len) {
		ctx->req->query = get_slice(buf + pos, target.len - pos);
	}

	ctx->state = PS_REQ_LINE_HTTP_NAME;
}

static void parse_absolute_form(struct parse_ctx *ctx) {
	const struct slice target = ctx->req->raw_target;
	const char *buf = target.ptr;
	size_t pos = 0;
	size_t mark;

	assert(ctx->req->target_form == TF_UNK);

	if (target.len < 8) {
		ctx->state = PS_PARSING_ERROR;
		return;
	}

	if (memcmp(buf, "http://", 7)) { /* TODO: allow https */
		ctx->state = PS_PARSING_ERROR;
		return;
	}
	ctx->req->scheme = get_slice(buf, 4);

	pos += 7;
	ctx->req->target_form = TF_ABSOLUTE;

	if (buf[pos] == ':') { /* Empty host */
		ctx->state = PS_PARSING_ERROR;
		return;
	}

	mark = pos;
	if (buf[pos] == '[') { /* IP-literal */
		do { /* TODO: parse IPv6 */
			pos++;
			if (pos >= target.len) {
				ctx->state = PS_PARSING_ERROR;
				return;
			}
		} while (is_hexdig(buf[pos]) || buf[pos] == ':');

		if (buf[pos] != ']') {
			ctx->state = PS_PARSING_ERROR;
			return;
		}
		pos++;
		if (pos >= target.len) {
			ctx->state = PS_REQ_LINE_HTTP_NAME;
			ctx->req->host = get_slice(buf + mark, pos - mark);
			return;
		}
	} else {
		while (is_regchar(buf[pos])) { /* Assume reg-name host */
			pos++;
			if (pos >= target.len) {
				ctx->state = PS_REQ_LINE_HTTP_NAME;
				ctx->req->host = get_slice(
					buf + mark,
					pos - mark
				);
				return;
			}
		}
		if (buf[pos] == '@') { /* Userinfo is deprecated */
			ctx->state = PS_PARSING_ERROR;
			return;
		}
	}

	ctx->req->host = get_slice(buf + mark, pos - mark);

	if (buf[pos] == ':') { /* Port */
		pos++;
		mark = pos;
		if (pos >= target.len) { /* Empty port */
			ctx->state = PS_REQ_LINE_HTTP_NAME;
			return;
		}
		while (is_digit(buf[pos])) {
			pos++;
			if (pos >= target.len) {
				ctx->state = PS_REQ_LINE_HTTP_NAME;
				ctx->req->port = parse_port(
					buf + mark,
					pos - mark
				);
				ctx->req->authority = get_slice(
					ctx->req->host.ptr,
					pos - mark + ctx->req->host.len + 1
				);
				return;
			}
		}
		ctx->req->port = parse_port(buf + mark, pos - mark);
		ctx->req->authority = get_slice(
			ctx->req->host.ptr,
			pos - mark + ctx->req->host.len + 1
		);
	}

	/* path-abempty */

	if (buf[pos] != '/' && buf[pos] != '?') {
		ctx->state = PS_PARSING_ERROR;
		return;
	}

	mark = pos;
	while (buf[pos] == '/' || is_pchar(buf[pos])) {
		pos++;
		if (pos >= target.len) {
			ctx->state = PS_REQ_LINE_HTTP_NAME;
			return;
		}
	}
	ctx->req->path = get_slice(buf + mark, pos - mark);

	/* [ ?query ] */

	if (buf[pos] != '?') {
		ctx->state = PS_PARSING_ERROR;
		return;
	}

	mark = pos + 1;
	do {
		pos++;
		if (pos < target.len) {
			continue;
		}
		ctx->req->query = get_slice(buf + mark, pos - mark);
		ctx->state = PS_REQ_LINE_HTTP_NAME;
		return;
	} while (buf[pos] == '/' || buf[pos] == '?' || is_pchar(buf[pos]));

	ctx->state = PS_PARSING_ERROR;
	return;
}

static enum parse_result parse_req_line_target(struct parse_ctx *ctx) {
	char ch;

	ch = ctx->buf[ctx->pos];

	if (ctx->mark == MARK_NONE) { /* First time into the function */
		if (ch == SYM_SP) {
			/* Two SP, reject for now, TODO: handle LWSP */
			ctx->state = PS_PARSING_ERROR;
			return PR_COMPLETE;
		}
		ctx->mark = ctx->pos;
		if (ch == '*') {
			ctx->req->target_form = TF_ASTERISK;
			ctx->pos++;
			ch = ctx->buf[ctx->pos];
			if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		} else if (ch == '/') {
			ctx->req->target_form = TF_ORIGIN;
			ctx->pos++;
			ch = ctx->buf[ctx->pos];
			if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		} else {
			/* Should be TF_UNK by default */
			ctx->req->target_form = TF_UNK;
		}
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
		case TF_ABSOLUTE:
			parse_absolute_form(ctx);
			break;
		case TF_AUTHORITY:
		case TF_UNK: assert(0);
		}
		return PR_COMPLETE;
	}

	switch (ctx->req->target_form) {
	case TF_ASTERISK:
		if (ch != SYM_SP) { /* More symbols after initial '*' */
			ctx->state = PS_PARSING_ERROR;
			ctx->mark = MARK_NONE;
			return PR_COMPLETE;
		}
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parse_asterisk_form(ctx);
		return PR_COMPLETE;

	case TF_ORIGIN:
		while (ch == '/' || ch == '?' || is_pchar(ch)) {
			if (ch == '%' && !consume_pct_enc(ctx)) {
				ctx->state = PS_PARSING_ERROR;
				return PR_COMPLETE;
			}
			ctx->pos++;
			if (ctx->pos >= ctx->len) return PR_NEED_MORE;
			ch = ctx->buf[ctx->pos];
		}
		if (ch != SYM_SP) { /* Invalid character for origin-form */
			ctx->state = PS_PARSING_ERROR;
			ctx->mark = MARK_NONE;
			return PR_COMPLETE;
		}
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parse_origin_form(ctx);
		return PR_COMPLETE;

	case TF_UNK: /* TF_AUTHORITY or TF_ABSOLUTE */
		while (ch == '/' || ch == '?' || is_pchar(ch)) {
			if (ch == '%' && !consume_pct_enc(ctx)) {
				ctx->state = PS_PARSING_ERROR;
				return PR_COMPLETE;
			}
			ctx->pos++;
			if (ctx->pos >= ctx->len) return PR_NEED_MORE;
			ch = ctx->buf[ctx->pos];
		}
		if (ch != SYM_SP) { /* Invalid character */
			ctx->state = PS_PARSING_ERROR;
			ctx->mark = MARK_NONE;
			return PR_COMPLETE;
		}
		ctx->req->raw_target = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		ctx->mark = MARK_NONE;
		ctx->pos++;
		parse_absolute_form(ctx);
		return PR_COMPLETE;

	case TF_AUTHORITY:
	case TF_ABSOLUTE: assert(0);
	}

	return PR_COMPLETE;
}

static enum parse_result parse_req_line_http_name(struct parse_ctx *ctx) {
	/* HTTP-version = "HTTP/" DIGIT "." DIGIT */
	/*                 ^                      */
	if (ctx->pos + 4 >= ctx->len) return PR_NEED_MORE;

	if (memcmp(ctx->buf + ctx->pos, "HTTP/", 5)) {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}
	ctx->pos += 5;
	ctx->state = PS_REQ_LINE_HTTP_MAJOR;
	return PR_COMPLETE;
}

static enum parse_result parse_req_line_http_major(struct parse_ctx *ctx) {
	/* HTTP-version = "HTTP/" DIGIT "." DIGIT */
	/*                          ^             */
	char ch = ctx->buf[ctx->pos];
	if (!is_digit(ch)) {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}
	ctx->req->http_major = to_digit(ch);
	ctx->pos++;
	ctx->state = PS_REQ_LINE_HTTP_PERIOD;
	return PR_COMPLETE;
}

static enum parse_result parse_req_line_http_period(struct parse_ctx *ctx) {
	/* HTTP-version = "HTTP/" DIGIT "." DIGIT */
	/*                               ^        */
	char ch = ctx->buf[ctx->pos];
	if (ch != '.') {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}
	ctx->pos++;
	ctx->state = PS_REQ_LINE_HTTP_MINOR;
	return PR_COMPLETE;
}

static enum parse_result parse_req_line_http_minor(struct parse_ctx *ctx) {
	/* HTTP-version = "HTTP/" DIGIT "." DIGIT */
	/*                                    ^   */
	char ch = ctx->buf[ctx->pos];
	if (!is_digit(ch)) {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}
	ctx->req->http_minor = to_digit(ch);
	ctx->pos++;
	ctx->state = PS_REQ_LINE_CRLF;
	return PR_COMPLETE;
}

static enum parse_result parse_req_line_crlf(struct parse_ctx *ctx) {
	if (ctx->pos + 1 >= ctx->len) return PR_NEED_MORE;

	if (memcmp(ctx->buf + ctx->pos, CRLF, 2)) {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}
	ctx->pos += 2;
	ctx->state = PS_FIELD_LINE_NAME;
	return PR_COMPLETE;
}

static enum http_header_type parse_header_type(struct slice *name) {
	enum http_header_type parsed_type = HH_UNK;
	switch (name->len) {
	case 2:
		if (!slice_str_cmp(name, "te"))
			parsed_type = HH_TE;
		break;
	case 3:
		if (!slice_str_cmp(name, "via"))
			parsed_type = HH_VIA;
		break;
	case 4:
		if (!slice_str_cmp(name, "date"))
			parsed_type = HH_DATE;
		else if (!slice_str_cmp(name, "host"))
			parsed_type = HH_HOST;
		else if (!slice_str_cmp(name, "from"))
			parsed_type = HH_FROM;
		else if (!slice_str_cmp(name, "vary"))
			parsed_type = HH_VARY;
		break;
	case 5:
		if (!slice_str_cmp(name, "range"))
			parsed_type = HH_RANGE;
		break;
	case 6:
		if (!slice_str_cmp(name, "accept"))
			parsed_type = HH_ACCEPT;
		else if (!slice_str_cmp(name, "expect"))
			parsed_type = HH_EXPECT;
		else if (!slice_str_cmp(name, "pragma"))
			parsed_type = HH_PRAGMA;
		break;
	case 7:
		if (!slice_str_cmp(name, "expires"))
			parsed_type = HH_EXPIRES;
		else if (!slice_str_cmp(name, "referer"))
			parsed_type = HH_REFERER;
		else if (!slice_str_cmp(name, "trailer"))
			parsed_type = HH_TRAILER;
		else if (!slice_str_cmp(name, "upgrade"))
			parsed_type = HH_UPGRADE;
		else if (!slice_str_cmp(name, "warning"))
			parsed_type = HH_WARNING;
		break;
	case 8:
		if (!slice_str_cmp(name, "if-match"))
			parsed_type = HH_IF_MATCH;
		else if (!slice_str_cmp(name, "if-range"))
			parsed_type = HH_IF_RANGE;
		break;
	case 10:
		if (!slice_str_cmp(name, "connection"))
			parsed_type = HH_CONNECTION;
		else if (!slice_str_cmp(name, "user-agent"))
			parsed_type = HH_USER_AGENT;
		break;
	case 11:
		if (!slice_str_cmp(name, "content-md5"))
			parsed_type = HH_CONTENT_MD5;
		break;
	case 12:
		if (!slice_str_cmp(name, "content-type"))
			parsed_type = HH_CONTENT_TYPE;
		else if (!slice_str_cmp(name, "max-forwards"))
			parsed_type = HH_MAX_FORWARDS;
		break;
	case 13:
		if (!slice_str_cmp(name, "authorization"))
			parsed_type = HH_AUTHORIZATION;
		else if (!slice_str_cmp(name, "cache-control"))
			parsed_type = HH_CACHE_CONTROL;
		else if (!slice_str_cmp(name, "content-range"))
			parsed_type = HH_CONTENT_RANGE;
		else if (!slice_str_cmp(name, "if-none-match"))
			parsed_type = HH_IF_NONE_MATCH;
		break;
	case 14:
		if (!slice_str_cmp(name, "accept-charset"))
			parsed_type = HH_ACCEPT_CHARSET;
		else if (!slice_str_cmp(name, "content-length"))
			parsed_type = HH_CONTENT_LENGTH;
		break;
	case 15:
		if (!slice_str_cmp(name, "accept-encoding"))
			parsed_type = HH_ACCEPT_ENCODING;
		else if (!slice_str_cmp(name, "accept-language"))
			parsed_type = HH_ACCEPT_LANGUAGE;
		break;
	case 16:
		if (!slice_str_cmp(name, "content-encoding"))
			parsed_type = HH_CONTENT_ENCODING;
		else if (!slice_str_cmp(name, "content-language"))
			parsed_type = HH_CONTENT_LANGUAGE;
		else if (!slice_str_cmp(name, "content-location"))
			parsed_type = HH_CONTENT_LOCATION;
		break;
	case 17:
		if (!slice_str_cmp(name, "if-modified-since"))
			parsed_type = HH_IF_MODIFIED_SINCE;
		else if (!slice_str_cmp(name, "transfer-encoding"))
			parsed_type = HH_TRANSFER_ENCODING;
		break;
	case 19:
		if (!slice_str_cmp(name, "if-unmodified-since"))
			parsed_type = HH_IF_UNMODIFIED_SINCE;
		else if (!slice_str_cmp(name, "proxy-authorization"))
			parsed_type = HH_PROXY_AUTHORIZATION;
		break;
	}
	return parsed_type;
}

static enum parse_result parse_field_line_name(struct parse_ctx *ctx) {
	char ch = ctx->buf[ctx->pos];
	struct slice field_name;
	size_t i;

	if (ch == SYM_CR) {
		if (ctx->pos + 1 >= ctx->len) return PR_NEED_MORE;
		if (ctx->buf[ctx->pos + 1] == SYM_LF) {
			ctx->pos += 2;
			ctx->state = PS_DONE;
			return PR_COMPLETE;
		}
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}

	if (ctx->mark == MARK_NONE) {
		ctx->mark = ctx->pos;
	}

	while (is_tchar(ch)) {
		ctx->pos++;
		if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		ch = ctx->buf[ctx->pos];
	}

	if (ch != ':') {
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}

	if (ctx->pos == ctx->mark) { /* Empty header */
		ctx->state = PS_PARSING_ERROR;
		return PR_COMPLETE;
	}

	field_name = get_slice(
		ctx->buf + ctx->mark,
		ctx->pos - ctx->mark
	);

	/* Header names are case-insensitive, keep lowercase for consistency */
	for (i = 0; i < field_name.len; ++i) {
		(ctx->buf + ctx->mark)[i] = lower((ctx->buf + ctx->mark)[i]);
	}

	ctx->pos++;
	ctx->mark = MARK_NONE;
	append_empty_header(ctx->req, field_name);
	ctx->req->headers[ctx->req->num_headers - 1].type = parse_header_type(
		&field_name
	);
	ctx->state = PS_FIELD_LINE_PRE_OWS;
	return PR_COMPLETE;
}

static enum parse_result parse_field_line_pre_ows(struct parse_ctx *ctx) {
	char ch = ctx->buf[ctx->pos];

	while (ch == SYM_SP || ch == SYM_HTAB) {
		ctx->pos++;
		if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		ch = ctx->buf[ctx->pos];
	}

	ctx->state = PS_FIELD_LINE_VALUE;
	return PR_COMPLETE;
}

static enum parse_result parse_field_line_value(struct parse_ctx *ctx) {
	char ch = ctx->buf[ctx->pos];
	struct slice h_value;

	if (ctx->mark == MARK_NONE) {
		if (ch == SYM_CR) { /* empty header value */
			ctx->state = PS_PARSING_ERROR;
			return PR_COMPLETE;
		}
		ctx->mark = ctx->pos;
	}

	while (is_vchar(ch) || is_obs_text(ch)) {
		ctx->pos++;
		if (ctx->pos >= ctx->len) return PR_NEED_MORE;
		ch = ctx->buf[ctx->pos];
	}

	if (ch == SYM_CR) {
		if (ctx->pos + 1 >= ctx->len) return PR_NEED_MORE;
		if (ctx->buf[ctx->pos + 1] != SYM_LF) {
			ctx->state = PS_PARSING_ERROR;
			return PR_COMPLETE;
		}
		h_value = get_slice(
			ctx->buf + ctx->mark,
			ctx->pos - ctx->mark
		);
		strip_postfix_ows(&h_value);
		ctx->req->headers[ctx->req->num_headers - 1].value = h_value;
		ctx->mark = MARK_NONE;
		ctx->pos += 2;
		ctx->state = PS_FIELD_LINE_NAME;
		return PR_COMPLETE;
	}
	ctx->mark = MARK_NONE;
	ctx->state = PS_PARSING_ERROR;
	return PR_COMPLETE;
}

static void parse_host(struct parse_ctx *ctx) {
	struct http_header *host = get_header(ctx->req, "host");
	if (!host) {
		ctx->state = PS_PARSING_ERROR;
		return;
	}
	if (!ctx->req->host.ptr) {
		ctx->req->host = host->value;
	}
	ctx->state = PS_DONE;
	return;
}

/* expect request_bytes to be allocated up to (request_bytes+n) */
enum parse_result feed(struct parse_ctx *ctx, const char *req_bytes, size_t n) {
	enum parse_result res = PR_NEED_MORE;

	append_to_buf(ctx, req_bytes, n);

	while (
			ctx->pos < ctx->len &&
			ctx->state < PS_PARSING_ERROR
	      ) {
		switch (ctx->state) {
		case PS_REQ_LINE_METHOD:
			res = parse_req_line_method(ctx);
			break;
		case PS_REQ_LINE_TARGET:
			res = parse_req_line_target(ctx);
			break;
		case PS_REQ_LINE_HTTP_NAME:
			res = parse_req_line_http_name(ctx);
			break;
		case PS_REQ_LINE_HTTP_MAJOR:
			res = parse_req_line_http_major(ctx);
			break;
		case PS_REQ_LINE_HTTP_PERIOD:
			res = parse_req_line_http_period(ctx);
			break;
		case PS_REQ_LINE_HTTP_MINOR:
			res = parse_req_line_http_minor(ctx);
			break;
		case PS_REQ_LINE_CRLF:
			res = parse_req_line_crlf(ctx);
			break;
		case PS_FIELD_LINE_NAME:
			res = parse_field_line_name(ctx);
			break;
		case PS_FIELD_LINE_PRE_OWS:
			res = parse_field_line_pre_ows(ctx);
			break;
		case PS_FIELD_LINE_VALUE:
			res = parse_field_line_value(ctx);
			break;

		case PS_PARSING_ERROR:
		case PS_UNSUPPORTED_METHOD:
		case PS_DONE:
			break;
		}

		if (res == PR_NEED_MORE) {
			return PR_NEED_MORE;
		}
	}

	if (res == PR_COMPLETE) {
		assert(ctx->state == PS_DONE);
		parse_host(ctx);
	}

	return res;
}
