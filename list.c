#include "list.h"
#include "buff.h"
#include "strutils.h"
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern jmp_buf begin;

void print_list(list *head)
{
	while (head != NULL)
	{
		printf("[%s]\n", head->word);
		head = head->next;
	}
}

void free_list(list **head)
{
	list *current;
	list *next;

	if (head == NULL)
	{
		return;
	}

	current = *head;

	while (current != NULL)
	{
		next = current->next;
		free(current->word);
		free(current);
		current = next;
	}

	*head = NULL;
}

int is_special(char c)
{
	const char special_chars[] = "|&;><()";
	if (strchr(special_chars, c) != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_double_special(char first, char second)
{
	return (first == '>' && second == '>') || (first == '&' && second == '&') ||
		   (first == '|' && second == '|');
}

static void append_token(list **head, const char *word)
{
	if (head == NULL || word == NULL)
	{
		return;
	}
	list *node = malloc(sizeof(*node));

	if (node == NULL)
	{
		perror("malloc");
		return;
	}

	node->word = copy_string(word);

	if (node->word == NULL)
	{
		perror("malloc");
		free(node);
		return;
	}

	node->next = NULL;
	if (*head == NULL)
	{
		*head = node;
		return;
	}

	list *current = *head;

	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = node;
}

static int flush_buffer(list **head, char **buf, size_t *len, size_t *capacity, int *token_started)
{
	if (!*token_started)
		return 1;

	if (!buffer_finish(buf, len, capacity))
		return 0;

	append_token(head, *buf);
	buffer_reset(buf, len, capacity);

	*token_started = 0;

	return 1;
}

void build_list(list **head, const char *s)
{
	char *buf;
	size_t len;
	size_t capacity;
	buffer_init(&buf, &len, &capacity);
	char quote = '\0';
	int token_started = 0;

	while (*s)
	{
		if (*s == '\\')
		{
			if (s[1] == '\0')
			{
				fprintf(stderr, "lexical error: нет символа после обратного слеша\n");
				buffer_reset(&buf, &len, &capacity);
				free_list(head);
				return;
			}
			++s;
			buffer_push(&buf, &len, &capacity, *s);
			token_started = 1;
		}
		else if (quote == '\0' && *s == '#')
		{
			break;
		}
		else if (quote != '\0')
		{
			if (*s == quote)
			{
				quote = '\0';
			}
			else
			{
				token_started = 1;
				buffer_push(&buf, &len, &capacity, *s);
			}
		}
		else if (*s == '"' || *s == '\'')
		{
			token_started = 1;
			quote = *s;
		}
		else if (*s == ' ' || *s == '\t')
		{
			if (token_started)
			{
				flush_buffer(head, &buf, &len, &capacity, &token_started);
			}
		}
		else if (is_special(*s))
		{
			if (token_started)
			{
				flush_buffer(head, &buf, &len, &capacity, &token_started);
			}
			char tmp[3];
			tmp[0] = *s;
			if (is_double_special(*s, s[1]))
			{
				tmp[1] = s[1];
				tmp[2] = '\0';
				++s;
			}
			else
			{
				tmp[1] = '\0';
			}
			append_token(head, tmp);
		}
		else
		{
			token_started = 1;
			buffer_push(&buf, &len, &capacity, *s);
		}
		s++;
	}

	if (quote != '\0')
	{
		fprintf(stderr, "lexical error: не закрыта кавычка\n");
		buffer_reset(&buf, &len, &capacity);
		free_list(head);
		return;
	}
	if (token_started)
	{
		flush_buffer(head, &buf, &len, &capacity, &token_started);
	}
}

static int replace_word(list *node, const char *value)
{
	if (value == NULL)
	{
		return 0;
	}
	char *new_value = copy_string(value);
	if (new_value == NULL)
	{
		return 0;
	}
	free(node->word);
	node->word = new_value;
	return 1;
}

void change_list(list *head)
{
	while (head != NULL)
	{
		if (strcmp(head->word, "$HOME") == 0)
		{
			const char *home = getenv("HOME");
			if (!replace_word(head, home))
				return;
		}
		else if (strcmp(head->word, "$USER") == 0)
		{
			const char *user = getlogin();
			if (!replace_word(head, user))
				return;
		}
		else if (strcmp(head->word, "$EUID") == 0)
		{
			char euid[32];
			snprintf(euid, sizeof(euid), "%lu", (unsigned long)geteuid());
			if (!replace_word(head, euid))
				return;
		}
		else if (strcmp(head->word, "$SHELL") == 0)
		{
			char path[1024];
			ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

			if (len == -1)
			{
				perror("readlink error");
				return;
			}

			if ((size_t)len >= sizeof(path) - 1)
			{
				fprintf(stderr, "Путь к shell слишком длинный\n");
				return;
			}

			path[len] = '\0';

			if (!replace_word(head, path))
				return;
		}
		head = head->next;
	}
}

vertex start(char, int *, int *, list **, int *, list **);
vertex word(char, int *, int *, list **, int *);
vertex bracket(char, int *, int *, list **, int *);
vertex spec(char, int *, int *, list **);
