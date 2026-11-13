
#include "shell.h"

// History storage
static char history[HISTORY_SIZE][MAX_LEN];
static int history_total = 0;

// Add command to history
void add_history(const char* command) {
    if (command == NULL || command[0] == '\0') {
        return;
    }

    int index = history_total % HISTORY_SIZE;

    strncpy(history[index], command, MAX_LEN - 1);
    history[index][MAX_LEN - 1] = '\0';

    history_total++;
}

// Display history
void show_history(void) {
    int first = history_total - HISTORY_SIZE + 1;

    if (first < 1) {
        first = 1;
    }

    for (int number = first; number <= history_total; number++) {
        int index = (number - 1) % HISTORY_SIZE;

        printf("%d  %s\n", number, history[index]);
    }
}

// Get a command using its history number
char* get_history_command(int number) {
    int first = history_total - HISTORY_SIZE + 1;

    if (first < 1) {
        first = 1;
    }

    if (number < first || number > history_total) {
        return NULL;
    }

    int index = (number - 1) % HISTORY_SIZE;

    char* command = malloc(MAX_LEN);

    if (command == NULL) {
        perror("malloc");
        return NULL;
    }

    strcpy(command, history[index]);

    return command;
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);

    char* cmdline = malloc(sizeof(char) * MAX_LEN);

    if (cmdline == NULL) {
        perror("malloc");
        return NULL;
    }

    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;

        if (pos < MAX_LEN - 1) {
            cmdline[pos++] = c;
        }
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }

    cmdline[pos] = '\0';

    return cmdline;
}

char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;

        if (*cp == '\0') break;

        start = cp;
        len = 1;

        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
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
