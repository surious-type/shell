#include "exec.h"
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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
    execvp(cmd->argv[0], cmd->argv);

    perror(cmd->argv[0]);
    _exit(127);
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

int exec_com_sh(tree *cmd) { return exec_external(cmd); }
