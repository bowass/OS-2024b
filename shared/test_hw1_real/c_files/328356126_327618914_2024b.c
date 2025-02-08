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
    int num;
    char cmd[BUFFER_SIZE];
    struct node* prev;
};
struct node* stack = NULL;

int main(void){
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    int  cmd_num=0;
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        cmd_num++;
        struct node* node=(struct node*)malloc(sizeof(struct node));
        node->prev=stack;
        node->num=cmd_num;
        char* tmp= strtok (command ," ");
        strcpy(node->cmd,tmp);
        int i=0;
        char* name;
        char* arg[BUFFER_SIZE];
        while((arg[i]=strtok (NULL, " "))!=NULL){
            if(i==0){
                strcpy(name,arg[i]);
            }
            strcpy(node->cmd," ");
            strcpy(node->cmd,arg[i]);
            i++;
        }
        stack= node;
        if (strncmp(command, "history", 7) == 0) {
          history();
        }
        pid_t pid=fork();
        if (pid < 0){
                perror("error");
                exit(EXIT_FAILURE);
            }   
        if(strcmp(arg[--i],"&")==0){
            arg[i] =NULL;
            if(pid==0){
                execvp(name, arg);
                perror("error");
                exit(EXIT_FAILURE);
            }
        }
        
        else{
            if(pid==0){
                execvp(name, arg);
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

void history(){
     struct node* temp;
           while(stack!=NULL){
           temp =stack;
           stack=stack ->prev;
           printf("%d %s\n", temp->num, temp->cmd);
           }
           free(temp);
    }
    

