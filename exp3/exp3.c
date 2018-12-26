#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#define BUF_NUM 10
#define BUF_SZ 4096
#define SEM_WRITE 0
#define SEM_READ 1
#define SHM_KEY 1700

typedef struct shared_buffer{
    char                  buffer[BUF_SZ];   // buffer to read
    ssize_t               length;           // length of buffer
    struct shared_buffer* next;             // next shared buffer
}shared_buffer;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
};

void create_attach_shared_memory();
void delete_detach_shared_memory();
void create_semaphores_set();
void delete_semaphores_set();
bool semaphore_P(unsigned short sum_num);
bool semaphore_V(unsigned short sum_num);
void write_buffer(const char *file_name);
void read_buffer(const char *dest_name);
long get_file_size(const char *file_name, long sz);
double show_size(long file_size, char *unit);
char* phrase_title(const char* file_name);
void  circle_title(char** title_name);

int sb_id[10] = {0,0,0,0,0,0,0,0,0,0};
shared_buffer *sb_set = NULL;
int semaphore_id = 0;
int pid1 = 0, pid2 = 0;

int main(int argc, const char *argv[]) {
    char *file_name = "/home/vinstarry/CLionProjects/osexp3/wushuang.mp4";
    char *dest_name = "/home/vinstarry/CLionProjects/osexp2/wushuang.mp4";
    /* create a set of shared memory */
    create_attach_shared_memory();
    /* create a set of semaphores */
    create_semaphores_set();

    /* create two process read_buffer and write_buffer */
    pid1 = fork();
    if (pid1 < 0) {
        perror("Fail to create the first child process!\n");
        exit(EXIT_FAILURE);
    }
    else if (pid1 == 0) {
        printf("Success to create the first child process, %d\n", getpid());
        write_buffer(file_name);
        exit(EXIT_SUCCESS);
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("Fail to create the read child process!\n");
        exit(EXIT_FAILURE);
    }
    else if (pid2 == 0) {
        printf("Success to create the write child process, %d\n", getpid());
        read_buffer(dest_name);
        exit(EXIT_SUCCESS);
    }

    /* wait for two process being finished */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    printf("Parent Process is killed!\n");

    /* delete semaphores */
    delete_semaphores_set();
    /* detach delete shared memories */
    delete_detach_shared_memory();
    return 0;
}

/**
 *  create_attach_shared_memory
 *  create shared memory and attach them to virtual address space
 *  link them as a circle linked list
 */
void create_attach_shared_memory() {
    int temp_id = 0;
    if ( (temp_id = shmget((SHM_KEY), sizeof(shared_buffer), IPC_CREAT | 0666) ) == -1) {
        perror("Shared memory get failed!\n");
        exit(EXIT_FAILURE);
    }
    if ( (sb_set = shmat(temp_id, NULL, 0)) == (shared_buffer *)-1 ) {
        perror("Shared memory attached failed!\n");
        exit(EXIT_FAILURE);
    }
    sb_id[0] = temp_id;
    shared_buffer *cur_ptr = sb_set;
    for (int i = 1; i < BUF_NUM; i++) {
        if ( (temp_id = shmget((SHM_KEY + i), sizeof(shared_buffer), IPC_CREAT | 0666) ) == -1) {
            perror("Shared memory get failed!\n");
            exit(EXIT_FAILURE);
        }
        if ( (cur_ptr->next = shmat(temp_id, NULL, 0)) == (shared_buffer *)-1 ) {
            perror("Shared memory attached failed!\n");
            exit(EXIT_FAILURE);
        }
        cur_ptr = cur_ptr->next;
        sb_id[i] = temp_id;
        cur_ptr->next = NULL;
        cur_ptr->length = 0;
    }
    cur_ptr->next = sb_set;
}

/**
 *  detach shared meory and free virtual space
 */

void delete_detach_shared_memory() {
    shared_buffer *curr = sb_set, *prev = NULL;
    for (int i = 0; i < BUF_NUM; i++) {
        prev = curr;
        curr = (i == BUF_NUM - 1) ? NULL : curr->next;
        if (shmdt(prev) == -1) {
            perror("Shared memory detech failed\n");
            exit(EXIT_FAILURE);
        }
        prev = NULL;
    }
    for (int i = 0; i < BUF_NUM; i++) {
        if (shmctl(sb_id[i], IPC_RMID, 0) == -1) {
            perror("Shared memory delete failed\n");
            exit(EXIT_FAILURE);
        }
    }
    sb_set = NULL;
}

/**
 *  create 2 semaphores,
 *  the SEM_READ specifies how many buffer can be written
 *  the SEM_WRITE specifies how many buffer can be read
 */

void create_semaphores_set() {
    union semun sem_union;

    semaphore_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semaphore_id == -1) {
        perror("Error creating semaphore set!\n");
        exit(EXIT_FAILURE);
    }

    sem_union.val = BUF_NUM;
    if (semctl(semaphore_id, SEM_WRITE, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        exit(EXIT_FAILURE);
    }
    sem_union.val = 0;
    if (semctl(semaphore_id, SEM_READ, SETVAL, sem_union) == -1) {
        perror("Fail to set value for semaphore!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 *  remove semaphores set
 */

void delete_semaphores_set() {
    union semun sem_union;
    sem_union.val = 0;

    if (semctl(semaphore_id, 0, IPC_RMID, sem_union) == -1) {
        perror("Failed to delete semaphore!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * semaphore P operation
 * @param sum_num : specify which semaphore to operate on
 * @return true if the operation succeed, false otherwise
 */

bool semaphore_P(unsigned short sum_num) {
    struct sembuf sem;
    sem.sem_num = sum_num;
    sem.sem_op = -1;
    sem.sem_flg = 0;
    if (semop(semaphore_id, &sem, 1) == -1) {
        perror("semaphore P failed!\n");
        return false;
    }
    return true;
}

/**
 * semaphore V operation
 * @param sum_num : specify which semaphore to operate on
 * @return true if the operation succeed, false otherwise
 */

bool semaphore_V(unsigned short sum_num) {
    struct sembuf sem;
    sem.sem_num = sum_num;
    sem.sem_op = 1;
    sem.sem_flg = 0;
    if (semop(semaphore_id, &sem, 1) == -1) {
        perror("semaphore V failed!\n");
        return false;
    }
    return true;
}

/**
 * writer from file to buffer, child process 1
 * @param file_name : source file's path
 */

void write_buffer(const char *file_name) {
    int write_fd = 0;
    bool finished = false;
    if ( (write_fd = open(file_name,O_RDONLY)) == -1) {
        perror("cannot open file for reading\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Read from %s.\n", file_name);
        long total_length = get_file_size(file_name, 0);

        ssize_t len = 0;
        shared_buffer *cur = sb_set;
        semaphore_P(SEM_WRITE);
        cur->length = total_length;
        cur = cur->next;
        semaphore_V(SEM_READ);
        while (!finished) {
            semaphore_P(SEM_WRITE);
            len = read(write_fd, cur->buffer, BUF_SZ);
            cur->length = len;
            if (len != BUF_SZ)
                finished = true;
            cur = cur->next;
            semaphore_V(SEM_READ);
        }
        semaphore_P(SEM_WRITE);
        cur->length = BUF_SZ + 1;
        semaphore_V(SEM_READ);
    }
    close(write_fd);
    exit(EXIT_SUCCESS);
}

/**
 * read buffer: read from buffer and write to dest file
 * @param dest_name : destination file path
 */

void read_buffer(const char *dest_name) {
    int read_fd = 0;
    long total_length = 0;
    long sent = 0;
    char unit1, unit2;
    double total_convert = 0.0, sent_convert = 0.0;
    int bar_pos = 0;
    char bar[21] = {0};
    char *title = phrase_title(dest_name);

    if ((read_fd = open(dest_name, O_RDWR | O_CREAT, S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH)) == -1) {
        perror("cannot write file for writing\n");
        exit(EXIT_FAILURE);
    }
    else {
        shared_buffer *cur = sb_set;
        semaphore_P(SEM_READ);
        total_length = cur->length;
        total_convert = show_size(total_length ,&unit1);
        cur = cur->next;
        semaphore_V(SEM_WRITE);
        for (int i = 0; i < 20; i++)
            bar[i] = ' ';
        bar[20] = '\0';
        while (true) {
            semaphore_P(SEM_READ);
            ssize_t sz = cur->length;
            if (sz > BUF_SZ)
                break;
            sz = write(read_fd, cur->buffer, (size_t)sz);
            sent += sz;
            cur = cur->next;
            semaphore_V(SEM_WRITE);
            sent_convert = show_size(sent, &unit2);
            while (bar_pos < (int)(sent * 100 / total_length * 20 / 100) && bar_pos < 20) {
                bar[bar_pos++] = '#';
                circle_title(&title);
            }
            printf("[%-15s][%-20s] %d%%",title,bar,(int)(sent * 100 / total_length));
            printf(", %4.2lf%c/%4.2lf%c\r",sent_convert,unit2,total_convert,unit1);
            //printf("\033[?25l");
            fflush(stdout);
        }
    }
    close(read_fd);
    printf("Write to %s finished, total length is: ", dest_name);
    get_file_size(dest_name, 0);
    exit(EXIT_SUCCESS);
}

/**
 * return size of the file
 * @param file_name if NULL, none use, otherwise use stat() to return its size
 * @param sz if file_name == NULL, sz is used to convert size to B/KB/MB/GB
 * @return size (Byte)
 */

long get_file_size(const char *file_name, long sz) {
    long file_size = 0;
    if (file_name == NULL) {
        file_size = sz;
    }
    else {
        struct stat tbuf;
        stat(file_name, &tbuf);
        file_size = tbuf.st_size;
    }
    if (file_size > 1024 * 1024 * 1024) {
        printf("File size is %.2lfG\n", (double)file_size / 1024.0 / 1024.0 / 1024.0);
    }
    else if (file_size > 1024 * 1024) {
        printf("File size is %.2lfM\n", (double)file_size / 1024.0 / 1024.0);
    }
    else if (file_size > 1024) {
        printf("File size is %.2lfK\n", (double)file_size / 1024.0);
    }
    else {
        printf("File size is %ldB\n", file_size);
    }
    return file_size;
}

double show_size(long file_size, char *unit) {
    if (file_size > 1024 * 1024 * 1024) {
        *unit = 'G';
        return (double)(file_size / 1024.0 / 1024.0 / 1024.0);
    }
    else if (file_size > 1024 * 1024) {
        *unit = 'M';
        return (double)(file_size / 1024.0 / 1024.0);
    }
    else if (file_size > 1024) {
        *unit = 'K';
        return (double)(file_size / 1024.0);
    }
    else {
        *unit = 'B';
        return (double)(file_size);
    }
}

char* phrase_title(const char* file_name) {
    bool found = false;
    bool change = false;
    char* title = (char *)malloc(16 * sizeof(char));
    int pos = 0;
    size_t length = strlen(file_name);
    for(int i = (int)(length - 1); i >= 0; i--) {
        if(file_name[i] == '/') {
            found = true;
            pos = i;
            break;
        }
    }
    if(!found) {
        pos = 0;
    }
    for(int i = 0; i < 14; i++) {
        title[i] = change ? (char)' ' : file_name[pos + i + 1];
        if(title[i] == '\0') {
            change = true;
            title[i] = ' ';
        }
    }
    title[15] = '\0';
    return title;
}

void circle_title(char** title_name) {
    char temp = (*title_name)[0];
    for (int i = 0; i < 15; i++) {
        (*title_name)[i - 1] = (*title_name)[i];
        if ((*title_name)[i - 1] == '\0')
            (*title_name)[i - 1] = ' ';
    }
    (*title_name)[14] = temp;
    (*title_name)[15] = '\0';
}
