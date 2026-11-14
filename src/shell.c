
#include "shell.h"

// Commands available for TAB completion
static char *commands[] = {
    "exit", "cd", "help", "jobs", "history",
    "pwd", "ls", "echo", "cat", "grep",
    "mkdir", "rmdir", "touch", "rm", "cp",
    "mv", "clear", "ps", "whoami", "date",
    "cal", "uname", "head", "tail", NULL
};

// Generate command suggestions
static char *command_generator(const char *text, int state) {
    static int index;
    const char *name;

    if (state == 0) {
        index = 0;
    }

    while ((name = commands[index]) != NULL) {
        index++;

        if (strncmp(name, text, strlen(text)) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

// Handle TAB completion
static char **command_completion(const char *text, int start, int end) {
    (void)end;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    return rl_completion_matches(text, rl_filename_completion_function);
}

// Read command using GNU Readline
char* read_cmd(char* prompt) {
    rl_attempted_completion_function = command_completion;
    return readline(prompt);
}

// Display command history
void show_history(void) {
    HIST_ENTRY **list = history_list();

    if (list == NULL) {
        return;
    }

    for (int i = 0; list[i] != NULL; i++) {
        printf("%d  %s\n", history_base + i, list[i]->line);
    }
}

// Get command using its history number
char* get_history_command(int number) {
    HIST_ENTRY *entry = history_get(number);

    if (entry == NULL) {
        return NULL;
    }

    return strdup(entry->line);
}

// Check whitespace characters
static int is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' ||
           ch == '\n' || ch == '\r';
}

// Tokenize command
char** tokenize(char* cmdline) {

    if (cmdline == NULL) {
        return NULL;
    }

    char **arglist = malloc(sizeof(char*) * (MAXARGS + 1));

    if (arglist == NULL) {
        perror("malloc");
        return NULL;
    }

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = calloc(ARGLEN, sizeof(char));

        if (arglist[i] == NULL) {
            perror("malloc");

            for (int j = 0; j < i; j++) {
                free(arglist[j]);
            }

            free(arglist);
            return NULL;
        }
    }

    char *cp = cmdline;
    char *start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {

        // Skip spaces, tabs and newline characters
        while (is_whitespace(*cp)) {
            cp++;
        }

        if (*cp == '\0') {
            break;
        }

        start = cp;
        len = 0;

        // Read one token until whitespace
        while (*cp != '\0' && !is_whitespace(*cp)) {
            len++;
            cp++;
        }

        if (len >= ARGLEN) {
            len = ARGLEN - 1;
        }

        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';

        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) {
            free(arglist[i]);
        }

        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;

    return arglist;
}
