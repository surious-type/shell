#ifndef TREE_H
#define TREE_H

#include "list.h"

typedef enum { NXT, AND, OR } next_type;

typedef struct Tree {
  char **argv;
  char *infile;
  char *outfile;
  int backgrnd;
  next_type type;
  int append;
  struct Tree *psubcmd;
  struct Tree *pipe;
  struct Tree *next;
} tree;

extern list *plst;

void err_file(void);
void in_file(tree *);
void out_file(tree *);
void out_append(tree *);
void error(char *, char *);
int is_oper(void);
int is_next(void);
int is_inout(void);
void background_sub(tree *);
void background(tree *);
void init_com(tree *);
tree *com_sh(void);
tree *com_list(void);
tree *conv(void);
tree *command(void);
tree *simple_com(void);
void print_struct(tree *);
void clear_tree(tree *);

#endif
