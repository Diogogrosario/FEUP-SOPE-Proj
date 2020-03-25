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
int block_size = 1024;
int max_depth = 0;
int depth_index = -1;
int L = 0;

void printArgs(int argc, char*argv[]) {
    for(int i = 0; i < argc; ++i)
        printf("i:%d           argv[]:%s\n", i, argv[i]);
}

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
    //printArgs(argc, argv);
    for(int i = 3; i < argc; ++i) {
        if(strcmp(argv[1],"-l")){
			printf("Invalid usage!\n");
			exit(8);
		}
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
        else if(!strncmp("--max_depth=", argv[i], 12)) {
            if(strlen(argv[i]) == 12) {
                printf("Invalid depth!\n");
                exit(9);
            }
            char depth[10];
            for(int j = 12; j < strlen(argv[i]); ++j) 
                depth[j-12] = argv[i][j];
            max_depth = atoi(depth) - 1;
            if(max_depth == -2) 
                exit(0);
            depth_index = i;
        }
        else if(!strcmp("-L", argv[i])) {
            L = 1;
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

    if(argc < 3){
        printf("Too few arguments!\n");
        exit(1);
    }
    else if(argc > 3) {
        setFlags(argc, argv);
        if(b && B)
        {
            printf("Incompatible flags!\n");
            exit(7);
        }
    }

    if((dir = opendir(argv[2])) == NULL){
        perror(argv[2]);
        exit(2);
    }


    while((d_entry = readdir(dir)) != NULL){
        sprintf(name,"%s/%s",argv[2],d_entry->d_name);
        if(lstat(name,&stat_entry) == -1){  //Getting status
            perror("lstat error");
            exit(3);
        }
        if(S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, ".") && strcmp(d_entry->d_name, "..")) { //Found a dir
            pipe(fd);
            if((pid = fork()) == 0){ //Child
                char *aux[1000];
                for(int i = 0; i < argc; ++i) {
                    
                    if(i == depth_index) {
                        char *temp1 = malloc(sizeof(char)*1000);
                        sprintf(temp1, "--max_depth=%d", max_depth);
                        aux[i] = temp1;
                    } else if (i == 2) {
                        char *temp2 = malloc(sizeof(char)*1000);
                        sprintf(temp2, "%s/%s", argv[i], d_entry->d_name);
                        aux[i] = temp2;
                    } else {
                        aux[i] = argv[i];
                    }
                }
                execvp(argv[0],aux);
                exit(10);
            }
            else { //parent
                wait(&pid);
            }
        }
        else if(S_ISREG(stat_entry.st_mode)) {
            if(a == 1) {
                if(b == 1) {
                    printf("%-25s%12d%3d\n", name, (int)stat_entry.st_size, (int)stat_entry.st_nlink);
                }
                else {
                    printf("%-25s%12d%3d\n", name, roundUp(stat_entry.st_size), (int)stat_entry.st_nlink);
                }
            }
        }
        else if(S_ISLNK(stat_entry.st_mode)) {
            if(a == 1) {
                if(b == 1) {
                    printf("%-25s%12d%3d\n", name, (int)stat_entry.st_size, (int)stat_entry.st_nlink);
                }
                else {
                    printf("%-25s%12d%3d\n", name, roundUp(stat_entry.st_size), (int)stat_entry.st_nlink);
                }
            }
            if(L == 1) {
               // execvp();
            }
        } 
    }

    //printing directory
    if(lstat(argv[2],&stat_entry) == -1){  //Getting status
            perror("lstat error");
            exit(3);
    }
    if(b == 1) {
        printf("%-25s%12d%3d\n", argv[2], (int)stat_entry.st_size, (int)stat_entry.st_nlink);
    }
    else {
        printf("%-25s%12d%3d\n", argv[2], roundUp(stat_entry.st_size), (int)stat_entry.st_nlink);
    }
}