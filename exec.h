#ifndef EXEC_H
#define EXEC_H

#include "tree.h"

typedef struct backgrndList
{
	int pid;
	struct backgrndList *next;
} intlist;

extern intlist *bckgrnd;
extern int exit_val;

void add_elem(intlist **, int);
void print_intlist(intlist *);
void clear_intlist(intlist *);
void clear_zombie(intlist **);
int is_com(tree *);
int exec_cd(tree *);
int exec_clear(void);
int sh_com(tree *);
void chng_iofiles(int, int, tree *);
int exec_com_sh(tree *);
int exec_com_list(tree *, int);
int exec_conv(tree *, int);
int exec_command(tree *, int, int, int, int, intlist **);
int exec_simple_com(tree *, int, int, int, int, intlist **);

#endif
