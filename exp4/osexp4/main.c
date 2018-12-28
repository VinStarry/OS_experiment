#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>

void list_directory(char *dirname, char *run_type);
void list_file(char *filename, char file_tpye);
char get_file_type(struct stat *st);
void get_file_perm(struct stat *st, char *perm);
char *phrase_month(int rank);

int main(int argc,char *argv[]) {
    if(argc != 3){
        fprintf(stderr,"usage:%s [filepath]\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    struct stat fstat;
    if(lstat(argv[2], &fstat) == -1) {
        perror("stat error");
        exit(EXIT_FAILURE);
    }
    if(S_ISDIR(fstat.st_mode))
    {
        list_directory(argv[2], argv[1]);
    }
    else{
        list_file(argv[2], 'f');
    }
    return 0;
}

/**
 * list directory and its info, recursively
 * @param dirname directory's name
 */
void list_directory(char *dirname, char *run_type) {
    DIR *dir;
    char filename[100] = {0};
    dir = opendir(dirname);

    if(dir == NULL) {
        perror("opendir error");
        exit(EXIT_FAILURE);
    }
    struct dirent *dentry;
    while((dentry = readdir(dir)) != NULL)
    {
        char *fname;
        fname = dentry->d_name;
        if(strncmp(fname,".",1) == 0)
            continue;
        if(strncmp(fname,"..",2) == 0)
            continue;
        sprintf(filename,"%s/%s",dirname,fname);
        struct stat fstat;
        if(lstat(filename, &fstat) == -1) {
            perror("stat error");
            exit(EXIT_FAILURE);
        }
        if (strncmp("-lr", run_type, 3) == 0) {
            if(S_ISDIR(fstat.st_mode)) {
                list_file(filename,'d');
                list_directory(filename, run_type);
            }
            else {
                list_file(filename,'f');
            }
        }
        else {
            if(S_ISDIR(fstat.st_mode)) {
                list_file(filename,'d');
            }
            else {
                list_file(filename,'f');
            }
        }
    }
    closedir(dir);
}


/**
 * list file info and print it on the console
 * @param filename name of the file in present working directory
 */
void list_file(char *filename, char file_tpye) {
    struct stat tmpstat;

    if(lstat(filename, &tmpstat) == -1) {
        perror("stat error");
        exit(EXIT_FAILURE);
    }

    char buf[11]= {0};
    strcpy(buf,"----------");
    char type;
    type = get_file_type(&tmpstat);
    char *bname = basename(filename);
    buf[0] = type;
    if(type == 'l') {
        char content[1024];
        if(readlink(filename,content,1024) == -1) {
            perror("readlink error");
            exit(EXIT_FAILURE);
        }
        sprintf(bname,"%s -> %s",bname,content);
    }
    get_file_perm(&tmpstat,buf);
    struct tm *ftime;
    ftime = localtime(&tmpstat.st_mtime);
    char *month = phrase_month(ftime->tm_mon);

    //-rw-r--r--.        1         xyx      xyx      size      Jul 10 17:00      ls.c
    //type and perm  hard link    owner    group    文件大小    creation time   file name
    printf("%s %d %s %s %10ld %s %2d %2d:%2d ",
           buf,(int)tmpstat.st_nlink,
           getpwuid(tmpstat.st_uid)->pw_name,
           getgrgid(tmpstat.st_gid)->gr_name,
           tmpstat.st_size,
           month,
           ftime->tm_mday,
           ftime->tm_hour,
           ftime->tm_min);
    if (file_tpye == 'd') {
        printf("\033[34m");
        printf("%s\n", bname);
    }
    else if (buf[1] != '-' && buf[2] != '-' && buf[3] != '-') {
        printf("\033[36m");
        printf("%s\n", bname);
    }
    else {
        printf("%s\n", bname);
    }
    printf("\033[0m");
    free(month);
    month = NULL;
}

/**
 * get type of the file
 * @param st file state
 * @return one character that specifies the type of the file
 */
char get_file_type(struct stat *st) {
    char type = '-';
    switch (st->st_mode  & S_IFMT) {
        case S_IFSOCK:
            type = 's';
            break;
        case S_IFLNK:
            type = 'l';
            break;
        case S_IFREG:
            type = '-';
            break;
        case S_IFBLK:
            type = 'b';
            break;
        case S_IFDIR:
            type = 'd';
            break;
        case S_IFCHR:
            type = 'c';
            break;
        case S_IFIFO:
            type = 'p';
            break;
        default:
            break;
    }
    return type;
}

/**
 * get file's permissions
 * @param st file state
 * @param perm permission info
 */
void get_file_perm(struct stat *st, char *perm) {
    mode_t mode = st->st_mode;
    if (mode & S_IRUSR)
        perm[1] = 'r';
    if (mode & S_IWUSR)
        perm[2] = 'w';
    if (mode & S_IXUSR)
        perm[3] = 'x';
    if (mode & S_IRGRP)
        perm[4] = 'r';
    if (mode & S_IWGRP)
        perm[5] = 'w';
    if (mode & S_IXGRP)
        perm[6] = 'x';
    if (mode & S_IROTH)
        perm[7] = 'r';
    if (mode & S_IWOTH)
        perm[8] = 'w';
    if (mode & S_IXOTH)
        perm[9] = 'x';
}

/**
 * phrase month rank to three characters form
 * @param rank 0~11 specifies the rank of month
 * @return three characters like Jan,Feb,Mar ...
 */
char *phrase_month(int rank) {
    char *month = (char *)malloc(sizeof(char) * 4);
    char *monthes[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    for (int i = 0; i < 3; i++) {
        month[i] = (*(monthes + rank))[i];
    }
    month[3] = '\0';
    return month;
}