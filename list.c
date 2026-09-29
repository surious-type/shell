#include "list.h"
#include "buff.h"
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern jmp_buf begin;

static char *copy_string(const char *src) {
  size_t len = strlen(src);

  char *dst = malloc(len + 1);
  if (dst == NULL)
    return NULL;

  memcpy(dst, src, len + 1);

  return dst;
}

void print_list(list *head) {
  while (head != NULL) {
    printf("%s\n", head->word);
    head = head->next;
  }
}

void free_list(list **head) {
  list *current;
  list *next;

  if (head == NULL) {
    return;
  }

  current = *head;

  while (current != NULL) {
    next = current->next;
    free(current->word);
    free(current);
    current = next;
  }

  *head = NULL;
}

int is_special(char c) {
  const char special_chars[] = "|&;><()";
  if (strchr(special_chars, c) != NULL) {
    return 1;
  } else {
    return 0;
  }
}

static int is_double_special(char first, char second) {
  return (first == '>' && second == '>') || (first == '&' && second == '&') ||
         (first == '|' && second == '|');
}
vertex start(char, int *, int *, list **, int *, list **);
vertex word(char, int *, int *, list **, int *);
vertex bracket(char, int *, int *, list **, int *);
vertex spec(char, int *, int *, list **);

static void append_token(list **head, char *word) {
  if (head == NULL || word == NULL) {
    return;
  }
  list *node = malloc(sizeof(*node));

  if (node == NULL) {
    perror("malloc");
    return;
  }

  node->word = copy_string(word);

  if (node->word == NULL) {
    perror("malloc");
    free(node);
    return;
  }

  node->next = NULL;
  if (*head == NULL) {
    *head = node;
    return;
  }

  list *current = *head;

  while (current->next != NULL) {
    current = current->next;
  }

  current->next = node;
}

static int flush_buffer(list **head, char **buf, size_t *len,
                        size_t *capacity) {
  if (*len == 0)
    return 1;

  if (!buffer_finish(buf, len, capacity))
    return 0;

  append_token(head, *buf);
  buffer_reset(buf, len, capacity);

  return 1;
}

void build_list(list **head, const char *s) {
  char *buf;
  size_t len;
  size_t capacity;
  buffer_init(&buf, &len, &capacity);
  while (*s) {
    if (*s == ' ') {
      if (len > 0) {
        flush_buffer(head, &buf, &len, &capacity);
      }
    } else if (is_special(*s)) {
      if (len > 0) {
        flush_buffer(head, &buf, &len, &capacity);
      }
      char tmp[3];
      tmp[0] = *s;
      if (is_double_special(*s, s[1])) {
        tmp[1] = s[1];
        tmp[2] = '\0';
        ++s;
      } else {
        tmp[1] = '\0';
      }
      append_token(head, tmp);

    } else {
      buffer_push(&buf, &len, &capacity, *s);
    }
    s++;
  }

  if (len > 0) {
    flush_buffer(head, &buf, &len, &capacity);
  }
}
void change_list(list *head) { (void)head; }
