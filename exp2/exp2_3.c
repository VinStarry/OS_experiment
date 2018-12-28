#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdbool.h>

enum ERR_LIST {CREATE_ERROR, INIT_ERROR, OPERAT_ERROR, DELETE_ERROR};

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
};

int philo_id[5] = {0,1,2,3,4};

void *philosopher(void * id);
bool  semaphore_P(unsigned short sum_num);
bool  semaphore_V(unsigned short sum_num);
bool  set_value_semaphore(void);
void  delete_semaphore(void);
void  display_last_sem_error(int err_type);
void  eat(unsigned short id) {printf("philosopher %d is eating meal!\n", id);}

int sem_id = 0;
int cnt = 0;

int main(void) {
    pthread_t ph0, ph1, ph2, ph3, ph4;
    int iret1 = 0, iret2 = 0, iret3 = 0, iret4 = 0, iret5 = 0;

    /* Create two semaphore to deal with Synchronization */
    sem_id = semget(IPC_PRIVATE, 6, IPC_CREAT | 0666);
    if (sem_id == -1) {
        // error creating semaphore
        display_last_sem_error(CREATE_ERROR);
        exit(1);
    }

    /* Set value for semaphores */
    if (set_value_semaphore() == false) {
        display_last_sem_error(INIT_ERROR);
        delete_semaphore();
        exit(1);
    }

    /* Create 5 independent threads each of which will execute function */
    if ((iret1 = pthread_create(&ph0, NULL, philosopher, (void *)(& philo_id[0]))) != 0) {
        // error creating the first thread
        perror("Create thread for philosopher 0 failed!\n");
        delete_semaphore();
        exit(1);
    }

    if ((iret2 = pthread_create(&ph1, NULL, philosopher, (void *)(& philo_id[1]))) != 0) {
        // error creating the second thread
        perror("Create thread for philosopher 1 failed!\n");
        delete_semaphore();
        exit(1);
    }

    if ((iret3 = pthread_create(&ph2, NULL, philosopher, (void *)(& philo_id[2]))) != 0) {
        // error creating the third thread
        perror("Create thread for philosopher 2 failed!\n");
        delete_semaphore();
        exit(1);
    }

    if ((iret4 = pthread_create(&ph3, NULL, philosopher, (void *)(& philo_id[3]))) != 0) {
        // error creating the fourth thread
        perror("Create thread for philosopher 3 failed!\n");
        delete_semaphore();
        exit(1);
    }

    if ((iret5 = pthread_create(&ph4, NULL, philosopher, (void *)(& philo_id[4]))) != 0) {
        // error creating the fifth thread
        perror("Create thread for philosopher 4 failed!\n");
        delete_semaphore();
        exit(1);
    }

    /* Wait till threads are complete before main continues */
    pthread_join(ph0, NULL);
    pthread_join(ph1, NULL);
    pthread_join(ph2, NULL);
    pthread_join(ph3, NULL);
    pthread_join(ph4, NULL);
    delete_semaphore();
    return 0;
}

void *philosopher(void *id) {
    unsigned short pher_id = *(unsigned short *)id;
    while(true) {
        semaphore_P(5);
        semaphore_P(pher_id);
        semaphore_P((unsigned short)((pher_id + 1) % 5));
        eat(pher_id);
        semaphore_V((unsigned short)((pher_id + 1) % 5));
        semaphore_V(pher_id);
        semaphore_V(5);
    }
    return NULL;
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

    sem_union.val = 1;
    if (semctl(sem_id, 1, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }

    sem_union.val = 1;
    if (semctl(sem_id, 2, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }

    sem_union.val = 1;
    if (semctl(sem_id, 3, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }

    sem_union.val = 1;
    if (semctl(sem_id, 4, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        display_last_sem_error(INIT_ERROR);
        return false;
    }

    sem_union.val = 4;
    if (semctl(sem_id, 5, SETVAL, sem_union) == -1) {
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