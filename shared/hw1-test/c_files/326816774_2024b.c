#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

//Node
typedef struct Node {
    char commandName[BUFFER_SIZE];
    struct Node* next;
}Node;

//createNode
Node* createNode(char* commandName){
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL){
        perror("error");
    }
    strcpy(newNode->commandName, commandName);
    newNode->next = NULL;
    return newNode;
}
//print history
void printList(Node* node){
    //count Nodes.
    int serialNum = 0;
    Node* newPtr = node;
    while(newPtr != NULL){
        serialNum++;
        newPtr = newPtr->next;
    }

    //print.
    while(node != NULL){
        printf("%d %s\n", serialNum, node->commandName);
        node = node->next;
        serialNum--;
    }
}
int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];

    Node* lastNode = NULL;
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);

        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = '\0';

        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }


        int runBackground = 0;

        //update history list
        Node* newNode = createNode(command);
        newNode->next = lastNode;
        lastNode = newNode;

        char* splitedInput = strtok(command," ");

        // check for history call
        if(!strncmp(splitedInput,"history",7)){
            printList(lastNode);
        }
        //no history or exit then regular commands
        else{

            char *argv[BUFFER_SIZE];//max possible size
            int wordCounter = 0;

            //update argv
            while (splitedInput != NULL) {
                argv[wordCounter++] = splitedInput;
                splitedInput = strtok(NULL, " ");
            }
            argv[wordCounter] = NULL;//now the reeding on argv will stop at wordCounter and not BUFFER_SIZE.

            //check if background run &.
            if (strcmp(argv[wordCounter - 1], "&") == 0) {
                argv[--wordCounter] = NULL;
                runBackground = 1;
            }








            //run command
            pid_t p = fork();
            if(p == 0){
                execvp(argv[0],argv);
                perror("error");//if command path not found will do perror("error") like which.
            } else {
                // wait if run in front.
                if(!runBackground){
                    wait(NULL);
                }
            }

        }
    }

    //free history.
    Node* tmp;
    while (lastNode != NULL){
       tmp = lastNode;
       lastNode = lastNode->next;
       free(tmp);
    }
    exit(0);
}
