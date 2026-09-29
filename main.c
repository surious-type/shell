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
static int is_double_special(char first, char second) {
  return (first == '>' && second == '>') || (first == '&' && second == '&') ||
         (first == '|' && second == '|');
}
int main(int argc, char *argv[]) {
  char *buf;
  size_t len;
  size_t capacity;
  const char *s = "cat>>file&&echo ok||echo fail";
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
      char tmp[3];
      tmp[0] = *s;
      if (is_double_special(*s, s[1])) {
        tmp[1] = s[1];
        tmp[2] = '\0';
        ++s;
      } else {
        tmp[1] = '\0';
      }
      build_list(&plst, tmp);

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
