#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

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
                /* Echo the most recent command */
                printf("%s\n", lastCommand);
                /* Replace input with the last command */
                strcpy(input, lastCommand);
            }
        } else {
            /* Save the current command to history */
            strcpy(lastCommand, input);
        }

        /* Tokenize the input into arguments */
        char *token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; 

        /* If no command is entered, continue */
        if (i == 0) {
            continue;
        }

        /* Check for background execution using "&" */
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
            /* In child process: execute the command */
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

