# PUFCIT's custom UNIX Shell

A simple command-line interpreter (shell) built in C for an Operating Systems course.

## Quick Start

### Build the Shell

To compile the project, run the `make` command:
```bash
make
```

### Run the Shell

The compiled executable will be located in the `bin/` directory.
```bash
./bin/psh
```

### Clean the Project

To remove all compiled object files and the final executable:
```bash
make clean
```

## Directory Structure

*   `/src`: All C source code files (`.c`).
*   `/include`: All header files (`.h`).
*   `/bin`: Contains the final compiled executable (`psh`).
*   `/obj`: Contains intermediate object files (`.o`) created during compilation.
*   `Makefile`: The build script for the project.

*   ## Supported Features

This shell currently supports the following features:

- Basic command execution using fork() and execvp().
- Built-in commands: exit, cd, help, and jobs.
- Command history with support for the last 20 commands.
- GNU Readline with command and file completion.
- Input and output redirection.
- Pipes for connecting multiple commands.
- Command sequencing using semicolons.
- Background job execution.
- Multiline if-then-else-fi conditionals.
- Shell variable assignment and expansion.
