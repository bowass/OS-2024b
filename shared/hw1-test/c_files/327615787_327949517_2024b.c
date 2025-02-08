#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

typedef struct Cell * CellPtr;

typedef struct Cell{
    CellPtr next;
    char cmd[BUFFER_SIZE];
    int index;
}Cell;



CellPtr add(CellPtr list){

    CellPtr temp;
    temp = (CellPtr)malloc(sizeof(Cell));
    if(temp == NULL){
        perror("error"); // malloc failed
        exit(1);
    }
    else{
        temp->next = list;
        if(list == NULL){
            temp->index = 1;
        }
        else{
            temp->index = list->index + 1;
        }
    }
    return temp;
}


void printHistory(const Cell *head) {
    const Cell *current = head;

    while (current != NULL) {
        printf("%d %s\n",current->index, current->cmd);
        current = current->next;
    }
}

/*void printHistory(CellPtr list){
    if(list == NULL){
        return;
    }
    printf("%d %s\n",list->index, list->cmd);
    printHistory(list->next);
}*/

void free_list(CellPtr list){
    CellPtr p;
    while(list != NULL){
        p = list->next;
        free(list);
        list = p;
    }
}

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char* argv[BUFFER_SIZE];
    int i=0;
    pid_t pid;
    CellPtr listHistory = NULL;

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

        if(command[strlen(command)-1] == '\n'){
            command[strlen(command) - 1] = '\0';
        }

        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        listHistory = add(listHistory);
        strcpy(listHistory->cmd,command);


        i=0;
        // separate to tokens
        argv[0] = strtok(command, " ");
        while(argv[i] != NULL){
            i++;
            argv[i] = strtok(NULL, " ");
        }

        // now the last word is in argv[i-1]

        if(strncmp(argv[0],"history",7) == 0){
            printHistory(listHistory);
        }
        else{
            pid = fork();


            if(pid == -1){
                perror("error");
            }
            else if(strcmp(argv[i-1],"&") == 0){
                argv[i-1] = NULL;
                if(pid == 0){

                    if(execvp(argv[0], argv) == -1){
                        perror("error");
                    }


                    exit(0);
                }
            }
            else{
                if(pid == 0){
                    if(execvp(argv[0], argv) == -1){
                        perror("error 1");
                    }

                    exit(0);
                }
                else{
                    waitpid(pid,NULL,0);
                }
            }
        }

    }

    free_list(listHistory);

    return 0;
}
