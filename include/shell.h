
#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define HISTORY_SIZE 20
#define PROMPT "FCIT> "

// Function prototypes
char* read_cmd(char* prompt);
char** tokenize(char* cmdline);
int execute(char** arglist);

void show_history(void);
char* get_history_command(int number);

#endif
