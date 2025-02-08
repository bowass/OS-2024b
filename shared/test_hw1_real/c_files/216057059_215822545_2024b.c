#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100




typedef struct node
{
  char data[BUFFER_SIZE];
  struct node *next;
}node;

void printHistory(node* head, int count);
node* createNode(char str[]);
void freeList(node* head);

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char* cmd = NULL;
    char* argv[BUFFER_SIZE];
    char* pch;
    char* last;
    node* head = NULL;
    int count = 0;
    while(1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(command[strlen(command) - 1] == '\n'){
            command[strlen(command)-1] = '\0';
        }
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        if(count == 0){
            head = createNode(command);
        }
        else{
            node* newNode = createNode(command);
            newNode->next = head;
            head = newNode;
        }
        count++;
        if(strncmp(command, "history", 7) == 0){
            printHistory(head, count);
        }
        else{
            pch = strtok(command, " ");
            cmd = pch;
            int i = 0;
            while(pch != NULL){
                last = pch;
                if(strcmp(last, "&") != 0){
                    argv[i] = pch;
                }
                else{
                    i--;
                }
                pch = strtok(NULL, " ");
                i++;
            }
            argv[i] = NULL;
            pid_t pid = fork();
            //fail
            if(pid < 0){
                perror("error");
            }
            //son
            else if(pid == 0){
                const int result = execvp(cmd, argv);
                if (result < 0) {
                    perror("error");
                }
                exit(1);
            }
            //father
            else{
                if(strcmp(last, "&") != 0){
                    waitpid(pid, NULL,0);
                }
            }
        }
    }
    freeList(head);
    return 0;
}

void printHistory(node* head, int count){
    node* copy = head;
    int i = count;
    while(copy != NULL){
        fprintf(stdout, "%d %s\n",i , copy->data);
        copy = copy->next;
        i--;
    }
}


node* createNode(char str[]){
    node* newNode = malloc(sizeof(node));
    strcpy(newNode->data, str);
    return newNode;
}


void freeList(node* head){
    node* tmp = NULL;
    while(head!=NULL){
        tmp = head;
        head = head->next;
        free(tmp);
    }
}
