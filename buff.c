#include <stdlib.h>

#define SIZE 5

void buffer_init(char **buf, size_t *len, size_t *capacity) {
  *buf = NULL;
  *len = 0;
  *capacity = 0;
}

int buffer_push(char **buf, size_t *len, size_t *capacity, char c) {
  if (*len >= *capacity) {
    size_t new_capacity = *capacity + SIZE;
    char *tmp = realloc(*buf, new_capacity);
    if (tmp == NULL) {
      return 0;
    }
    *buf = tmp;
    *capacity = new_capacity;
  }

  (*buf)[*len] = c;
  (*len)++;

  return 1;
}

int buffer_finish(char **buf, size_t *len, size_t *capacity) {
  if (*len >= *capacity) {
    size_t new_capacity = *capacity + SIZE;
    char *tmp = realloc(*buf, new_capacity);
    if (tmp == NULL) {
      return 0;
    }
    *buf = tmp;
    *capacity = new_capacity;
  }

  (*buf)[*len] = '\0';

  return 1;
}

void buffer_reset(char **buf, size_t *len, size_t *capacity) {
  free(*buf);
  *buf = NULL;
  *len = 0;
  *capacity = 0;
}
