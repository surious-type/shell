#include <stdlib.h>
#include <string.h>

char *copy_string(const char *src)

{
	size_t len = strlen(src);

	char *dst = malloc(len + 1);
	if (dst == NULL)
		return NULL;

	memcpy(dst, src, len + 1);

	return dst;
}
