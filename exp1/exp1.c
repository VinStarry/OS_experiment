#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define BUFLEN 50

int pipefd[2];
int pid1, pid2;
int count = 0;
unsigned long sleeptime = 1;

void killForks(int sig_no);
void childrenKilled(int sig_no);
void pip1_write(void);
void pip2_read(void);

int main(int argc, const char *argv[]) {

    if (argc > 2) {
        if (!strcmp(argv[1], "-t")) {
            sleeptime = strtoul(argv[2], NULL, 10);
        }
    }

    if (pipe(pipefd) == -1) {
        perror("Fail to create a pipe!\n");
        _exit(1);
    }

    signal(SIGINT, killForks);

    pid1 = fork();
    if (pid1 < 0) {
        perror("Fail to create the first child process!\n");
    }
    else if (pid1 == 0) {
        printf("Success to create the first child process, %d\n", getpid());
        signal(SIGUSR1, childrenKilled);
        pip1_write();
    }
    else {
        pid2 = fork();
        if (pid2 < 0) {
            perror("Fail to create the second child process!\n");
        }
        else if (pid2 == 0) {
            printf("Success to create the second child process, %d\n", getpid());
            signal(SIGUSR1, childrenKilled);
            pip2_read();
        }
        waitpid(pid1, NULL, 0);
    	waitpid(pid2, NULL, 0);
    	printf("Parent Process is killed!\n");
    }
    return 0;
}

void killForks(int sig_no) {
    switch (sig_no) {
        case SIGINT:
            printf("Receive SIGINT signal, kill forks\n");
            kill(pid1, SIGUSR1);
            kill(pid2, SIGUSR1);
            break;

        default:
            printf("Something wrong, kill forks!\n");
            kill(pid1, SIGUSR1);
            kill(pid2, SIGUSR1);
            break;
    }
}

void childrenKilled(int sig_no) {
    close(pipefd[0]);
    close(pipefd[1]);
    if (pid1 == 0) {
        printf("Child Process 1 is Killed by Parent!\n");
        _exit(0);
    }
    if (pid2 == 0) {
        printf("Child Process 2 is Killed by Parent!\n");
        _exit(0);
    }
}

void pip1_write(void) {
    close(pipefd[0]);
    char buf[BUFLEN];
    while (true) {
    	sleep((unsigned int)sleeptime);
        count ++;
        sprintf(buf,"I send you %d times",count);
        buf[strlen(buf)] = '\0';
        printf("Children 1: %s\n",buf);
        write(pipefd[1],buf,strlen(buf));
    }
}

void pip2_read(void) {
    close(pipefd[1]);
    char recv[BUFLEN];
    int length = 0;
    while (1) {
    	sleep((unsigned int)sleeptime);
        length = read(pipefd[0], recv, sizeof(recv));
        recv[length] = '\0';
        printf("Children 2: receive %s\n", recv);
    }
}