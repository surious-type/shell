#include "tree.h"
#include "string.h"
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SIZE 5
/*
typedef enum
{
	NXT,
	AND,
	OR
} next_type;

typedef struct Tree
{
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
*/
static int add_arg(tree *cmd, const char *word)
{
	size_t argc = 0;

	if (cmd->argv != NULL)
	{
		while (cmd->argv[argc] != NULL)
			argc++;
	}

	char **tmp = realloc(cmd->argv, (argc + 2) * sizeof(char *));

	if (tmp == NULL)
		return 0;

	cmd->argv = tmp;

	cmd->argv[argc] = copy_string(word);

	return 1;
}

void init_com(tree *cmd)
{
	cmd->argv = NULL;
	cmd->infile = NULL;
	cmd->outfile = NULL;
	cmd->backgrnd = 0;
	cmd->type = NXT;
	cmd->append = 0;
	cmd->psubcmd = NULL;
	cmd->pipe = NULL;
	cmd->next = NULL;
}
tree *com_sh(void);
tree *com_list(void);
tree *conv(void);
tree *command(void);
tree *simple_com(void)
{
}
void print_struct(tree *head)
{
	while (head != NULL)
	{
		printf("[%s]\n", *head->argv);
		head = head->next;
	}
}
void clear_tree(tree *);
