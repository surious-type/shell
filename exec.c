#include "exec.h"
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int redirect_io(tree *cmd)
{
	if (cmd->infile != NULL)
	{
		int fd = open(cmd->infile, O_RDONLY);

		if (fd < 0)
		{
			perror(cmd->infile);
			return 0;
		}

		if (dup2(fd, STDIN_FILENO) < 0)
		{
			perror("dup2");
			close(fd);
			return 0;
		}

		close(fd);
	}

	if (cmd->outfile != NULL)
	{
		int flags = O_WRONLY | O_CREAT;

		if (cmd->append)
			flags |= O_APPEND;
		else
			flags |= O_TRUNC;

		int fd = open(cmd->outfile, flags, 0666);

		if (fd < 0)
		{
			perror(cmd->outfile);
			return 0;
		}

		if (dup2(fd, STDOUT_FILENO) < 0)
		{
			perror("dup2");
			close(fd);
			return 0;
		}

		close(fd);
	}

	return 1;
}
static void run_node(tree *cmd)
{
	if (!redirect_io(cmd))
	{
		_exit(1);
	}
	if (cmd->psubcmd != NULL)
	{
		int status = exec_com_sh(cmd->psubcmd);
		_exit(status);
	}
	execvp(cmd->argv[0], cmd->argv);

	perror(cmd->argv[0]);
	_exit(127);
}
static int exec_external(tree *cmd)
{
	if (cmd == NULL)
	{
		return 1;
	}

	if (cmd->psubcmd == NULL && (cmd->argv == NULL || cmd->argv[0] == NULL))
	{
		return 1;
	}

	pid_t pid = fork();

	if (pid < 0)
	{
		perror("fork");
		return 1;
	}

	if (pid == 0)
	{
		run_node(cmd);
	}

	int status;

	if (waitpid(pid, &status, 0) < 0)
	{
		perror("waitpid");
		return 1;
	}

	if (WIFEXITED(status))
	{
		return WEXITSTATUS(status);
	}

	if (WIFSIGNALED(status))
	{
		return 128 + WTERMSIG(status);
	}

	return 1;
}
static int exec_pipeline(tree *cmd)
{
	int prev_read = -1;
	pid_t last_pid = -1;
	int count = 0;
	tree *current = cmd;
	pid_t *pids = malloc((count + 1) * sizeof(*pids));

	if (pids == NULL)
	{
		perror("malloc");
		return 1;
	}
	while (current != NULL)
	{
		int fd[2] = {-1, -1};
		int has_next = current->pipe != NULL;

		if (has_next)
		{
			if (pipe(fd) < 0)
			{
				perror("pipe");
				return 1;
			}
		}

		pid_t pid = fork();

		if (pid < 0)
		{
			perror("fork");

			if (prev_read != -1)
				close(prev_read);

			if (has_next)
			{
				close(fd[0]);
				close(fd[1]);
			}

			return 1;
		}

		if (pid == 0)
		{
			if (prev_read != -1)
			{
				if (dup2(prev_read, STDIN_FILENO) < 0)
				{
					perror("dup2");
					_exit(1);
				}
			}

			if (has_next)
			{
				if (dup2(fd[1], STDOUT_FILENO) < 0)
				{
					perror("dup2");
					_exit(1);
				}
			}

			if (prev_read != -1)
				close(prev_read);

			if (has_next)
			{
				close(fd[0]);
				close(fd[1]);
			}

			run_node(current);
		}
		pid_t *tmp = realloc(pids, (count + 1) * sizeof(*pids));

		if (tmp == NULL)
		{
			perror("realloc");
			return 1;
		}
		pids = tmp;
		pids[count++] = pid;
		last_pid = pid;

		if (prev_read != -1)
			close(prev_read);

		if (has_next)
		{
			close(fd[1]);
			prev_read = fd[0];
		}
		else
		{
			prev_read = -1;
		}

		current = current->pipe;
	}

	int last_status = 1;

	for (int i = 0; i < count; i++)
	{
		int status;

		if (waitpid(pids[i], &status, 0) < 0)
		{
			perror("waitpid");
			continue;
		}

		if (pids[i] == last_pid)
			last_status = status;
	}
	free(pids);
	if (WIFEXITED(last_status))
		return WEXITSTATUS(last_status);

	if (WIFSIGNALED(last_status))
		return 128 + WTERMSIG(last_status);

	return 1;
}
static int exec_foreground(tree *cmd)
{
	if (cmd->pipe != NULL)
		return exec_pipeline(cmd);
	return exec_external(cmd);
}
static int exec_background_group(tree *first, tree *last)
{
	pid_t pid = fork();

	if (pid < 0)
	{
		perror("fork");
		return 1;
	}

	if (pid == 0)
	{
		signal(SIGINT, SIG_IGN);

		int fd = open("/dev/null", O_RDONLY);

		if (fd < 0)
			_exit(1);

		if (dup2(fd, STDIN_FILENO) < 0)
		{
			close(fd);
			_exit(1);
		}

		close(fd);

		tree *current = first;
		int status = exec_foreground(current);

		while (current != last)
		{
			next_type type = current->type;
			current = current->next;

			if (type == AND && status == 0)
				status = exec_foreground(current);
			else if (type == OR && status != 0)
				status = exec_foreground(current);
		}

		_exit(status);
	}

	printf("[background pid %d]\n", pid);
	return 0;
}
static int exec_group(tree *first, tree *last)
{
	tree *current = first;
	int status = exec_foreground(current);

	while (current != last)
	{
		next_type type = current->type;
		current = current->next;

		if (type == AND && status == 0)
			status = exec_foreground(current);
		else if (type == OR && status != 0)
			status = exec_foreground(current);
	}

	return status;
}
int exec_com_sh(tree *cmd)
{
	if (cmd == NULL)
		return 1;

	tree *current = cmd;
	int status = 0;

	while (current != NULL)
	{
		tree *last = current;

		while (last->next != NULL && last->type != NXT)
		{
			last = last->next;
		}

		if (current->backgrnd)
			status = exec_background_group(current, last);
		else
			status = exec_group(current, last);

		current = last->next;
	}

	return status;
}
