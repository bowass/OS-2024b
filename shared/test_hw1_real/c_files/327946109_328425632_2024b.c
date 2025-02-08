#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
void history();

struct node
{
    int number;
    char s[BUFFER_SIZE];
    struct node* next;
};
struct node* stack = NULL;

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    int com_count=0;

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        /*from here*/
        command[strcspn(command, "\n")] = '\0';
        com_count++;
        struct node* node = (struct node*)malloc(sizeof(struct node));
        node->number = com_count;
        strcpy(node->s, command);
        node->next = stack;
        stack = node;
        char r[BUFFER_SIZE];
        strcpy(r, command);
        char *pch=strtok(command," ");
        int count=0;
        char* cmd = pch;
        
        while(pch!=NULL)
        {
            count++;
            pch=strtok(NULL," ");
        }
        char* args[count+1];
        count=0;
        cmd = strtok(r, " ");
        while(cmd!=NULL)
        {
            args[count] = cmd;
            count++;
            cmd=strtok(NULL," ");
        }
        args[count]=NULL;
        if (strncmp(args[0], "history", 7) == 0)
        {
            history();
        }
        else if(strcmp(args[count-1],"&")==0)/*if the last char is & -we run on background*/
        {
            args[count - 1] =NULL;
            pid_t pid=fork();
            if (pid < 0)
            {
                perror("error");
                exit(EXIT_FAILURE);
            }
            else if(pid==0){
                execvp(args[0], args);
                perror("error");
                exit(EXIT_FAILURE);
            }
        }
        else{
            pid_t pid=fork();
            if (pid < 0)
            {
                perror("error");
                exit(EXIT_FAILURE);
            }   
            else if(pid==0)
            {
                execvp(args[0], args);
                perror("error");
                exit(EXIT_FAILURE);
            }
            else {
                wait(NULL);
            }
        }
    }
    return 0;
}



void history()
{
    int i;
    struct node* tmp;
    while (stack != NULL)
    {
        tmp = stack;
        stack = stack->next;
        printf("%d ", tmp->number);
        i = 0;
        while (tmp->s[i] != '\0' && i < BUFFER_SIZE)
        {
            printf("%c", tmp->s[i]);
            i++;
        }
        printf("\n");
        free(tmp);
    }
}
