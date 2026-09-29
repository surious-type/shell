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

void handler(int s) {
  (void)s;
  signal(SIGINT, handler);
}

int main(int argc, char *argv[]) {
  char *buf;
  size_t len;
  size_t capacity;
  char *s = "cat    asd.txt|asd";

  buffer_init(&buf, &len, &capacity);
  while (*s) {
    if (*s == ' ') {
      if (len > 0) {
        buffer_finish(&buf, &len, &capacity);
        build_list(&plst, buf);
        buffer_reset(&buf, &len, &capacity);
      }
    } else if (is_special(*s)) {
      if (len > 0) {
        buffer_finish(&buf, &len, &capacity);
        build_list(&plst, buf);
        buffer_reset(&buf, &len, &capacity);
      }
      buffer_push(&buf, &len, &capacity, *s);
      buffer_finish(&buf, &len, &capacity);
      build_list(&plst, buf);
      buffer_reset(&buf, &len, &capacity);

    } else {
      buffer_push(&buf, &len, &capacity, *s);
    }
    s++;
  }

  if (len > 0) {
    buffer_finish(&buf, &len, &capacity);
    build_list(&plst, buf);
    buffer_reset(&buf, &len, &capacity);
  }
  print_list(plst);
  free_list(&plst);

  free(buf);

  return 0;
}
