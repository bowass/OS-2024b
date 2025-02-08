#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define coSize 100


// A struct for containing the given commands
typedef struct Node
{
    int count;
    char coStruct[coSize];
    struct Node* next;
    struct Node* before;
} node;

typedef node* nodeptr;


// An implementation for strdup, a function for duplicating a string
char* strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, str, len);
    }
    return copy;
}


// A helper function for deleteing whitespaces or '\n' at the end of a string
char* trim_trailing_whitespace(char* str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
    return str;
}


int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];

    node first;
    first.count = 0;
    first.next = NULL;
    first.before = NULL;
    nodeptr head = &first;
    nodeptr tail = &first;

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }


        // A flag to determine whether or not the command needs to be executed in the back
        int flag = 0;


        if (tail->count == 0)
        {
            tail->count = 1;
            strcpy(tail->coStruct, command);
        }
        else
        {
            nodeptr p = (nodeptr)malloc(sizeof(node));
            tail->next = p;
            p->next = NULL;
            p->before = tail;
            p->count = tail->count + 1;
            tail = p;
            strcpy(tail->coStruct, command);
        }


        // A special case for 'history' command
        // Note that only the prefix matters for this command
        if (strncmp(command, "history", 7) == 0)
        {
            nodeptr printer = tail;
            while (printer != NULL)
            {
                printf("%d %s", printer->count, printer->coStruct);
                printer = printer->before;
            }
            continue;
        }

        
        char* argv[coSize]; // An array of string for keeping the command's words, seperately
        int token = 0;
        char* command_copy = strdup(command);
        command_copy = trim_trailing_whitespace(command_copy);
        char* tokenized = strtok(command_copy, " ");


        // This while loop uses a copy of the command to store the command's seperate words,
        // each an element in 'args'
        while (tokenized != NULL)
        {
            argv[token] = strdup(tokenized);
            
            if (strcmp(argv[token], "&") == 0)
            {
                flag = 1;
                break;
            }
            
            if (strcmp(argv[token], "7\n") == 0)
            {
                printf("7 recognized \n");
            }

            token++;
            tokenized = strtok(NULL, " ");
        }
        argv[token] = NULL; // This element tells us where there are no more words in the command


        // Calling for the program's child
        int cpid = fork();

        if (cpid == -1) // If the child's run failed
        {
            perror("error");
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0)
        {
            if (execvp(argv[0], argv) == -1) // Calling for the command + cheking if the execution failed
            {
                perror("error");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (flag == 0) // Checking if we need to wait for the child to finish 
                // aka, whether or not we run the command at the front or at the back
            {
                wait(NULL); // Waiting for the child to finish running

            }
        }

        free(command_copy);
        for (int i = 0; i < token; i++)
        {
            if (argv[i] != NULL) { // Check if memory was allocated
                free(argv[i]);
            }
        }
    }

    return 0;
}
