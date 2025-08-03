#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#endif
