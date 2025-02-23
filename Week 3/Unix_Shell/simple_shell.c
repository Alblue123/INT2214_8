#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80  /* 80 chars per line, per command */

int main(void)
{
    char lastCommand[MAX_LINE] = "";  // History buffer for the most recent command
    int should_run = 1;
    
    while (should_run) {
        char input[MAX_LINE];
        int background = 0;
        
        printf("osh> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        
        /* Remove trailing newline if present */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        /* History feature: if user enters "!!", use the last command */
        if (strcmp(input, "!!") == 0) {
            if (strlen(lastCommand) == 0) {
                printf("No commands in history.\n");
                continue;
            } else {
                printf("%s\n", lastCommand);
                strcpy(input, lastCommand);
            }
        } else {
            /* Save current command to history */
            strcpy(lastCommand, input);
        }
        
        /* Tokenize input into an array of tokens */
        char *tokens[MAX_LINE/2 + 1];
        int numTokens = 0;
        char *token = strtok(input, " ");
        while (token != NULL && numTokens < (MAX_LINE/2 + 1)) {
            tokens[numTokens++] = token;
            token = strtok(NULL, " ");
        }
        tokens[numTokens] = NULL;
        
        /* Check if the command includes a pipe '|' */
        int pipeIndex = -1;
        for (int i = 0; i < numTokens; i++) {
            if (strcmp(tokens[i], "|") == 0) {
                pipeIndex = i;
                break;
            }
        }
        
        if (pipeIndex != -1) {
            /* Handle pipe commands */
            tokens[pipeIndex] = NULL;  // Terminate the left-hand command's token list
            char **leftCmd = tokens;            // Command before the pipe
            char **rightCmd = &tokens[pipeIndex + 1];  // Command after the pipe
            
            int fd[2];
            if (pipe(fd) == -1) {
                perror("pipe failed");
                continue;
            }
            
            /* Fork to create a process that sets up the pipeline */
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                continue;
            } else if (pid == 0) {
                /* In the child process, fork again to handle both commands */
                pid_t pid2 = fork();
                if (pid2 < 0) {
                    perror("fork failed");
                    exit(1);
                } else if (pid2 == 0) {
                    /* Grandchild process: Execute right-hand command */
                    close(fd[1]);  // Close write end (not needed here)
                    if (dup2(fd[0], STDIN_FILENO) < 0) {
                        perror("dup2 failed for right command");
                        exit(1);
                    }
                    close(fd[0]);
                    if (execvp(rightCmd[0], rightCmd) < 0) {
                        perror("execvp failed for right command");
                        exit(1);
                    }
                } else {
                    /* Child process: Execute left-hand command */
                    close(fd[0]);  // Close read end (not needed here)
                    if (dup2(fd[1], STDOUT_FILENO) < 0) {
                        perror("dup2 failed for left command");
                        exit(1);
                    }
                    close(fd[1]);
                    if (execvp(leftCmd[0], leftCmd) < 0) {
                        perror("execvp failed for left command");
                        exit(1);
                    }
                }
            } else {
                /* In the parent process: close both ends of the pipe so no extraneous FDs remain */
                close(fd[0]);
                close(fd[1]);
                waitpid(pid, NULL, 0);
            }
            
            continue;  // Process next command iteration
        }
        else {
            /* Handle commands with possible input/output redirection */
            char *args[MAX_LINE/2 + 1];
            int argIndex = 0;
            char *inputFile = NULL;
            char *outputFile = NULL;
            
            for (int i = 0; i < numTokens; i++) {
                if (strcmp(tokens[i], "<") == 0) {
                    if (i + 1 < numTokens) {
                        inputFile = tokens[i + 1];
                        i++;  // Skip the filename token
                    } else {
                        fprintf(stderr, "Error: no input file specified\n");
                        break;
                    }
                } else if (strcmp(tokens[i], ">") == 0) {
                    if (i + 1 < numTokens) {
                        outputFile = tokens[i + 1];
                        i++;  // Skip the filename token
                    } else {
                        fprintf(stderr, "Error: no output file specified\n");
                        break;
                    }
                } else {
                    args[argIndex++] = tokens[i];
                }
            }
            args[argIndex] = NULL;
            
            /* Check for background execution using "&" at the end */
            if (argIndex > 0 && strcmp(args[argIndex - 1], "&") == 0) {
                background = 1;
                args[argIndex - 1] = NULL;
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                continue;
            } else if (pid == 0) {
                /* Child process: handle input and output redirection if specified */
                if (inputFile != NULL) {
                    int fdIn = open(inputFile, O_RDONLY);
                    if (fdIn < 0) {
                        perror("open input file failed");
                        exit(1);
                    }
                    if (dup2(fdIn, STDIN_FILENO) < 0) {
                        perror("dup2 for input failed");
                        exit(1);
                    }
                    close(fdIn);
                }
                if (outputFile != NULL) {
                    int fdOut = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fdOut < 0) {
                        perror("open output file failed");
                        exit(1);
                    }
                    if (dup2(fdOut, STDOUT_FILENO) < 0) {
                        perror("dup2 for output failed");
                        exit(1);
                    }
                    close(fdOut);
                }
                if (execvp(args[0], args) < 0) {
                    perror("execvp failed");
                    exit(1);
                }
            } else {
                /* Parent process: wait for child unless running in background */
                if (!background) {
                    waitpid(pid, NULL, 0);
                }
            }
        }
    }
    
    return 0;
}
