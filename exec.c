#include "exec.h"
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int redirect_io(tree *cmd) {
  if (cmd->infile != NULL) {
    int fd = open(cmd->infile, O_RDONLY);

    if (fd < 0) {
      perror(cmd->infile);
      return 0;
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
      perror("dup2");
      close(fd);
      return 0;
    }

    close(fd);
  }

  if (cmd->outfile != NULL) {
    int flags = O_WRONLY | O_CREAT;

    if (cmd->append)
      flags |= O_APPEND;
    else
      flags |= O_TRUNC;

    int fd = open(cmd->outfile, flags, 0666);

    if (fd < 0) {
      perror(cmd->outfile);
      return 0;
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2");
      close(fd);
      return 0;
    }

    close(fd);
  }

  return 1;
}
static void run_child(tree *cmd) {
  if (!redirect_io(cmd)) {
    _exit(1);
  }
  execvp(cmd->argv[0], cmd->argv);

  perror(cmd->argv[0]);
  _exit(127);
}
static int exec_external(tree *cmd) {
  if (cmd == NULL || cmd->argv == NULL || cmd->argv[0] == NULL) {
    return 1;
  }

  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    run_child(cmd);
  }

  int status;

  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    return 1;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }

  return 1;
}
static int exec_pipeline2(tree *cmd) {
  tree *second = cmd->pipe;

  int fd[2];

  if (pipe(fd) < 0) {
    perror("pipe");
    return 1;
  }

  pid_t first_pid = fork();

  if (first_pid < 0) {
    perror("fork");
    close(fd[0]);
    close(fd[1]);
    return 1;
  }

  if (first_pid == 0) {
    /*
     * stdout первой команды отправляем
     * в канал.
     */
    if (dup2(fd[1], STDOUT_FILENO) < 0) {
      perror("dup2");
      _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    run_child(cmd);
  }

  pid_t second_pid = fork();

  if (second_pid < 0) {
    perror("fork");
    close(fd[0]);
    close(fd[1]);
    waitpid(first_pid, NULL, 0);
    return 1;
  }

  if (second_pid == 0) {
    /*
     * stdin второй команды берём
     * из канала.
     */
    if (dup2(fd[0], STDIN_FILENO) < 0) {
      perror("dup2");
      _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    run_child(second);
  }

  /*
   * Shell сам ничего через этот pipe
   * не читает и не пишет.
   */
  close(fd[0]);
  close(fd[1]);

  int first_status;
  int second_status;

  waitpid(first_pid, &first_status, 0);
  waitpid(second_pid, &second_status, 0);

  if (WIFEXITED(second_status))
    return WEXITSTATUS(second_status);

  if (WIFSIGNALED(second_status))
    return 128 + WTERMSIG(second_status);

  return 1;
}
int exec_com_sh(tree *cmd) {
  if (cmd == NULL)
    return 1;

  if (cmd->pipe != NULL)
    return exec_pipeline2(cmd);
  return exec_external(cmd);
}
