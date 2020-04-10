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
#include <signal.h>

#define READ 0
#define WRITE 1

#define LOG_FILENAME "LOG_FILENAME"
#define TIMENOW "TIMENOW"

int createLog = 0;
int b = 0;
int l=0;
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
int err;

pid_t groupId = 0;

DIR *dir;
FILE *file;

typedef struct
{
    long int size_total;
    int blocks;
} info;

enum logPrints
{
    ENTRY,
    CREATE,
    SEND_PIPE,
    RECV_PIPE,
    GRP_CTRL,
    GRP_OK,
    EXIT
};

void stringArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i)
        fprintf(file, "%s ", argv[i]);
    fprintf(file, "\n");
}

int roundUp(int size)
{
    return size * 512;
}

void setFlags(int argc, char *argv[])
{
    
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytes"))
        {
            b = 1;
        }
        else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--count-links"))
        {
            l=1;
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--all"))
        {
            a = 1;
        }
        else if (!strcmp(argv[i], "-B"))
        {
            if (!B)
            {
                if (i + 1 >= argc)
                {
                    printf("No block size!\n");
                    exit(1);
                }
                ++i;
                for (int j = 0; j < strlen(argv[i]); ++j)
                {
                    if (argv[i][j] > '9' || argv[i][j] < '0')
                    {
                        printf("Invalid block size!\n");
                        exit(1);
                    }
                }
                B = 1;
                block_size = atoi(argv[i]);
            }
        }
        else if (!strncmp("--block-size=", argv[i], 13))
        {
            if (!B)
            {
                if (strlen(argv[i]) == 14)
                {
                    printf("Invalid block size!\n");
                    exit(1);
                }
                char newBlockSize[10];
                for (int j = 13; j < strlen(argv[i]); ++j)
                    newBlockSize[j - 13] = argv[i][j];
                block_size = atoi(newBlockSize);
                B = 1;
            }
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
                exit(1);
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

            if ((dir = opendir(argv[i])) == NULL)
            {
                perror(argv[i]);
                exit(1);
            }
            pathPos = i;
        }
    }
}

void sig_handler(int intType)
{
    char choice;

    switch (intType)
    {
    case SIGINT:
        kill(-groupId, SIGSTOP);
        write(STDOUT_FILENO, "\nDo you really want to quit?(y/n)\n", 35);
        fflush(stdin);
        scanf("%c", &choice);
        while (choice != 'y' && choice != 'n' && choice != 'Y' && choice != 'N')
        {
            fflush(stdin);
            write(STDOUT_FILENO, "\nDo you really want to quit?(y/n)\n", 35);
            scanf("%c", &choice);
        }
        if (choice == 'y' || choice == 'Y')
        {
            kill(-groupId, SIGTERM);
            exit(1);
        }
        else
        {
            kill(-groupId, SIGCONT);
        }
        break;
    default:
        break;
    }
}

void printLog(int exitCode, int argc, char *argv[], enum logPrints type, info *myInfo, char *finished)
{

    clock_gettime(CLOCK_REALTIME, &tempo);
    float timeNow = tempo.tv_nsec / 1000000.0;

    switch (type)
    {
    case CREATE:
        fprintf(file, "%f - %.8d - CREATE -  ", timeNow - atof(getenv(TIMENOW)), getpid());
        stringArgs(argc, argv);
        break;
    case ENTRY:
        if (b)
            fprintf(file, "%f - %.8d - ENTRY - %ld %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), myInfo->size_total, argv[pathPos]);
        else
            fprintf(file, "%f - %.8d - ENTRY - %d %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), (myInfo->blocks + block_size - 1) / block_size, argv[pathPos]);
        break;
    case SEND_PIPE:
        fprintf(file, "%f - %.8d - SEND_PIPE - %d %ld\n", timeNow - atof(getenv(TIMENOW)), getpid(), (myInfo->blocks + block_size - 1) / block_size, myInfo->size_total);
        break;
    case RECV_PIPE:
        fprintf(file, "%f - %.8d - RECV_PIPE - %d %ld\n", timeNow - atof(getenv(TIMENOW)), getpid(), (myInfo->blocks + block_size - 1) / block_size, myInfo->size_total);
        break;
    case GRP_CTRL:
        fprintf(file, "%f - %.8d - SEND_PIPE - %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), finished);
        break;
    case GRP_OK:
        fprintf(file, "%f - %.8d - RECV_PIPE - %s\n", timeNow - atof(getenv(TIMENOW)), getpid(), finished);
        break;
    case EXIT:
        fprintf(file, "%f - %.8d - EXIT - %d\n", timeNow - atof(getenv(TIMENOW)), getpid(), exitCode);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{

    int fd[2];
    int killFirstChild[2];
    pid_t pid;
    char name[FILENAME_MAX];
    struct dirent *d_entry;
    struct stat stat_entry;
    int nChilds = 0;
    info *myInfo = malloc(sizeof(info));
    myInfo->size_total = 0;
    myInfo->blocks = 0;
    struct timespec auxTime;
    int firstChildPipe[2];

    if (argc < 3)
    {
        printf("Too few arguments!\n");
        exit(1);
    }

    else if (argc >= 3)
    {
        setFlags(argc, argv);
        if (b && B)
        {
            printf("Incompatible flags!\n");
            exit(1);
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
        groupId = getpgid(getpid());
    }
    else
    {
        if (signal(SIGINT, sig_handler) < 0)
        {
            //fprintf(stderr, "Unable to install SIGINT handler\n");
            exit(1);
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
        for (int j = 0; j < strlen(argv[pathPos]); j++)
        {
            if (j == strlen(argv[pathPos]) - 1)
            {
                if (argv[pathPos][j] == '/')
                {
                    sprintf(name, "%s%s", argv[pathPos], d_entry->d_name);
                }
                else
                {
                    sprintf(name, "%s/%s", argv[pathPos], d_entry->d_name);
                }
            }
        }

        if (lstat(name, &stat_entry) == -1)
        { //Getting status
            perror("lstat error");
            if (createLog)
            {
                printLog(1, 0, NULL, EXIT, NULL, NULL);
            }
            exit(1);
        }

        if (S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, ".") && strcmp(d_entry->d_name, ".."))
        { //Found a dir

            if (!timePassed && nChilds == 0)
            {
                pipe(firstChildPipe);
                pipe(killFirstChild);
            }
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
                        for (int j = 0; j < strlen(argv[i]); j++)
                        {
                            if (j == strlen(argv[i]) - 1)
                            {
                                if (argv[i][j] == '/')
                                {
                                    sprintf(temp2, "%s%s", argv[i], d_entry->d_name);
                                }
                                else
                                {
                                    sprintf(temp2, "%s/%s", argv[i], d_entry->d_name);
                                }
                            }
                        }

                        aux[i] = temp2;
                    }
                    else
                    {
                        aux[i] = argv[i];
                    }
                }

                if (createLog)
                {
                    printLog(0, argc, aux, CREATE, NULL, NULL);
                }

                if (nChilds == 0 && timePassed == 0)
                { // Primeiro Filho
                    dup2(killFirstChild[READ], 3);
                    if (setpgid(0, getpid()) == -1)
                    {
                        perror("Error on setpgid 1");
                        if (createLog)
                        {
                            printLog(1, 0, NULL, EXIT, NULL, NULL);
                        }
                        exit(1);
                    }
                    close(firstChildPipe[READ]);
                    write(firstChildPipe[WRITE], "finished", 9);

                    close(firstChildPipe[WRITE]);

                    close(killFirstChild[WRITE]);

                    if (createLog)
                        printLog(0, 0, NULL, GRP_CTRL, NULL, "finished");
                    //fclose(file);
                }
                else
                {

                    if ((setpgid(0, groupId)) == -1)
                    {
                        printf("\n\n%d\n\n", errno);
                        if (createLog)
                        {
                            printLog(1, 0, NULL, EXIT, NULL, NULL);
                        }
                        exit(1);
                    }
                }

                if (createLog)
                    fclose(file);

                execvp(argv[0], aux);
                exit(1);
            }
            else
            { //parent

                if (nChilds == 0 && timePassed == 0)
                {

                    close(firstChildPipe[WRITE]);
                    groupId = pid;
                    char recive[9];
                    while (strcmp(recive, "finished"))
                    {
                        read(firstChildPipe[READ], &recive, 9);
                    }

                    if (createLog)
                    {
                        printLog(0, 0, NULL, GRP_OK, NULL, "finished");
                    }
                    close(firstChildPipe[READ]);
                    close(killFirstChild[READ]);
                }
                nChilds++;
            }
        }
        else if (S_ISREG(stat_entry.st_mode))
        {

            myInfo->size_total += stat_entry.st_size;
            myInfo->blocks += roundUp(stat_entry.st_blocks);
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
                        printf("%d\t%s\n", (roundUp(stat_entry.st_blocks) + block_size - 1) / block_size, name);
                    }
                }
            }
        }
        else if (S_ISLNK(stat_entry.st_mode))
        {

            if (L)
            {
                if (stat(name, &stat_entry) == -1)
                { //Getting status
                    perror("lstat error");
                    if (createLog)
                    {
                        printLog(1, 0, NULL, EXIT, NULL, NULL);
                    }
                    exit(1);
                }

                if (S_ISDIR(stat_entry.st_mode) && strcmp(d_entry->d_name, argv[pathPos]))
                {
                    if (!timePassed && nChilds == 0)
                    {
                        pipe(firstChildPipe);
                        pipe(killFirstChild);
                    }

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
                                for (int j = 0; j < strlen(argv[i]); j++)
                                {
                                    if (j == strlen(argv[i]) - 1)
                                    {
                                        if (argv[i][j] == '/')
                                        {
                                            sprintf(temp2, "%s%s", argv[i], d_entry->d_name);
                                        }
                                        else
                                        {
                                            sprintf(temp2, "%s/%s", argv[i], d_entry->d_name);
                                        }
                                    }
                                }
                                aux[i] = temp2;
                            }
                            else
                            {
                                aux[i] = argv[i];
                            }
                        }

                        if (createLog)
                        {
                            printLog(0, argc, aux, CREATE, NULL, NULL);
                        }

                        if (nChilds == 0 && timePassed == 0)
                        { // Primeiro Filho
                            dup2(killFirstChild[READ], 3);
                            if (setpgid(0, getpid()) == -1)
                            {
                                perror("Error on setpgid 1");
                                if (createLog)
                                {
                                    printLog(1, 0, NULL, EXIT, NULL, NULL);
                                }
                                exit(1);
                            }
                            close(firstChildPipe[READ]);
                            write(firstChildPipe[WRITE], "finished", 9);

                            close(firstChildPipe[WRITE]);

                            close(killFirstChild[WRITE]);

                            if (createLog)
                            {
                                printLog(0, 0, NULL, GRP_CTRL, NULL, "finished");
                            }
                            //fclose(file);
                        }
                        else
                        {
                            if ((err = setpgid(0, groupId)) != 0)
                            {
                                perror("Error on setpgid 4");
                                if (createLog)
                                {
                                    printLog(1, 0, NULL, EXIT, NULL, NULL);
                                }
                                exit(1);
                            }
                        }

                        if (createLog)
                            fclose(file);

                        execvp(argv[0], aux);

                        exit(1);
                    }
                    else
                    { //parent
                        if (nChilds == 0 && timePassed == 0)
                        {
                            close(firstChildPipe[WRITE]);
                            groupId = pid;
                            char recive[9];
                            while (strcmp(recive, "finished"))
                            {
                                read(firstChildPipe[READ], &recive, 9);
                            }
                            close(firstChildPipe[READ]);

                            close(killFirstChild[READ]);

                            if (createLog)
                                printLog(0, 0, NULL, GRP_OK, NULL, "finished");
                        }

                        nChilds++;
                    }
                }
                else if (S_ISREG(stat_entry.st_mode))
                {

                    myInfo->size_total += stat_entry.st_size;
                    myInfo->blocks += roundUp(stat_entry.st_blocks);
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
                                printf("%d\t%s\n", (roundUp(stat_entry.st_blocks) + block_size - 1) / block_size, name);
                            }
                        }
                    }
                }
            }
            else
            {
                myInfo->size_total += stat_entry.st_size;
                myInfo->blocks += roundUp(stat_entry.st_blocks);
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
                            printf("%d\t%s\n", (roundUp(stat_entry.st_blocks) + block_size - 1) / block_size, name);
                        }
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
            printLog(1, 0, NULL, EXIT, NULL, NULL);
        }
        exit(1);
    }

    while (nChilds > 0) //receives all the children info
    {

        if (nChilds == 1 && !timePassed) // if its the main process and there is only one child left a message is sent for the child to finish
        {
            write(killFirstChild[WRITE], "done", 5);
            if (createLog)
                printLog(0, 0, NULL, GRP_CTRL, NULL, "done");
            close(killFirstChild[WRITE]);
        }

        wait(&pid);
        info *childInfo = malloc(sizeof(info));
        read(fd[READ], childInfo, sizeof(info));
        if (createLog)
        {
            printLog(0, 0, NULL, RECV_PIPE, childInfo, NULL);
        }

        myInfo->size_total += childInfo->size_total * S;
        myInfo->blocks += childInfo->blocks * S;

        nChilds--;
    }

    close(fd[READ]);

    //MYSELF
    if (S_ISLNK(stat_entry.st_mode))
    {
        if (stat(argv[pathPos], &stat_entry) == -1)
        { //Getting status
            perror("lstat error");
            if (createLog)
            {
                printLog(1, 0, NULL, EXIT, NULL, NULL);
            }
            exit(1);
        }
    }

    myInfo->size_total += stat_entry.st_size;
    myInfo->blocks += roundUp(stat_entry.st_blocks);

    if (max_depth >= -1)
    {
        if (b)
            printf("%ld\t%s\n", myInfo->size_total, argv[pathPos]);
        else
            printf("%d\t%s\n", (myInfo->blocks + block_size - 1) / block_size, argv[pathPos]);
    }

    if (createLog)
        printLog(0, argc, argv, ENTRY, myInfo, NULL);

    if (timePassed)
    {
        write(2, myInfo, sizeof(info));
        if (createLog)
        {
            printLog(0, 0, NULL, SEND_PIPE, myInfo, NULL);
        }
        close(fd[WRITE]);
    }

    if (getpid() == getpgrp() && timePassed == 1) //first child waits for the signal from the parent so that he can close after everyone else because of the process group
    {
        char receive[5] = "";
        while (strcmp(receive, "done"))
        {

            printf("%s", receive);
            read(3, &receive, 5);
        }
        if (createLog)
            printLog(0, 0, NULL, GRP_OK, NULL, "done");
        close(killFirstChild[READ]);
        return 0;
    }

    if (createLog)
    {
        printLog(0, 0, NULL, EXIT, NULL, NULL);
        fclose(file);
    }

    return 0;
}
