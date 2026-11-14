
#include "shell.h"
#include <ctype.h>

#define MAX_BLOCK_LINES 50

/* Remove leading and trailing whitespace */
static char *trim(char *text) {
    while (isspace((unsigned char)*text)) {
        text++;
    }

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    *end = '\0';
    return text;
}

/* Execute one normal command line */
static int run_line(const char *input) {
    char *line = strdup(input);

    if (line == NULL) {
        perror("strdup");
        return 1;
    }

    char *command = trim(line);

    if (*command == '\0') {
        free(line);
        return 0;
    }

    /* Support !n history execution */
    if (command[0] == '!') {
        char *end;
        long number = strtol(command + 1, &end, 10);

        if (number <= 0 || end == command + 1 || *end != '\0') {
            printf("Invalid history command.\n");
            free(line);
            return 1;
        }

        char *old_command = get_history_command((int)number);

        if (old_command == NULL) {
            printf("History number %ld not found.\n", number);
            free(line);
            return 1;
        }

        free(line);
        line = old_command;
        command = trim(line);

        printf("%s\n", command);
    }

    add_history(command);

    char **arglist = tokenize(command);
    int status = 0;

    if (arglist != NULL) {
        status = execute(arglist);

        for (int i = 0; arglist[i] != NULL; i++) {
            free(arglist[i]);
        }

        free(arglist);
    }

    free(line);
    return status;
}

/* Store one command in a conditional block */
static int store_line(char **lines, int *count, const char *line) {
    if (*count >= MAX_BLOCK_LINES) {
        fprintf(stderr, "Conditional block is too large.\n");
        return 0;
    }

    lines[*count] = strdup(line);

    if (lines[*count] == NULL) {
        perror("strdup");
        return 0;
    }

    (*count)++;
    return 1;
}

/* Free commands stored in a conditional block */
static void free_block(char **lines, int count) {
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
}

/* Read and execute a multiline if-then-else-fi block */
static int execute_if_block(char *first_line) {
    char *condition[MAX_BLOCK_LINES] = {0};
    char *then_commands[MAX_BLOCK_LINES] = {0};
    char *else_commands[MAX_BLOCK_LINES] = {0};

    int condition_count = 0;
    int then_count = 0;
    int else_count = 0;

    int section = 0;
    int found_then = 0;
    int found_fi = 0;

    /* Support: if command */
    char *rest = trim(first_line + 2);

    if (*rest != '\0') {
        if (!store_line(condition, &condition_count, rest)) {
            return 1;
        }
    }

    printf("Enter commands. Finish the block with fi.\n");

    char *input;

    while ((input = read_cmd("....> ")) != NULL) {
        char *command = trim(input);

        if (strcmp(command, "then") == 0 && section == 0) {
            section = 1;
            found_then = 1;
        }
        else if (strcmp(command, "else") == 0 && section == 1) {
            section = 2;
        }
        else if (strcmp(command, "fi") == 0 && found_then) {
            found_fi = 1;
            free(input);
            break;
        }
        else if (strcmp(command, "if") == 0) {
            fprintf(stderr, "Nested if blocks are not supported.\n");
            free(input);
            continue;
        }
        else {
            int stored = 0;

            if (section == 0) {
                stored = store_line(condition, &condition_count, command);
            }
            else if (section == 1) {
                stored = store_line(then_commands, &then_count, command);
            }
            else {
                stored = store_line(else_commands, &else_count, command);
            }

            if (!stored) {
                free(input);
                break;
            }
        }

        free(input);
    }

    if (!found_then || !found_fi || condition_count == 0) {
        fprintf(stderr, "Invalid if-then-else-fi syntax.\n");

        free_block(condition, condition_count);
        free_block(then_commands, then_count);
        free_block(else_commands, else_count);

        return 1;
    }

    /* Execute condition commands */ 
 int status = 0;

for (int i = 0; i < condition_count; i++) {
    status = run_line(condition[i]);
}

    /* Execute the selected branch */
    if (status == 0) {
        for (int i = 0; i < then_count; i++) {
            status = run_line(then_commands[i]);
        }
    }
    else {
        for (int i = 0; i < else_count; i++) {
            status = run_line(else_commands[i]);
        }
    }

    free_block(condition, condition_count);
    free_block(then_commands, then_count);
    free_block(else_commands, else_count);

    return status;
}

/* Check whether input starts with the if keyword */
static int is_if_start(char *line) {
    char *command = trim(line);

    return strncmp(command, "if", 2) == 0 &&
           (command[2] == '\0' ||
            isspace((unsigned char)command[2]));
}

int main(void) {
    char *cmdline;

    using_history();
    stifle_history(HISTORY_SIZE);

    while ((cmdline = read_cmd(PROMPT)) != NULL) {
        char *command = trim(cmdline);

        if (*command == '\0') {
            free(cmdline);
            continue;
        }

        if (is_if_start(command)) {
            execute_if_block(command);
        }
        else {
            run_line(command);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    return 0;
}
