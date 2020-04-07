#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#define READ 0
#define WRITE 1

#define LOG_FILENAME "LOG_FILENAME"
#define TIMENOW "TIMENOW"


int createLog = 0;
int b = 0;
int a = 0;
int B = 0;
int S = 1;
int block_size = 1024;
int max_depth = 0;
int depth_index = -1;
int L = 0;
int timePassed = 0;
struct timespec tempo;
int pathPos;

DIR *dir;
FILE *file;

pid_t groupId;

typedef struct
{
    long int size_total;
    int blocks;
} info;

void stringArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i)
        fprintf(file, "%s ", argv[i]);
    fprintf(file, "\n");
}

int roundUp(int size)
{
    int aux = 0;
    double add = ceil(4096.0/block_size);
    while(size > 4096){
        aux += ceil(add);
        size-=4096;
    }
    
    if(size == 0){
        return aux;
    }
    else{
        return aux + ceil(add);
    }
}

void setFlags(int argc, char *argv[])
{
    if (strcmp(argv[1], "-l") && strcmp(argv[1], "--count-links"))
    {
        printf("Invalid usage!\n");
        exit(8);
    }
    for (int i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytes"))
        {
            b = 1;
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--all"))
        {
            a = 1;
        }
        else if (!strcmp(argv[i], "-B"))
        {
            if (i + 1 >= argc)
            {
                printf("No block size!\n");
                if (createLog)
                    fprintf(file, "tempo - %.8d - EXIT - 4\n", getpid());
                exit(4);
            }
            ++i;
            for (int j = 0; j < strlen(argv[i]); ++j)
            {
                if (argv[i][j] > '9' || argv[i][j] < '0')
                {
                    printf("Invalid block size!\n");
                    if (createLog)
                        fprintf(file, "tempo - %.8d - EXIT - 5\n", getpid());
                    exit(5);
                }
            }
            B = 1;
            block_size = atoi(argv[i]);
        }
        else if (!strcmp("-S", argv[i]))
        {
            S = 0;
        }
        else if (!strncmp("--max-depth=", argv[i], 12))
        {
            if (strlen(argv[i]) == 12)
            {
                printf("Invalid depth!\n");
                if (createLog)
                    fprintf(file, "tempo - %.8d - EXIT - 9\n", getpid());
                exit(9);
            }
            char depth[10];
            for (int j = 12; j < strlen(argv[i]); ++j)
                depth[j - 12] = argv[i][j];
            max_depth = atoi(depth) - 1;
            depth_index = i;
        }
        else if (!strcmp("-L", argv[i]))
        {
            L = 1;
        }
        else
        {
            pathPos = i;
            if ((dir = opendir(argv[i])) == NULL)
            {
                perror(argv[i]);
                if (createLog)
                {
                    clock_gettime(CLOCK_REALTIME, &tempo);
                    float timeNow;
                    timeNow = tempo.tv_nsec / 1000000.0;
                    fprintf(file, "%f - %.8d - EXIT - 2\n", timeNow - atof(getenv(TIMENOW)), getpid());
                }
                exit(2);
            }
        }
    }
}

void sig_handler(int intType) {
    char choice;

    switch (intType)
    {
    case SIGINT:
        kill(-groupId, SIGSTOP);
        write(STDOUT_FILENO, "\nDo you really want to quit?(y/n)\n", 35);
        fflush(stdin);
        scanf("%c", &choice);
        while(choice != 'y' && choice != 'n' && choice != 'Y' && choice != 'N') {
            fflush(stdin);
            write(STDOUT_FILENO, "\nDo you really want to quit?(y/n)\n", 35);
            scanf("%c", &choice);
        }
        if(choice == 'y' || choice == 'Y') {
            kill(-groupId, SIGTERM);
            exit(50);
        }
        else {
            kill(-groupId, SIGCONT);
        }
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{

    int fd[2];
    pid_t pid;
    char name[FILENAME_MAX];
    struct dirent *d_entry;
    struct stat stat_entry;
    int nChilds = 0;
    info *myInfo = malloc(sizeof(info));
    myInfo->size_total = 0;
    myInfo->blocks = 0;
    struct timespec auxTime;

    groupId = getpgid(getpid());
    

    if (argc < 3)
    {
        printf("Too few arguments!\n");
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = tempo.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - EXIT - 1\n", timeNow - atof(getenv(TIMENOW)), getpid());
        }
        exit(1);
    }

    else if (argc >= 3)
    {
        setFlags(argc, argv);
        if (b && B)
        {
            printf("Incompatible flags!\n");
            if (createLog)
            {
                clock_gettime(CLOCK_REALTIME, &tempo);
                float timeNow;
                timeNow = tempo.tv_nsec / 1000000.0;
                fprintf(file, "%f - %.8d - EXIT - 7\n", timeNow - atof(getenv(TIMENOW)), getpid());
            }
            exit(7);
        }
    }

    if (getenv(LOG_FILENAME) != NULL)
    {
        if (getenv(TIMENOW) != NULL)
        {
            file = fopen(getenv(LOG_FILENAME), "a");
        }
        else
        {
            file = fopen(getenv(LOG_FILENAME), "w");
            fclose(file);
            file = fopen(getenv(LOG_FILENAME), "a");
        }
        createLog = 1;
    }

    if (getenv(TIMENOW) != NULL)
    {
        timePassed = 1;

       
    }
    else
    {
        if (signal(SIGINT, sig_handler) < 0)
        {
            fprintf(stderr, "Unable to install SIGINT handler\n");
            exit(31);
        }

        clock_gettime(CLOCK_REALTIME, &tempo);
        char *timeInString = malloc(sizeof(char) * 5);
        sprintf(timeInString, "%f", tempo.tv_nsec / 1000000.0);
        setenv(TIMENOW, timeInString, 0);
    }

    clock_gettime(CLOCK_REALTIME, &auxTime);

    pipe(fd);
    
    
    
    while ((d_entry = readdir(dir)) != NULL)
    {

        sprintf(name, "%s/%s", argv[pathPos], d_entry->d_name);
        if (L)
        {
            if (stat(name, &stat_entry) == -1)
            { //Getting status
                perror("lstat error");
                if (createLog)
                {
                    clock_gettime(CLOCK_REALTIME, &tempo);
                    float timeNow;
                    timeNow = tempo.tv_nsec / 1000000.0;
                    fprintf(file, "%f - %.8d - EXIT - 3\n", timeNow - atof(getenv(TIMENOW)), getpid());
                }
                exit(3);
            }
        }
        else
        {
            if (lstat(name, &stat_entry) == -1)
            { //Getting status
                perror("lstat error");
                if (createLog)
                {
                    clock_gettime(CLOCK_REALTIME, &tempo);
                    float timeNow;
                    timeNow = tempo.tv_nsec / 1000000.0;

                    fprintf(file, "%f - %.8d - EXIT - 3\n", timeNow - atof(getenv(TIMENOW)), getpid());
                }
                exit(3);
            }
        }
        if (S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, ".") && strcmp(d_entry->d_name, ".."))
        { //Found a dir

            if ((pid = fork()) == 0)
            { //Child
                
                dup2(fd[WRITE], 2);

                char *aux[1000];
                int i;
                for (i = 0; i < argc; ++i)
                {

                    if (i == depth_index)
                    {
                        char *temp1 = malloc(sizeof(char) * 1000);
                        sprintf(temp1, "--max-depth=%d", max_depth);
                        aux[i] = temp1;
                    }
                    else if (i == pathPos)
                    {
                        char *temp2 = malloc(sizeof(char) * 1000);
                        sprintf(temp2, "%s/%s", argv[i], d_entry->d_name);
                        aux[i] = temp2;
                    }
                    else
                    {
                        aux[i] = argv[i];
                    }
                }

                if (createLog)
                {
                    clock_gettime(CLOCK_REALTIME, &tempo);
                    float timeNow;
                    timeNow = tempo.tv_nsec / 1000000.0;
                    fprintf(file, "%f - %.8d - CREATE -  ", timeNow - atof(getenv(TIMENOW)), getpid());
                    stringArgs(argc, aux);
                    fclose(file);
                }

                if(nChilds == 0 && timePassed == 0) {
                    if(setpgid(0, getpid()) != 0) {
                        perror("Error on setpgid");
                        exit(40);
                    }
                }
                else if(timePassed == 0) {
                    if(setpgid(0, groupId) != 0) {
                        perror("Error on setpgid");
                        exit(40);
                    }
                }
                else {
                    if(setpgid(0, getpgid(getpid())) != 0) {
                        perror("Error on setpgid");
                        exit(40);
                    }
                }

                execvp(argv[0], aux);
                exit(10);
            }
            else
            { //parent
                if(nChilds == 0 && timePassed != 1)
                    groupId = pid;
                nChilds++;
            }
        }
        else if (S_ISREG(stat_entry.st_mode))
        {
            myInfo->size_total += stat_entry.st_size;
            myInfo->blocks += roundUp(stat_entry.st_size);
            if (max_depth >= 0)
            {
                if (a)
                {
                    if (b)
                    {
                        printf("%d\t%s\n", (int)stat_entry.st_size, name);
                    }
                    else
                    {
                        printf("%d\t%s\n", roundUp(stat_entry.st_size), name);
                    }
                }
            }
        }
        else if (S_ISLNK(stat_entry.st_mode))
        {
            myInfo->size_total += stat_entry.st_size;
            myInfo->blocks += roundUp(stat_entry.st_size);

            if (max_depth >= 0)
            {
                if (a)
                {
                    if (b)
                    {
                        printf("%d\t%s\n", (int)stat_entry.st_size, name);
                    }
                    else
                    {
                        printf("%d\t%s\n", roundUp(stat_entry.st_size), name);
                    }
                }
            }
        }
    }

    //printing directory
    if (lstat(argv[pathPos], &stat_entry) == -1)
    { //Getting status
        perror("lstat error");
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = tempo.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - EXIT - 3\n", timeNow - atof(getenv(TIMENOW)), getpid());
        }
        exit(3);
    }

    while (nChilds > 0)
    {
        wait(&pid);
        info *childInfo = malloc(sizeof(info));
        read(fd[READ], childInfo, sizeof(info));
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = tempo.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - RECV_PIPE - %d %ld\n", timeNow - atof(getenv(TIMENOW)), getpid(), childInfo->blocks, childInfo->size_total);
        }

        myInfo->size_total += childInfo->size_total * S;
        myInfo->blocks += childInfo->blocks * S;

        nChilds--;
    }

    close(fd[READ]);

    //MYSELF
    myInfo->size_total += stat_entry.st_size;
    myInfo->blocks += roundUp(stat_entry.st_size);

    if (b == 1)
    {
        if (max_depth >= -1)
            printf("%ld\t%s\n", myInfo->size_total, argv[pathPos]);
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = auxTime.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - ENTRY - %ld %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), myInfo->size_total, argv[pathPos]);
        }
    }
    else
    {
        if (max_depth >= -1)
            printf("%d\t%s\n", myInfo->blocks, argv[pathPos]);
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = auxTime.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - ENTRY - %d %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), myInfo->blocks, argv[pathPos]);
        }
    }

    if (timePassed)
    {
        write(2, myInfo, sizeof(info));
        if (createLog)
        {
            clock_gettime(CLOCK_REALTIME, &tempo);
            float timeNow;
            timeNow = tempo.tv_nsec / 1000000.0;
            fprintf(file, "%f - %.8d - SEND_PIPE - %d %ld\n", timeNow - atof(getenv(TIMENOW)), getpid(), myInfo->blocks, myInfo->size_total);
        }
        close(fd[WRITE]);
    }

    if (createLog)
    {
        clock_gettime(CLOCK_REALTIME, &tempo);
        float timeNow;
        timeNow = tempo.tv_nsec / 1000000.0;
        fprintf(file, "%f - %.8d - EXIT - 0\n", timeNow - atof(getenv(TIMENOW)), getpid());
        fclose(file);
    }

    return 0;
}
