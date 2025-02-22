#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int N = 10;  
    int i;
    pid_t original_parent = getpid(); 

    for (i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid < 0) {  
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    if (getpid() == original_parent) {
        char command[256];
        sprintf(command, "pstree -lp %d | grep -v -E '(sh|pstree)' > process_tre.txt", original_parent);
        system(command);
    }


    sleep(10);

    return 0;
}

