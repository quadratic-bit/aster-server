#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct slice {
	const char *ptr;
	size_t len;
};

enum http_header_type {
	HH_UNK = 0,
	HH_ACCEPT,
	HH_ACCEPT_CHARSET,
	HH_ACCEPT_ENCODING,
	HH_ACCEPT_LANGUAGE,
	HH_AUTHORIZATION,
	HH_CACHE_CONTROL,
	HH_CONNECTION,
	HH_CONTENT_ENCODING,
	HH_CONTENT_LANGUAGE,
	HH_CONTENT_LENGTH,
	HH_CONTENT_LOCATION,
	HH_CONTENT_MD5,
	HH_CONTENT_RANGE,
	HH_CONTENT_TYPE,
	HH_DATE,
	HH_EXPECT,
	HH_EXPIRES,
	HH_FROM,
	HH_HOST,
	HH_IF_MATCH,
	HH_IF_MODIFIED_SINCE,
	HH_IF_NONE_MATCH,
	HH_IF_RANGE,
	HH_IF_UNMODIFIED_SINCE,
	HH_MAX_FORWARDS,
	HH_PRAGMA,
	HH_PROXY_AUTHORIZATION,
	HH_RANGE,
	HH_REFERER,
	HH_TE,
	HH_TRAILER,
	HH_TRANSFER_ENCODING,
	HH_UPGRADE,
	HH_USER_AGENT,
	HH_VARY,
	HH_VIA,
	HH_WARNING
};

struct http_header {
	struct slice name;
	struct slice value;
	enum http_header_type type;
};

enum http_method {
	HM_UNK = 0,
	HM_GET,
	HM_HEAD,
	HM_POST,
	HM_PUT,
	HM_DELETE,
	HM_CONNECT,
	HM_OPTIONS,
	HM_TRACE
};

enum request_target_form {
	TF_UNK = 0,
	TF_ORIGIN,
	TF_ABSOLUTE,
	TF_AUTHORITY,
	TF_ASTERISK
};

struct http_request {
	enum http_method method;

	uint8_t http_major, http_minor;

	enum request_target_form target_form;
	struct slice raw_target;
	struct slice scheme, authority, host, path, query;
	uint16_t port; /* 0 if unspecified */

	struct http_header *headers;
	size_t num_headers, cap_headers;

	void *body_ctx;
};

struct slice get_slice(const char *ptr, size_t len);

int slice_str_cmp(const struct slice *sl, const char *str);

void http_request_free(struct http_request *req);

struct http_request new_request(void);

void append_empty_header(
		struct http_request *req,
		struct slice header_name
);

/* return pointer to the first header with the same (null-terminated) name,
   NULL if didn't find any. case-sensitive (all lowercased) */
struct http_header *get_header(struct http_request *req, const char *name);

/* return pointer to the first header with the specified type, NULL if didn't find any */
struct http_header *get_header_by_type(struct http_request *req, enum http_header_type type);

void strip_postfix_ows(struct slice *header_value);
