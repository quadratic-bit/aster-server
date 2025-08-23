#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

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
	HH_WARNING,
	HH__COUNT
};

struct http_header {
	struct slice name;
	struct slice value;
	enum http_header_type type;
	size_t next_same_type; /* index into headers array or SIZE_MAX */
};

/* use `http_header_type` as indices */
struct field_index {
	size_t heads[HH__COUNT]; /* first entrance of field type or SIZE_MAX */
	size_t tails[HH__COUNT]; /* last entrance of field type or SIZE_MAX */
	size_t count[HH__COUNT]; /* count of entrances of such type */
};

enum http_method {
	HM_UNK = 0,
	HM_GET,
	HM_HEAD,
	HM_POST,
	HM_PUT,
	HM_DELETE,
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
	struct field_index *h_index; /* index into headers */
	size_t num_headers, cap_headers;

	unsigned te_chunked:1;
	unsigned keep_alive:1;
	unsigned expect_100:1;
	unsigned upgrade:1;

	ssize_t content_length; /* -1 if unknown */

	void *body_ctx;
};

struct slice get_slice(const char *ptr, size_t len);

int slice_str_cmp(const struct slice *sl, const char *str);
int slice_str_cmp_check(const struct slice *sl, const char *str);

int slice_str_cmp_ci(const struct slice *sl, const char *str);
int slice_str_cmp_ci_check(const struct slice *sl, const char *str);

void http_request_free(struct http_request *req);

struct http_request new_request(void);

void append_empty_header(
		struct http_request *req,
		struct slice header_name
);

/* return pointer to the first header with the same (null-terminated) name,
   NULL if didn't find any. case-sensitive (all lowercased) */
struct http_header *get_header_by_name(struct http_request *req, const char *name);

/* return pointer to the first header with the specified type, NULL if didn't find any */
struct http_header *get_header(struct http_request *req, enum http_header_type type);

void strip_postfix_ows(struct slice *header_value);

/* return 1 if http versions match, 0 otherwise */
int is_http_ver(struct http_request *req, uint8_t major, uint8_t minor);

/* O(1) lookup */
size_t headers_count(const struct http_request *req, enum http_header_type htype);

/* returns SIZE_MAX if none */
size_t headers_first(const struct http_request *req, enum http_header_type htype);

/* SIZE_MAX if end */
size_t headers_next(const struct http_request *req, size_t idx);

struct header_item_iter {
	size_t header_index;
	struct slice header_item;
	size_t offset;
	int last_comma;
};

/* -1 if malformed field */
struct header_item_iter header_items_init(const struct http_request *req, enum http_header_type htype);

/* -1 if malformed field */
int header_items_next(const struct http_request *req, struct header_item_iter *it);

#endif
