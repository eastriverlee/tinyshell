#ifndef TINYSHELL_H
#define TINYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define ERROR -1
#define INTERACTIVE 0
#define COMMANDLINE 1
#define CHILD 0
#define CURRENT 0
#define PREVIOUS 1
#define READ 0
#define WRITE 1
#define TRUNCATE 1
#define APPEND 2

#define is_same !strcmp

typedef int io[2];

extern int I;
extern int pipe_count;
extern bool is_background;

bool redirect_in(char **arguments);
bool redirect_out(char **arguments);
void execute(char **command);
void free_strings(char **strings);
char ***parse(char *line);
void free_strings(char **strings);
void exit_error(const char *message);
bool connect(io pipes[2]);
void close_(io pipes[2]);
void alternate(int **pipes);
void erase_from(char **strings, size_t count);
void *xmalloc(size_t size);

#endif
