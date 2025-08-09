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
	if ((len1) != (len2) || memcmp((p1), (p2), (len1)) != 0) { \
		fprintf(stderr, \
			"%s:%d: ASSERT_EQ_MEM failed (len %lu vs %lu)\n", \
			__FILE__, \
			__LINE__, \
			(size_t)(len1), \
			(size_t)(len2)); \
		exit(1); \
	} \
} while (0)

#endif
