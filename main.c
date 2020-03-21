#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    DIR* dir;
    pid_t pid;
    char name[FILENAME_MAX];
    struct dirent* d_entry; 
    struct stat stat_entry;  
    
    if(argc<2){
        printf("Too few arguments!\n");
        exit(1);
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
        if(S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, ".") && strcmp(d_entry->d_name, "..")){ //Found a dir
            if((pid = fork()) == 0){ //Child
                sprintf(argv[1],"%s/%s",argv[1],d_entry->d_name);
                printf("Directory!   ");
                printf("%s\n",argv[1]);
                execvp("./simpledu",argv);
            }
            else{ //parent
                wait(&pid);
            }
        }
        else if(S_ISREG(stat_entry.st_mode)){
            printf("Regular!   ");
            printf("%s\n",d_entry->d_name);
        }
    }
}