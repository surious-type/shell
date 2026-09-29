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

vertex start(char, int *, int *, list **, int *, list **);
vertex word(char, int *, int *, list **, int *);
vertex bracket(char, int *, int *, list **, int *);
vertex spec(char, int *, int *, list **);

void build_list(list **head, char *word) {
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

void change_list(list *head) { (void)head; }
