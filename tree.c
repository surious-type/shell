#include "tree.h"
#include "strutils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SIZE 5

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
static tree *and_or(void)
{
	tree *head = conv();

	if (head == NULL)
		return NULL;

	tree *current = head;

	while (plst != NULL && (strcmp(plst->word, "&&") == 0 || strcmp(plst->word, "||") == 0))
	{
		if (strcmp(plst->word, "&&") == 0)
		{
			current->type = AND;
		}
		else
		{
			current->type = OR;
		}

		plst = plst->next;

		tree *next_conv = conv();

		if (next_conv == NULL)
		{
			clear_tree(head);
			return NULL;
		}

		current->next = next_conv;
		current = next_conv;
	}

	return head;
}
static void background_pipeline(tree *cmd)
{
	while (cmd != NULL)
	{
		cmd->backgrnd = 1;
		cmd = cmd->pipe;
	}
}
/*
 * Выставить backgroud всем кто AND т.е. сгруппрованы
 * */
static void background_group(tree *first, tree *last)
{
	tree *current = first;

	while (current != NULL)
	{
		background_pipeline(current);

		if (current == last)
			break;

		current = current->next;
	}
}
tree *com_list(void)
{
	tree *head = and_or();

	if (head == NULL)
		return NULL;

	tree *current = head;
	tree *group_start = head;

	while (current->next != NULL)
	{
		current = current->next;
	}
	while (plst != NULL && (strcmp(plst->word, ";") == 0 || strcmp(plst->word, "&") == 0))
	{
		if (strcmp(plst->word, "&") == 0)
		{
			background_group(group_start, current);
		}

		plst = plst->next;

		if (plst == NULL)
		{
			// если завершающий ; считается нормой, то break, иначе нужно выдавать ошибку
			break;
		}
		tree *next = and_or();

		if (next == NULL)
		{
			clear_tree(head);
			return NULL;
		}

		current->type = NXT;
		current->next = next;

		group_start = next;
		current = next;

		while (current->next != NULL)
		{
			current = current->next;
		}
	}

	return head;
}
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
			clear_tree(head);
			return NULL;
		}
		tree *next_cmd = command();
		if (next_cmd == NULL)
		{
			clear_tree(head);
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
			clear_tree(cmd);
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
				clear_tree(cmd);
				return NULL;
			}
		}
		else if (strcmp(plst->word, ">") == 0)
		{
			if (!out_file(cmd))
			{
				clear_tree(cmd);
				return NULL;
			}
		}
		else if (strcmp(plst->word, ">>") == 0)
		{
			if (!out_append(cmd))
			{
				clear_tree(cmd);
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

	for (int i = 1; head != NULL; i++)
	{
		printf("Список %d:\n", i);
		tree *cmd = head;
		for (int j = 1; cmd != NULL; j++)
		{
			printf("    Команда: %d:\n", j);
			for (size_t a = 0; cmd->argv != NULL && cmd->argv[a] != NULL; a++)
			{
				printf("        argv[%zu] = [%s]\n", a, cmd->argv[a]);
			}
			if (cmd->infile != NULL)
			{
				printf("        infile = [%s]\n", cmd->infile);
			}
			if (cmd->outfile != NULL)
			{
				printf("        outfile = [%s]\n", cmd->outfile);
			}
			if (cmd->append)
			{
				printf("        append = [%d]\n", cmd->append);
			}
			printf("        backgrnd = %d\n", cmd->backgrnd);
			if (cmd->type == AND)
				printf("        type = AND\n");
			else if (cmd->type == OR)
				printf("        type = OR\n");
			else
				printf("        type = NXT\n");
			cmd = cmd->pipe;
		}
		head = head->next;
	}
}
void clear_tree(tree *head)
{
	if (head == NULL)
		return;

	clear_tree(head->pipe);
	clear_tree(head->next);

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
}
