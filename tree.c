#include "tree.h"
#include "strutils.h"
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
	if (cmd->argv[argc] == NULL)
	{
		return 0;
	}
	cmd->argv[argc + 1] = NULL;

	return 1;
}

int is_oper(void)
{
	if (plst == NULL)
		return 0;

	const char *word = plst->word;

	return strcmp(word, "|") == 0 || strcmp(word, ">") == 0 || strcmp(word, ">>") == 0 ||
		   strcmp(word, "<") == 0 || strcmp(word, "&") == 0 || strcmp(word, "&&") == 0 ||
		   strcmp(word, "||") == 0 || strcmp(word, ";") == 0 || strcmp(word, "(") == 0 ||
		   strcmp(word, ")") == 0;
}
int is_inout(void)
{
	if (plst == NULL)
		return 0;

	return strcmp(plst->word, "<") == 0 || strcmp(plst->word, ">") == 0 ||
		   strcmp(plst->word, ">>") == 0;
}
int in_file(tree *cmd)
{
	/* сейчас plst указывает на "<" */

	plst = plst->next;

	if (plst == NULL || is_oper())
	{
		fprintf(stderr, "syntax error: ожидалось имя файла после <\n");
		return 0;
	}

	cmd->infile = copy_string(plst->word);

	if (cmd->infile == NULL)
	{
		perror("malloc");
		return 0;
	}

	plst = plst->next;

	return 1;
}
int out_file(tree *cmd)
{
	plst = plst->next;

	if (plst == NULL || is_oper())
	{
		fprintf(stderr, "syntax error: ожидалось имя файла после >\n");
		return 0;
	}

	cmd->outfile = copy_string(plst->word);

	if (cmd->outfile == NULL)
	{
		perror("malloc");
		return 0;
	}

	cmd->append = 0;

	plst = plst->next;

	return 1;
}
int out_append(tree *cmd)
{
	plst = plst->next;

	if (plst == NULL || is_oper())
	{
		fprintf(stderr, "syntax error: ожидалось имя файла после >>\n");
		return 0;
	}

	cmd->outfile = copy_string(plst->word);

	if (cmd->outfile == NULL)
	{
		perror("malloc");
		return 0;
	}

	cmd->append = 1;

	plst = plst->next;

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

tree *conv(void)
{
	tree *head = command();

	if (head == NULL)
		return NULL;

	tree *current = head;

	while (plst != NULL && strcmp(plst->word, "|") == 0)
	{
		plst = plst->next;
		if (plst == NULL)
		{
			fprintf(stderr, "syntax error: ожидалась команда после |\n");
			return NULL;
		}
		tree *next_cmd = command();
		if (next_cmd == NULL)
		{
			return NULL;
		}

		current->pipe = next_cmd;
		current = next_cmd;
	}
	return head;
}

tree *command(void)
{
	return simple_com();
}

tree *simple_com(void)
{
	if (plst == NULL || is_oper())
	{
		fprintf(stderr, "syntax error: ожидалось имя команды\n");
		return NULL;
	}
	tree *cmd = malloc(sizeof(*cmd));

	if (cmd == NULL)
	{
		perror("malloc");
		return NULL;
	}

	init_com(cmd);

	while (plst != NULL && !is_oper())
	{
		if (!add_arg(cmd, plst->word))
		{
			return NULL;
		}

		plst = plst->next;
	}
	while (plst != NULL && is_inout())
	{
		if (strcmp(plst->word, "<") == 0)
		{
			if (!in_file(cmd))
			{
				return NULL;
			}
		}
		else if (strcmp(plst->word, ">") == 0)
		{
			if (!out_file(cmd))
			{
				return NULL;
			}
		}
		else if (strcmp(plst->word, ">>") == 0)
		{
			if (!out_append(cmd))
			{
				return NULL;
			}
		}
	}
	return cmd;
}

void print_struct(tree *head)
{
	if (head == NULL)
		return;

	for (int n = 1; head != NULL; n++)
	{
		printf("Команда: %d:\n", n);
		for (size_t i = 0; head->argv != NULL && head->argv[i] != NULL; i++)
		{
			printf("    argv[%zu] = [%s]\n", i, head->argv[i]);
		}
		if (head->infile != NULL)
		{
			printf("    infile = [%s]\n", head->infile);
		}
		if (head->outfile != NULL)
		{
			printf("    outfile = [%s]\n", head->outfile);
		}
		if (head->append)
		{
			printf("    append = [%d]\n", head->append);
		}
		head = head->pipe;
	}
}
void clear_tree(tree *head)
{
	while (head != NULL)
	{
		tree *next = head->pipe;

		if (head->argv != NULL)
		{
			for (size_t i = 0; head->argv[i] != NULL; i++)
			{
				free(head->argv[i]);
			}

			free(head->argv);
		}

		free(head->infile);
		free(head->outfile);

		free(head);

		head = next;
	}
}
