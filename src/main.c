
#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        // Handle !n history execution
        if (cmdline[0] == '!') {
            char* end;
            long number = strtol(cmdline + 1, &end, 10);

            if (number <= 0 || end == cmdline + 1 || *end != '\0') {
                printf("Invalid history command.\n");
                free(cmdline);
                continue;
            }

            char* old_command = get_history_command((int)number);

            if (old_command == NULL) {
                printf("History number %ld not found.\n", number);
                free(cmdline);
                continue;
            }

            free(cmdline);
            cmdline = old_command;

            printf("%s\n", cmdline);
        }

        // Save command in history
        add_history(cmdline);

        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    return 0;
}
