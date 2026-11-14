
#include "shell.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS 20

typedef struct {
    int job_id;
    pid_t pids[MAXARGS];
    int process_count;
    char command[MAX_LEN];
    int active;
} Job;

static Job job_list[MAX_JOBS];
static int next_job_id = 1;

static void reap_background_jobs(void);
static void show_jobs(void);
static void add_job(pid_t pids[], int count, const char *command);
static int execute_single(char **arglist, int background,
                          const char *command);
static int execute_pipeline(char **arglist, int background,
                            const char *command);

/* Add a background job to the job table */
static void add_job(pid_t pids[], int count, const char *command) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_list[i].active) {
            job_list[i].job_id = next_job_id++;
            job_list[i].process_count = count;
            job_list[i].active = 1;

            for (int j = 0; j < count; j++) {
                job_list[i].pids[j] = pids[j];
            }

            snprintf(job_list[i].command, MAX_LEN, "%s", command);

            printf("[%d] %d Running %s\n",
                   job_list[i].job_id, pids[0], command);
            fflush(stdout);
            return;
        }
    }

    fprintf(stderr, "Maximum background jobs reached.\n");

    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

/* Reap completed background processes without blocking */
static void reap_background_jobs(void) {
    int status;
    pid_t finished_pid;

    /*
     * -1 means wait for any child process.
     * WNOHANG makes waitpid non-blocking.
     */
    while (1) {
        finished_pid = waitpid(-1, &status, WNOHANG);

        if (finished_pid > 0) {
            for (int i = 0; i < MAX_JOBS; i++) {
                if (!job_list[i].active) {
                    continue;
                }

                for (int j = 0; j < job_list[i].process_count; j++) {
                    if (job_list[i].pids[j] == finished_pid) {
                        job_list[i].pids[j] = -1;
                        break;
                    }
                }
            }
        }
        else if (finished_pid == 0) {
            break;
        }
        else {
            if (errno == EINTR) {
                continue;
            }

            break;
        }
    }

    /* Check which jobs have completed */
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_list[i].active) {
            continue;
        }

        int completed = 1;

        for (int j = 0; j < job_list[i].process_count; j++) {
            if (job_list[i].pids[j] > 0) {
                completed = 0;
                break;
            }
        }

        if (completed) {
            printf("[%d] Done %s\n",
                   job_list[i].job_id,
                   job_list[i].command);

            job_list[i].active = 0;
        }
    }
}

/* Display background jobs */
static void show_jobs(void) {
    reap_background_jobs();

    int found = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].active) {
            found = 1;

            printf("[%d] Running %d %s\n",
                   job_list[i].job_id,
                   job_list[i].pids[0],
                   job_list[i].command);
        }
    }

    if (!found) {
        printf("No background jobs.\n");
    }
}

/* Execute built-in commands */
static int execute_builtin(char **arglist) {

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

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

    if (strcmp(arglist[0], "help") == 0) {
        printf("\nBuilt-in commands:\n");
        printf("  exit     - Exit the shell\n");
        printf("  cd       - Change directory\n");
        printf("  help     - Display help information\n");
        printf("  jobs     - Display background jobs\n");
        printf("  history  - Display command history\n");
        printf("  ;        - Execute commands sequentially\n");
        printf("  &        - Execute command in background\n\n");

        return 0;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        show_jobs();
        return 0;
    }

    if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 0;
    }

    return -1;
}

/* Execute a command with input/output descriptors */
static void execute_command(char **args, int input_fd, int output_fd) {

    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input");
            _exit(1);
        }
    }

    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output");
            _exit(1);
        }
    }

    if (input_fd != STDIN_FILENO) {
        close(input_fd);
    }

    if (output_fd != STDOUT_FILENO) {
        close(output_fd);
    }

    execvp(args[0], args);

    perror("Command execution failed");
    _exit(1);
}

/* Execute pipelines and redirections */
static int execute_pipeline(char **arglist, int background,
                            const char *command) {

    char *commands[MAXARGS][MAXARGS + 1] = {0};
    char *input_files[MAXARGS] = {0};
    char *output_files[MAXARGS] = {0};

    int command_count = 1;
    int argument_count = 0;

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

            if (arglist[i + 1] == NULL ||
                input_files[command_count - 1] != NULL) {
                fprintf(stderr, "Invalid input redirection\n");
                return 1;
            }

            input_files[command_count - 1] = arglist[++i];
        }

        else if (strcmp(arglist[i], ">") == 0) {

            if (arglist[i + 1] == NULL ||
                output_files[command_count - 1] != NULL) {
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

    for (int i = 0; i < command_count; i++) {
        if (commands[i][0] == NULL) {
            fprintf(stderr, "Missing command\n");
            return 1;
        }
    }

    int previous_read = STDIN_FILENO;
    pid_t pids[MAXARGS];
    int created = 0;

    for (int i = 0; i < command_count; i++) {

        int pipe_fd[2] = {-1, -1};

        if (i < command_count - 1) {
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                break;
            }
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");

            if (pipe_fd[0] != -1) close(pipe_fd[0]);
            if (pipe_fd[1] != -1) close(pipe_fd[1]);

            break;
        }

        if (pid == 0) {

            int input_fd = previous_read;
            int output_fd = (i < command_count - 1)
                            ? pipe_fd[1] : STDOUT_FILENO;

            if (input_files[i] != NULL) {
                input_fd = open(input_files[i], O_RDONLY);

                if (input_fd == -1) {
                    perror("input file");
                    _exit(1);
                }

                if (previous_read != STDIN_FILENO) {
                    close(previous_read);
                }
            }

            if (output_files[i] != NULL) {
                output_fd = open(output_files[i],
                                  O_WRONLY | O_CREAT | O_TRUNC,
                                  0644);

                if (output_fd == -1) {
                    perror("output file");
                    _exit(1);
                }

                if (pipe_fd[1] != -1) {
                    close(pipe_fd[1]);
                }
            }

            if (pipe_fd[0] != -1) {
                close(pipe_fd[0]);
            }

            execute_command(commands[i], input_fd, output_fd);
        }

        pids[created++] = pid;

        if (previous_read != STDIN_FILENO) {
            close(previous_read);
        }

        if (pipe_fd[1] != -1) {
            close(pipe_fd[1]);
        }

        previous_read = pipe_fd[0];
    }

    if (previous_read != STDIN_FILENO && previous_read != -1) {
        close(previous_read);
    }

    if (created == 0) {
        return 1;
    }

    if (background) {
        add_job(pids, created, command);
        return 0;
    }

    int status = 0;

    for (int i = 0; i < created; i++) {
        int child_status;

        if (waitpid(pids[i], &child_status, 0) == -1) {
            perror("waitpid");
            status = 1;
        }
        else if (i == created - 1) {
            status = WIFEXITED(child_status)
                     ? WEXITSTATUS(child_status) : 1;
        }
    }

    return status;
}

/* Execute one command */
static int execute_single(char **arglist, int background,
                          const char *command) {

    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    int has_operator = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0 ||
            strcmp(arglist[i], ">") == 0 ||
            strcmp(arglist[i], "|") == 0) {
            has_operator = 1;
            break;
        }
    }

    if (has_operator) {
        return execute_pipeline(arglist, background, command);
    }

    if (!background) {
        int result = execute_builtin(arglist);

        if (result != -1) {
            return result;
        }
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (background) {
            int result = execute_builtin(arglist);

            if (result != -1) {
                _exit(result);
            }
        }

        execvp(arglist[0], arglist);
        perror("Command execution failed");
        _exit(1);
    }

    if (background) {
        add_job(&pid, 1, command);
        return 0;
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

/* Build the complete command text for job display */
static void build_command(char *buffer, size_t size,
                          char **arglist, int start, int end) {

    buffer[0] = '\0';
    size_t used = 0;

    for (int i = start; i < end; i++) {
        int written = snprintf(buffer + used, size - used,
                               "%s%s",
                               (i == start) ? "" : " ",
                               arglist[i]);

        if (written < 0 || (size_t)written >= size - used) {
            break;
        }

        used += (size_t)written;
    }
}

/* Main execute function */
int execute(char **arglist) {

    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    reap_background_jobs();

    int start = 0;
    int status = 0;

    for (int i = 0; ; i++) {

        if (arglist[i] == NULL ||
            strcmp(arglist[i], ";") == 0 ||
            strcmp(arglist[i], "&") == 0) {

            int background = 0;

            if (arglist[i] != NULL &&
                strcmp(arglist[i], "&") == 0) {
                background = 1;
            }

            char command[MAX_LEN];
            build_command(command, sizeof(command), arglist, start, i);

            char *separator = arglist[i];
            arglist[i] = NULL;

            if (start < i) {
                status = execute_single(&arglist[start],
                                        background,
                                        command);
            }

            if (separator == NULL) {
                break;
            }

            arglist[i] = separator;
            start = i + 1;
        }
    }

    reap_background_jobs();

    return status;
}
