#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define BACKGROUND 1
#define FRONT 0

typedef struct Node {
    struct Node* next;
    struct Node* prev;
    char data[BUFFER_SIZE];
}Node;

typedef struct Stack {
    Node* top;
    Node* bottom;
    int len;
}Stack;

void initNode(Node* node, char newData[BUFFER_SIZE]) {
    newData = strtok(newData, "\n");
    strcpy(node->data, newData);
    node->next = node->prev = NULL;
}

void initStack(Stack* stack) {
    stack->len = 0;
    stack->top = stack->bottom = NULL;
}

void addNode(Stack* s, Node* n) {
    n->prev = s->top;
    if (s->top != NULL) {
        s->top->next = n;
    }
    s->top = n;
    s->len++;
}

void printStack(Stack* s) {
    Node* p = s->top;
    int i = s->len;
    while (p != NULL) {
        printf("%d %s\n", i, p->data);
        p = p->prev;
        i--;
    }
}

void deleteStack(Stack* s) {
    Node* p = s->top;
    Node* temp;
    while (p != NULL) {
        temp = p;
        p = p->prev;
        free(temp);
	s->top = p;
        s->len--;
    }
}

int main(void)
{
    int i = 0;
    Stack s;
    Node* p1;
    char* p2;
    int stateOfRun;
    char * args[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    initStack(&s);
    close(2);
    dup(1);
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
	     break;
        }
	/*add new command to the stack*/
	p1 = (Node*)malloc(sizeof(Node));
	initNode(p1, command);
	addNode(&s, p1);
        /*set args according to input*/
        i = 0;
        p2 = strtok(command, " \n");
        while(p2 != NULL){
            args[i++]= p2;
            p2 = strtok(NULL, " \n");
        }
	/*check for '&' at the end of the word*/
	/*and set stateOfRun accordingly*/ 
	if(strncmp(args[i-1],"&",1) == 0){
    	    args[i-1] = NULL;
	    stateOfRun = BACKGROUND;
    	}
    	else{
	    stateOfRun = FRONT;
    	    args[i] = NULL;
    	}
    	/*Check if command is history and execute accordingly*/
    	if(strncmp(command, "history", 7) == 0){
    	    printStack(&s);
    	}
	else{
	    pid_t pid = fork();
            if(pid > 0){
                /*parent process - wait if required*/
	        if(stateOfRun == FRONT){ 
		    /*if wait failed print error*/
                    if(wait(NULL)<0) perror("error");
	        }
            }
	    else if(pid < 0){
	        /*fork() failed*/
	        perror("error");
            }
	    else{
                /*child process - execute then exit*/
                if(execvp(args[0], args) == -1) perror("error");
	        exit(0);
            }
	}
    }
    deleteStack(&s);
    return 0;
}
