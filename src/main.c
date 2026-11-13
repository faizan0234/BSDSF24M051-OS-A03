
#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // Initialize GNU Readline history
    using_history();
    stifle_history(HISTORY_SIZE);

    while ((cmdline = read_cmd(PROMPT)) != NULL) {

        // Ignore empty commands
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

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

        // Add command to GNU Readline history
        add_history(cmdline);

        // Tokenize and execute
        arglist = tokenize(cmdline);

        if (arglist != NULL) {
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
