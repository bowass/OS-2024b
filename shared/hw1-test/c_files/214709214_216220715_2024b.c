#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

struct Node {
    char* command;
    struct Node* next;
};

void concatenateStrings(char *str1, char *str2, char *res)
{
    strcat(res, str1);
    strcat(res, " ");
    strcat(res, str2);
    strcat(res," ");
    return;
}

void MakeNullArray(char *argv[BUFFER_SIZE] )
{
    for (size_t i = 0; i < BUFFER_SIZE; i++)
    {
        argv[i]=NULL;
    }
}

struct Node* createNode(char* command) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        perror("error\n");
        exit(1);
    }
    newNode->command = command;
    newNode->next = NULL;
    return newNode;
}

size_t calculateLength(struct Node* head) {
    size_t length = 0;
    struct Node* current = head;
    while (current != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

void printList(struct Node* head)
{
    size_t length = calculateLength(head);
    while (head != NULL) 
    {
        printf("%d %s\n",length, head->command);
        head = head->next;
        length--;
    }
}

void insert(struct Node** headRef, char* command) {
    struct Node* newNode = createNode(command);
    newNode->next = *headRef;
    *headRef = newNode;
}

void freeList(struct Node* head) {
    struct Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

char* arrayOfStrToStr(char*argv[BUFFER_SIZE])
{
    char* res = (char*)malloc(BUFFER_SIZE * sizeof(char));
    char* str1;
    char * str2;
    for (size_t i = 0; (i+1 < BUFFER_SIZE)&&argv[i+1]!=NULL;)
    {
        str1=argv[i];
        str2=argv[i+1];
        concatenateStrings(str1,str2,res);
        i+=2;
    }
    if(argv[1]==NULL)
    {
        concatenateStrings(argv[0],"",res);
    }
    return res;
} 
int main(void)
{
    close(2);
    dup(1);
    size_t counter;
    pid_t pid ;
    char command[BUFFER_SIZE];
    char * pch;
    char * cmd;
    char * argv[BUFFER_SIZE];
    char* strCommand;
    struct Node* head = NULL;
    
    while (1)
    {
        counter = 0;
        MakeNullArray(argv);
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        command[strcspn(command,"\n")]='\0';
        pch = strtok (command, " ");
        cmd = pch;
        while (pch != NULL)
        {
            argv[counter++]=pch;
            pch = strtok (NULL, " ");
        }
        //printf("1\n");
        strCommand=arrayOfStrToStr(argv);
        //printf("2\n");
        insert(&head,strCommand);
       /*for (size_t i = 0; (i < BUFFER_SIZE)&&argv[i]!=NULL; i++)
       {
            printf("argv[%d] is : %s\n",i,argv[i]);
       }
       continue;*/
        if(strncmp(command,"history",7)==0)
        {
           printList(head);
        }
        else 
        {
            if(strncmp(argv[counter-1],"&",1)==0)
            {
                argv[counter-1]=NULL;
                pid = fork();
                if(pid==0)
                {
                    if(execvp(cmd, argv)==-1)
                    {
                        perror("error\n");
                    }
                    exit(1);
                }
                else if(pid<0)
                {
                    perror("error");
                }
            }
            else
            {
                pid=fork();
                if(pid==0)
                {
                   if(execvp(cmd, argv)<0)
                   {
                    perror("error\n");
                   }
                   exit(1);
                }
                else if(pid>0)
                {
                    wait(NULL);
                }
                else
                {
                    perror("error");
                }
            }
        }
    }
    
    return 0;
}
