#include "buff.h"
#include "exec.h"
#include "list.h"
#include "tree.h"
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(int argc, char *argv[])
{
	const char *line = "echo hello\ world";
	build_list(&plst, line);
	print_list(plst);
	free_list(&plst);

	return 0;
}
