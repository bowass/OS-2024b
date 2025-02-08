#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100


void call_function_foreground(char * args[])
{
    pid_t p = fork();

    if (p < 0) {
        perror("fork failed");
        return;
    }
    if (p == 0) { // child
        int status = execvp(args[0], args);
        if (status < 0) {
            perror("error");
        }

        exit(0);
    }
    waitpid(p, NULL, 0);
}

void call_function_background(char * args[])
{
    pid_t p = fork();

    if (p < 0) {
        perror("fork");
    } else if (p == 0) { // child
        int status = execvp(args[0], args);
        if (status < 0) {
            perror("error");
        }
        exit(0);
    }
}


void find_function(char input[])
{
    char *args[BUFFER_SIZE];
    input[strcspn(input, "\n")] = 0;
    int bg_command = input[strlen(input) - 1] == '&';

    int index = 0;
    char * pch = strtok(input, " ");
    while (pch != NULL) {
        pch[strcspn(pch, "\n")] = 0;
        if (strcmp(pch, "&") != 0) {
            args[index] = pch;
            index++;
        }
        pch = strtok(NULL, " ");
    }
    args[index] = NULL;

    if (bg_command == 1) {
        call_function_background(args);
    } else {
        call_function_foreground(args);
    }
}

int startsWith(const char *string, const char *prefix) {
    for (int i = 0; i < strlen(prefix); i++) {
        if (string[i] != prefix[i]) {
            return 0;
        }
    }
    return 1;
}

// We decided to make dynamic array for history

typedef struct {
    char** array;
    int used;
    int size;
} DA;

void initArray(DA* a, int size)
{
    a->array = (char**)malloc(size * sizeof(char*));
    for (int i = 0; i < size; i++) {
        a->array[i] = (char*)malloc((BUFFER_SIZE + 1) * sizeof(char));
    }
    a->used = 0;
    a->size = size;
}

void insertDA(DA* a, char* element)
 {
    if (a->used == a->size)
    {
        a->size *= 2;
        a->array = (char**)realloc(a->array, a->size * sizeof(char*));
        for (int i = a->used; i < a->size; i++)
        {
            a->array[i] = (char*)malloc((BUFFER_SIZE+1) * sizeof(char));
        }
    }
    strncpy(a->array[a->used++], element, BUFFER_SIZE);
    a->array[a->used - 1][BUFFER_SIZE] = '\0';
}

void freeDA(DA* a) {
    for (int i = 0; i < a->size; i++) {
        free(a->array[i]);
    }
    free(a->array);
    a->array = NULL;
    a->used =0;
    a->size = 0;
}


int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    DA history;
    initArray(&history, 5);

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

        insertDA(&history, command);

        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        } else if (startsWith(command, "history") == 1) {
            for (int i = history.used - 1; i >= 0; i--) {
                printf("%d %s", i+1, history.array[i]);
            }
        } else {
            find_function(command);
        }
    }

    freeDA(&history);

    return 0;
}



