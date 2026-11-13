
#include "shell.h"
#include <fcntl.h>

// Execute built-in commands
static int execute_builtin(char **arglist) {

    // Built-in: exit
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // Built-in: cd
    if (strcmp(arglist[0], "cd") == 0) {
        char *path = arglist[1];

        if (path == NULL) {
            path = getenv("HOME");
        }

        if (path == NULL || chdir(path) != 0) {
            perror("cd");
            return 1;
        }

        return 0;
    }

    // Built-in: help
    if (strcmp(arglist[0], "help") == 0) {
        printf("\nBuilt-in commands:\n");
        printf("  exit     - Exit the shell\n");
        printf("  cd       - Change directory\n");
        printf("  help     - Display help information\n");
        printf("  jobs     - Display background jobs\n");
        printf("  history  - Display command history\n\n");
        return 0;
    }

    // Built-in: jobs
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs.\n");
        return 0;
    }

    // Built-in: history
    if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 0;
    }

    return -1;
}

// Execute a command with input/output redirection
static void execute_command(char **args, int input_fd, int output_fd) {

    // Connect input
    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input");
            _exit(1);
        }
    }

    // Connect output
    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output");
            _exit(1);
        }
    }

    // Close original descriptors
    if (input_fd != STDIN_FILENO) {
        close(input_fd);
    }

    if (output_fd != STDOUT_FILENO) {
        close(output_fd);
    }

    // Execute command
    execvp(args[0], args);

    perror("Command execution failed");
    _exit(1);
}

// Execute commands containing pipes and redirection
static int execute_pipeline(char **arglist) {

    char *commands[MAXARGS][MAXARGS + 1];
    char *input_files[MAXARGS] = {NULL};
    char *output_files[MAXARGS] = {NULL};

    int command_count = 1;
    int argument_count = 0;

    // Parse commands and redirection operators
    for (int i = 0; arglist[i] != NULL; i++) {

        if (strcmp(arglist[i], "|") == 0) {

            if (argument_count == 0 || command_count >= MAXARGS) {
                fprintf(stderr, "Invalid pipe syntax\n");
                return 1;
            }

            commands[command_count - 1][argument_count] = NULL;
            command_count++;
            argument_count = 0;
        }

        else if (strcmp(arglist[i], "<") == 0) {

            if (arglist[i + 1] == NULL || input_files[command_count - 1] != NULL) {
                fprintf(stderr, "Invalid input redirection\n");
                return 1;
            }

            input_files[command_count - 1] = arglist[++i];
        }

        else if (strcmp(arglist[i], ">") == 0) {

            if (arglist[i + 1] == NULL || output_files[command_count - 1] != NULL) {
                fprintf(stderr, "Invalid output redirection\n");
                return 1;
            }

            output_files[command_count - 1] = arglist[++i];
        }

        else {

            if (argument_count >= MAXARGS) {
                fprintf(stderr, "Too many arguments\n");
                return 1;
            }

            commands[command_count - 1][argument_count++] = arglist[i];
        }
    }

    if (argument_count == 0) {
        fprintf(stderr, "Invalid command syntax\n");
        return 1;
    }

    commands[command_count - 1][argument_count] = NULL;

    // Check that every command has a name
    for (int i = 0; i < command_count; i++) {
        if (commands[i][0] == NULL) {
            fprintf(stderr, "Missing command\n");
            return 1;
        }
    }

    int previous_read = STDIN_FILENO;
    int status = 0;
    pid_t pids[MAXARGS];

    // Create processes for pipeline commands
    for (int i = 0; i < command_count; i++) {

        int pipe_fd[2] = {-1, -1};

        // Create pipe except for last command
        if (i < command_count - 1) {
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                return 1;
            }
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {

            int input_fd = previous_read;
            int output_fd = (i < command_count - 1) ? pipe_fd[1] : STDOUT_FILENO;

            // Input file redirection
            if (input_files[i] != NULL) {
                input_fd = open(input_files[i], O_RDONLY);

                if (input_fd == -1) {
                    perror("input file");
                    _exit(1);
                }
            }

            // Output file redirection
            if (output_files[i] != NULL) {
                output_fd = open(output_files[i],
                                  O_WRONLY | O_CREAT | O_TRUNC,
                                  0644);

                if (output_fd == -1) {
                    perror("output file");
                    _exit(1);
                }
            }

            // Close unused pipe ends
            if (pipe_fd[0] != -1) {
                close(pipe_fd[0]);
            }

            execute_command(commands[i], input_fd, output_fd);
        }

        pids[i] = pid;

        // Parent closes descriptors it no longer needs
        if (previous_read != STDIN_FILENO) {
            close(previous_read);
        }

        if (pipe_fd[1] != -1) {
            close(pipe_fd[1]);
        }

        previous_read = pipe_fd[0];
    }

    // Wait for all pipeline processes
    for (int i = 0; i < command_count; i++) {
        int child_status;

        if (waitpid(pids[i], &child_status, 0) == -1) {
            perror("waitpid");
            status = 1;
        } else if (i == command_count - 1) {
            status = WIFEXITED(child_status) ? WEXITSTATUS(child_status) : 1;
        }
    }

    return status;
}

// Main execute function
int execute(char **arglist) {

    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    // Check for redirection or pipe operators
    int has_operator = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0 ||
            strcmp(arglist[i], ">") == 0 ||
            strcmp(arglist[i], "|") == 0) {
            has_operator = 1;
            break;
        }
    }

    // Execute normal built-in commands in parent process
    if (!has_operator) {
        int result = execute_builtin(arglist);

        if (result != -1) {
            return result;
        }

        // Normal external command
        int status;
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) {
            execvp(arglist[0], arglist);
            perror("Command execution failed");
            _exit(1);
        }

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }

    return execute_pipeline(arglist);
}
