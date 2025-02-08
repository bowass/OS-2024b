#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100


typedef struct History {
    char data[BUFFER_SIZE];
    struct History* next;
    int index;
} History;

int check_type(char command[])
{

    char* word, * last_word;
    char cmd[BUFFER_SIZE];
    strcpy(cmd, command);

    word = strtok(cmd, " \n");
    if (strncmp(word, "history", 7) == 0)
    {
        return 2;
    }

    last_word = NULL;
    while (word != NULL)
    {
        if (word != " ")
            last_word = word;
        word = strtok(NULL, " \n");
    }

    if (last_word != NULL && last_word[strlen(last_word) - 1] == '&')
    {
        return 1;
    }
    return 0;

}


void print_history(const History* head)
{
    const History* current = head;

    while (current != NULL) {
        printf("%d %s", current->index, current->data);
        current = current->next;

    }

}


void execute(char command[])
{

    int i = 0;
    char* word;
    char* argv[BUFFER_SIZE + 1]; //what size

    char cmd[BUFFER_SIZE];
    strcpy(cmd, command);


    word = strtok(cmd, " \n");

    if (word == NULL) {
        perror("error");
        return;
    }


    argv[i++] = word;

    while ((word = strtok(NULL, " \n")) != NULL && i < BUFFER_SIZE) {
        if (*word == '&')
            break;
        argv[i++] = word;
    }

    argv[i] = NULL;

    execvp(argv[0], argv);

    perror("error");
}




void run_front(char command[])
{

    pid_t p = fork();

    if (p == 0)
        execute(command);
    else if (p > 0)
        wait(NULL);
    else {
        perror("error");
        exit(1);
    }


}


void run_back(char command[])
{

    pid_t p = fork();

    if (p < 0) {
        perror("error");
        exit(1);
    }
    else if (p == 0)
        execute(command);


}



int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    int x;
    History* history = NULL;

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        /*if (command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = '\0';
        }
        */
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        History* tmp;
        tmp = (History*)malloc(sizeof(History));
        /*if (tmp == NULL) {
            perror("error");
            exit(1);
        }*/
        tmp->next = NULL;

        char cmd[BUFFER_SIZE];
        strcpy(tmp->data, command);

        if (tmp->data == NULL) {
            perror("error");
            exit(1);
        }

        tmp->next = history;
        if (history == NULL)
            tmp->index = 1;
        else
            tmp->index = history->index + 1;
        history = tmp;

        x = check_type(command);
        switch (x) {

        case 0:
            run_front(command);
            break;

        case 1:
            run_back(command);
            break;

        case 2:
            print_history(history);
            break;

        default:
            perror("error");
            break;
        }

    }
    while (history)
    {
        History* tmp = history->next;
        free(history);
       history= tmp;
    }
    return 0;
}
