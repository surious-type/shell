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

void handler(int s) {
  (void)s;
  signal(SIGINT, handler);
}

int main(void) {
  const char *line = "(false || echo YES) | cat";
  list *tokens = NULL;
  build_list(&tokens, line);
  plst = tokens;
  tree *cmds = com_sh();
  print_list(tokens);
  free_list(&tokens);

  if (cmds != NULL) {
    print_struct(cmds, 1);
    int status = exec_com_sh(cmds);
    printf("exit status = %d\n", status);
  }
  clear_tree(cmds);

  return 0;
}
