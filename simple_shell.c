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

        /* Tokenize the input into arguments*/
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

        /* Check for conccurent*/
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
            /* In parent process: if not background, wait for the child to finish */
            if (!background) {
                waitpid(pid, NULL, 0);
            }
        }
    }
    
    return 0;
}

