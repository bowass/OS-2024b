#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define HISTORY_LENGTH 100

char history[HISTORY_LENGTH][BUFFER_SIZE];

int starts_with(char* prefix, char* string)
{
    size_t len_prefix = strlen(prefix),
           len_string = strlen(string);
    return len_string < len_prefix ? 0 : memcmp(prefix, string, len_prefix) == 0;
}

void add_history(char *command)
{
    int i = 0;
    int j;
    while (history[i][0] != '\0')
        i++;
    if (i == HISTORY_LENGTH - 1)
    {
        perror("error");
    }
    else
    {
        for (j = 0; j < BUFFER_SIZE; j++)
        {
            history[i][j] = command[j];
        }
    }
}

void print_history()
{
    int i = 0;
    int j = 0;
    while (history[i][0] != '\0')
        i++;
    for (i--; i >= 0; i--)
    {
        j = 0;
        fprintf(stdout, "%d\t", i + 1);
        while (history[i][j] != '\0')
        {
            fprintf(stdout, "%c", history[i][j]);
            j++;
        }
    }
}


int main(void)
{
    char* substr;
    int argc;
    int i;
    char command[BUFFER_SIZE];
    char* argv[50];
    char* ptr;
    int pid;
    close(2);
    dup(1);

    for (i = 0; i < HISTORY_LENGTH; i++)
    {
        history[i][0] = '\0';
    }

    while (1)
    {
        argc = 0;
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        /*

        ^
        |
            Add your code here
        |
        v

        */
        add_history(command);

        if (starts_with("history",command))
        {
            print_history();
            continue;
        }
        substr = strtok(command," ");

        while(substr != NULL)
        {
            argv[argc] = substr;
            substr = strtok(NULL," ");
            argc++;
        }

        ptr = argv[argc - 1];

        while(*ptr != '\n')
        {
            ptr++;
        }

        *ptr = '\0';

        argv[argc] = NULL;

        if (strcmp(argv[argc - 1],"&") == 0)
        {
            argv[--argc] = NULL;

            pid = fork();

            if(pid==0)
            {
                execvp(argv[0],argv);
                perror("error");
                exit(-1);
            }
        }
        else
        {
           pid = fork();

            if(pid==0)
            {
                execvp(argv[0],argv);
                perror("error");
                exit(-1);
            }
            else
            {
                wait(NULL);
            }
        }
    }

    return 0;
}
