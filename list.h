#ifndef LIST_H
#define LIST_H

#include <setjmp.h>
#include <stdlib.h>

typedef struct List {
  char *word;
  struct List *next;
} list;

typedef enum { START, W, BRCK, SPEC, END } vertex;

extern jmp_buf begin;

void print_list(list *);
void free_list(list **);
int is_special(char);
static int is_double_special(char, char);
vertex start(char, int *, int *, list **, int *, list **);
vertex word(char, int *, int *, list **, int *);
vertex bracket(char, int *, int *, list **, int *);
vertex spec(char, int *, int *, list **);
static void append_token(list **, char *);
void build_list(list **, const char *);
void change_list(list *);
#endif
