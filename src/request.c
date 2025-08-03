#include "request.h"
#include <assert.h>
#include <string.h>

struct slice get_slice(const char *ptr, size_t len) {
	struct slice new_slice;
	new_slice.ptr = ptr;
	new_slice.len = len;
	return new_slice;
}

int slice_str_cmp(const struct slice *sl, const char *str) {
	assert(sl->len == strlen(str));
	return memcmp(sl->ptr, str, sl->len);
}
