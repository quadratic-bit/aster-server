#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CRLF "\r\n"
#define RL11(method, target) method " " target " " "HTTP/1.1" CRLF
#define H(key, value) key ": " value CRLF
#define HOST(value) H("Host", value)
#define END CRLF

#define RUN_TEST(fn)				\
	do {					\
		fprintf(stderr, "%s: ", #fn);	\
		fn();				\
		fprintf(stderr, "OK\n");	\
	} while (0)

#define ASSERT_EQ_INT(a, b) do { \
	if ((a) != (b)) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_INT(%s,%s) got %d vs %d\n", \
			__FILE__, \
			__LINE__, \
			#a, \
			#b, \
			(int)(a), \
			(int)(b)); \
		exit(1); \
	} \
} while (0)

#define ASSERT_EQ_MEM(p1, len1, p2, len2) do { \
	if ((len1) != (len2) || memcmp((p1), (p2), (len1))) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_MEM failed (len %lu vs %lu)\n", \
			__FILE__, \
			__LINE__, \
			(size_t)(len1), \
			(size_t)(len2)); \
		exit(1); \
	} \
} while (0)

#define ASSERT_EQ_SLICE(slice, str) do { \
	if (slice.len != strlen(str) || memcmp(slice.ptr, (str), slice.len)) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_SLICE failed (expected \"%s\", got \"%.*s\")\n", \
			__FILE__, \
			__LINE__, \
			(const char*)(str), \
			(int)(slice.len), \
			(const char*)(slice.ptr)); \
		exit(1); \
	} \
} while (0)

#define ASSERT_EQ_HEADER(req, type, expect) do { \
	struct http_header *_test_header = get_header(req, type); \
	if (!_test_header) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_HEADER failed - no header with type (%d)\n", \
			__FILE__, \
			__LINE__, \
			(int)(type)); \
		exit(1); \
	} \
	if (_test_header->value.len != strlen(expect) || \
			memcmp(_test_header->value.ptr, (expect), _test_header->value.len)) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_HEADER failed (expected \"%s\", got \"%.*s\")\n", \
			__FILE__, \
			__LINE__, \
			(const char*)(expect), \
			(int)(_test_header->value.len), \
			(const char*)(_test_header->value.ptr)); \
		exit(1); \
	} \
} while (0)

#define ASSERT_EQ_HEADER_NAME(req, name, expect) do { \
	struct http_header *_test_header = get_header_by_name(req, name); \
	if (!_test_header) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_HEADER failed - no header named \"%s\"\n", \
			__FILE__, \
			__LINE__, \
			(const char*)(name)); \
		exit(1); \
	} \
	if (_test_header->value.len != strlen(expect) || \
			memcmp(_test_header->value.ptr, (expect), _test_header->value.len)) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_HEADER failed (expected \"%.*s\", got \"%s\")\n", \
			__FILE__, \
			__LINE__, \
			(const char*)(expect), \
			(int)(_test_header->value.len), \
			(const char*)(_test_header->value.ptr)); \
		exit(1); \
	} \
} while (0)

#endif
