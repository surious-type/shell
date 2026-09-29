#include <stdlib.h>

void buffer_init(char **, size_t *, size_t *);

int buffer_push(char **, size_t *, size_t *, char);

int buffer_finish(char **, size_t *, size_t *);

void buffer_reset(char **, size_t *, size_t *);
