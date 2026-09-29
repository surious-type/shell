#include "buff.h"
#include "exec.h"
#include "list.h"
#include "tree.h"
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
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

int main(void)
{
	const char *line = "echo hello | world";

	list *tokens = NULL;
	build_list(&plst, line);
	plst = tokens;
	tree *cmd = conv();
	free_list(&tokens);
	print_struct(cmd);

	return 0;
}
