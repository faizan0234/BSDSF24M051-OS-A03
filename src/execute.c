#include "shell.h"

int execute(char* arglist[]) {

    // Built-in command: exit
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // Built-in command: cd
    if (strcmp(arglist[0], "cd") == 0) {
        char *path = arglist[1];

        if (path == NULL) {
            path = getenv("HOME");
        }

        if (path == NULL || chdir(path) != 0) {
            perror("cd");
        }

        return 0;
    }

    // Built-in command: help
    if (strcmp(arglist[0], "help") == 0) {
        printf("\nBuilt-in commands:\n");
        printf("  exit  - Exit the shell\n");
        printf("  cd    - Change directory\n");
        printf("  help  - Display help information\n");
        printf("  jobs  - Display background jobs\n\n");
        return 0;
    }

    // Built-in command: jobs
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs.\n");
        return 0;
    }

    // External commands
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            return -1;

        case 0:
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);

        default:
            waitpid(cpid, &status, 0);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}
