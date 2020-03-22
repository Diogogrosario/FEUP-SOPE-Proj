#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include <string.h>

#define READ 0
#define WRITE 1

int b = 0;
int a = 0;
int B = 0;
int S = 0;
int block_size = 512;

int roundUp(int size) {
    if(size == 0)
        return 0;
    if(size < block_size)
        return 1;
    int aux = 0;
    if(size % block_size != 0) {
        aux = 1;
    }
    return size/block_size + aux;
}

void setFlags(int argc, char* argv[]) {
    for(int i = 2; i < argc; ++i) {
        if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytes")) {
            b = 1;
        }
        else if(!strcmp(argv[i], "-a") || !strcmp(argv[i], "--all")) {
            a = 1;
        }
        else if(!strcmp(argv[i], "-B")) {
            if(i+1 >= argc) {
                printf("No block size!\n");
                exit(4);
            }
            ++i;
            for(int j = 0; j < strlen(argv[i]); ++j) {
                if(argv[i][j] > '9' || argv[i][j] < '0') {
                    printf("Invalid block size!\n");
                    exit(5);
                }
            }
            B = 1;
            block_size = atoi(argv[i]);
        }
        else if(!strcmp("-S", argv[i])) {
            S = 1;
        }
        else {
            printf("Merdou as flags feio ein: i:%d   argv[i]:%s\n", i, argv[i]);
            exit(6);
        }
    }
}

int main(int argc, char *argv[])
{
    DIR* dir;
    int fd[2];
    pid_t pid;
    char name[FILENAME_MAX];
    struct dirent* d_entry; 
    struct stat stat_entry;  

    if(argc<2){
        printf("Too few arguments!\n");
        exit(1);
    }
    else if(argc > 2) {
        setFlags(argc, argv);
        if(b && B)
        {
            printf("Incompatible flags!\n");
            exit(7);
        }
    }

    if((dir = opendir(argv[1])) == NULL){
        perror(argv[1]);
        exit(2);
    }
    

    while((d_entry = readdir(dir)) != NULL){
        sprintf(name,"%s/%s",argv[1],d_entry->d_name);
        if(lstat(name,&stat_entry) == -1){  //Getting status
            perror("lstat error");
            exit(3);
        }
        //stat_entry.st_blksize = DEFLT_BLK_SIZE;
        if(S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, ".") && strcmp(d_entry->d_name, "..")){ //Found a dir
            pipe(fd);
            if((pid = fork()) == 0){ //Child
                char *aux[100];
                for(int i = 0; i < argc; ++i) {
                    if(i != 1) {
                        aux[i] = argv[i];
                    }
                    else
                    {
                        char temp[1000] = "";
                        sprintf(temp, "%s/%s", argv[1], d_entry->d_name);
                        aux[i] = temp;
                    }
                }
                execvp(argv[0],aux);
                exit(10);
            }
            else{ //parent
                wait(&pid);
            }
        }
        else if(S_ISREG(stat_entry.st_mode)){
            if(a == 1) {
                if(b == 1) {
                    printf("%-25s%12d%3d\n", name, (int)stat_entry.st_size, (int)stat_entry.st_nlink);
                }
                else {
                    printf("%-25s%12d%3d\n", name, roundUp(stat_entry.st_size), (int)stat_entry.st_nlink);
                }
            }
        }
    }

    //printing directory
    if(lstat(argv[1],&stat_entry) == -1){  //Getting status
            perror("lstat error");
            exit(3);
    }
    if(b == 1) {
        printf("%-25s%12d%3d\n", argv[1], (int)stat_entry.st_size, (int)stat_entry.st_nlink);
    }
    else {
        printf("%-25s%12d%3d\n", argv[1], roundUp(stat_entry.st_size), (int)stat_entry.st_nlink);
    }
}