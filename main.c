#include "buff.h"
#include "exec.h"
#include "list.h"
#include "tree.h"
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFF_SIZE 10

jmp_buf begin;
list *plst;
intlist *bckgrnd;
int exit_val = 0;

void handler(int s)
{
	(void)s;
	signal(SIGINT, handler);
}
static int read_line(char **line)
{
	char *buf;
	size_t len;
	size_t capacity;

	buffer_init(&buf, &len, &capacity);

	int c;

	while ((c = getchar()) != '\n' && c != EOF)
	{
		if (!buffer_push(&buf, &len, &capacity, (char)c))
		{
			perror("realloc");
			buffer_reset(&buf, &len, &capacity);
			return -1;
		}
	}

	/*
	 * Ctrl-D
	 */
	if (c == EOF && len == 0)
	{
		buffer_reset(&buf, &len, &capacity);
		return 0;
	}

	if (!buffer_finish(&buf, &len, &capacity))
	{
		perror("realloc");
		buffer_reset(&buf, &len, &capacity);
		return -1;
	}

	*line = buf;
	return 1;
}
int main(void)
{
	signal(SIGINT, handler);

	while (1)
	{
		clear_zombie(&bckgrnd);

		printf("$ ");
		fflush(stdout);

		char *line = NULL;

		int read_status = read_line(&line);

		if (read_status == 0)
		{
			printf("\n");
			break;
		}

		if (read_status < 0)
			break;

		list *tokens = NULL;

		build_list(&tokens, line);

		free(line);

		if (tokens == NULL)
			continue;

		change_list(tokens);

		plst = tokens;

		tree *cmds = com_sh();

		print_list(tokens);

		free_list(&tokens);

		if (cmds == NULL)
			continue;

		print_struct(cmds, 1);
		int status = exec_com_sh(cmds);
		exit_val = status;

		clear_tree(cmds);

		// print_intlist(bckgrnd);
	}

	clear_zombie(&bckgrnd);
	clear_intlist(bckgrnd);

	return exit_val;
}
