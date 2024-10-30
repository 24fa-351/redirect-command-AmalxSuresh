#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// find absolute path 
char *find_command_path(char *command) {

    // Search in PATH environment variable
    char *path = getenv("PATH");
    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return strdup(full_path);  // Found absolute path
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;  // Command not found
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <inputFile> <command> <outputFile>\n", argv[0]);
        return 1;
    }

    char *input = argv[1];
    char *cmd = argv[2];
    char *output = argv[3];

    // Split command into program and arguments
    char *program = strtok(cmd, " ");
    char **newargv = malloc(sizeof(char*) * (argc - 1));
    newargv[0] = program;
    int ix = 1;
    while ((newargv[ix++] = strtok(NULL, " ")) != NULL);

    // Find the absolute path for the command
    char *cmd_path = find_command_path(program);

    newargv[0] = cmd_path;  // Use the absolute path

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (strcmp(input, "-") != 0) {
            int input_fd = open(input, O_RDONLY);
            if (input_fd < 0) {
                perror("Error opening input file");
                _exit(1);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if (strcmp(output, "-") != 0) {
            int output_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output_fd < 0) {
                perror("Error opening output file");
                _exit(1);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Execute command with arguments using execve
        execve(cmd_path, newargv, NULL);
        perror("exec failed"); 
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("Parent PID %d, forked child PID %d. Parent exiting.\n", getpid(), pid);
    } else {
        perror("fork failed");
        free(newargv);
        free(cmd_path);
        return 1;
    }

    free(newargv);
    free(cmd_path);
    return 0;
}
