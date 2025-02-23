#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  

#define MAX_LINE 80 /* 80 chars per line, per command */

int main(void)
{
    char *args[MAX_LINE/2 + 1];
    char lastCommand[MAX_LINE] = "";  // History buffer for the most recent command
    int should_run = 1;

    while (should_run) {
        char input[MAX_LINE];
        int i = 0;
        int background = 0;
        char *input_file = NULL;   // For input redirection (<)
        char *output_file = NULL;  // For output redirection (>)

        printf("osh> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        /* Remove trailing newline, if any */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        /* Check for history command "!!" */
        if (strcmp(input, "!!") == 0) {
            if (strlen(lastCommand) == 0) {
                printf("No commands in history.\n");
                continue;
            } else {
                printf("%s\n", lastCommand);
                strcpy(input, lastCommand);
            }
        } else {
            /* Save the current command to history */
            strcpy(lastCommand, input);
        }

        /* Tokenize the input into arguments and check for redirection tokens */
        char *token = strtok(input, " ");
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                // Next token should be the input file
                token = strtok(NULL, " ");
                if (token != NULL) {
                    input_file = token;
                }
            } else if (strcmp(token, ">") == 0) {
                // Next token should be the output file
                token = strtok(NULL, " ");
                if (token != NULL) {
                    output_file = token;
                }
            } else {
                args[i++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  

        /* If no command is entered, continue */
        if (i == 0) {
            continue;
        }

        /* Check for background execution*/
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            background = 1;
            args[i - 1] = NULL;
        }

        /* Fork a child process */
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            /* In child process: handle redirection if specified */
            if (input_file != NULL) {
                int fd_in = open(input_file, O_RDONLY);
                if (fd_in < 0) {
                    perror("open input file failed");
                    exit(1);
                }
                if (dup2(fd_in, STDIN_FILENO) < 0) {
                    perror("dup2 for input failed");
                    exit(1);
                }
                close(fd_in);
            }
            if (output_file != NULL) {
                int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("open output file failed");
                    exit(1);
                }
                if (dup2(fd_out, STDOUT_FILENO) < 0) {
                    perror("dup2 for output failed");
                    exit(1);
                }
                close(fd_out);
            }

            /* Execute the command */
            if (execvp(args[0], args) == -1) {
                perror("execvp failed");
            }
            exit(1); 
        } else {
            /* In parent process: wait for child if not a background process */
            if (!background) {
                waitpid(pid, NULL, 0);
            }
        }
    }

    return 0;
}

