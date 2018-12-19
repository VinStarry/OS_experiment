#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define BUFLEN 50
#define MAX_TIME 100

enum ERR_LIST {CREATE_ERROR, INIT_ERROR, OPERAT_ERROR, DELETE_ERROR};

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
};

int pipefd[2];
int pid1, pid2;
int count = 0;
int sem_id = 0;

void counter_write(void);
void printer_read(void);
bool semaphore_P(unsigned short sum_num);
bool semaphore_V(unsigned short sum_num);
bool  set_value_semaphore(void);
void  delete_semaphore(void);
void  display_last_sem_error(int err_type);

int main(void) {

    /* Create two semaphore to deal with Synchronization */
    sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (sem_id == -1) {
        // error creating semaphore
        display_last_sem_error(CREATE_ERROR);
        _exit(1);
    }

    /* Set value for semaphores */
    if (set_value_semaphore() == false) {
        display_last_sem_error(INIT_ERROR);
        delete_semaphore();
        _exit(1);
    }

    if (pipe2(pipefd, O_NONBLOCK) == -1) {
        perror("Fail to create a pipe!\n");
        delete_semaphore();
        _exit(1);
    }

    pid1 = fork();
    if (pid1 < 0) {
        perror("Fail to create the counter process!\n");
        delete_semaphore();
    }
    else if (pid1 == 0) {
        printf("Success to create the counter process, %d\n", getpid());
        counter_write();
    }
    else {
        pid2 = fork();
        if (pid2 < 0) {
            perror("Fail to create the second printer process!\n");
            delete_semaphore();
        }
        else if (pid2 == 0) {
            printf("Success to create the second printer process, %d\n", getpid());
            printer_read();
        }
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
    return 0;
}

void counter_write(void) {
    close(pipefd[0]);
    char buf[BUFLEN];
    for (int i = 0; i < MAX_TIME; i++) {
        semaphore_P(0);
        count ++;
        sprintf(buf,"I send you %d times",count);
        buf[strlen(buf)] = '\0';
        printf("Counter: %s\n",buf);
        if (write(pipefd[1],buf,strlen(buf)) < 0) {
            perror("Write failed!\n");
            break;
        }
        semaphore_V(1);
    }
}

void printer_read(void) {
    close(pipefd[1]);
    char recv[BUFLEN];
    ssize_t length = 0;
    for (int i = 0; i < MAX_TIME; i++) {
        semaphore_P(1);
        if ((length = read(pipefd[0], recv, sizeof(recv))) == 0 ) {
            perror("Read failed!\n");
            break;
        }
        recv[length] = '\0';
        printf("Printer: receive %s\n", recv);
        semaphore_V(0);
    }
}

bool semaphore_P(unsigned short sum_num) {
    struct sembuf sem;
    sem.sem_num = sum_num;
    sem.sem_op = -1;
    sem.sem_flg = 0;
    if (semop(sem_id, &sem, 1) == -1) {
        perror("semaphore P failed!\n");
        display_last_sem_error(OPERAT_ERROR);
        return false;
    }
    return true;
}

bool semaphore_V(unsigned short sum_num) {
    struct sembuf sem;
    sem.sem_num = sum_num;
    sem.sem_op = 1;
    sem.sem_flg = 0;
    if (semop(sem_id, &sem, 1) == -1) {
        perror("semaphore V failed!\n");
        display_last_sem_error(OPERAT_ERROR);
        return false;
    }
    return true;
}

bool set_value_semaphore(void) {
    union semun sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }
    sem_union.val = 0;
    if (semctl(sem_id, 1, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }
    return true;
}

void delete_semaphore(void) {
    union semun sem_union;

    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1) {
        perror("Failed to delete semaphore!\n");
        display_last_sem_error(DELETE_ERROR);
    }
}

void display_last_sem_error(int err_type) {
    switch (err_type) {
        case CREATE_ERROR:
            switch (errno) {
                case EACCES:
                    printf("A semaphore set exists for key, but the calling process does not"
                           "have permission to  access  the  set\n");
                    break;
                case EEXIST:
                    printf("IPC_CREAT and IPC_EXCL were specified in semflg, but a semaphore"
                           "set already exists for key.\n");
                    break;
                case EINVAL:
                    printf("nsems  is less than 0 or greater than the limit on the number of"
                           "semaphores per semaphore set\n");
                    break;
                case ENOMEM:
                    printf("A semaphore set has to be created but the system does  not  have"
                           "enough memory for the new data structure\n");
                    break;
                default:
                    perror("Something wrong!\n");
                    break;
            }
        case OPERAT_ERROR:
            switch (errno) {
                case E2BIG:
                    printf("The argument nsops is greater than SEMOPM, the maximum number of "
                           "operations allowed per system call.");
                case EIDRM:
                    printf("The semaphore set was removed.");
                case EINTR:
                    printf("While  blocked  in this system call, the thread caught a signal\n");
                case EINVAL:
                    printf("The semaphore set doesn't exist, or semid is less than zero,  or "
                           "nsops has a nonpositive value.\n");
                default:
                    perror("Something wrong!\n");
                    break;
            }
        case INIT_ERROR:
        case DELETE_ERROR:
            switch (errno) {
                case EACCES:
                    printf("The argument cmd has one of the values GETALL,  GETPID,  GETVAL "
                           "GETNCNT,  GETZCNT, IPC_STAT, SEM_STAT, SETALL, or SETVAL and the "
                           "calling process does not have the required  permissions  on  the "
                           "semaphore  set and does not have the CAP_IPC_OWNER capability in "
                           "the user namespace that governs its IPC namespace.\n");
                case EIDRM:
                    printf("The semaphore set was removed.\n");
                case EINVAL:
                    printf("Invalid value for cmd or semid.  Or: for a  SEM_STAT  operation, "
                           "the  index  value  specified  in semid referred to an array slot "
                           "that is currently unused.\n");
                default:
                    perror("Something wrong!\n");
                    break;
            }

        default:
            perror("Something wrong!\n");
            break;
    }
}