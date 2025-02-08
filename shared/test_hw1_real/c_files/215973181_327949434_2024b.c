#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define COMMAND_SIZE 64

struct Node {
    char data[BUFFER_SIZE];
    int index;
    struct Node* next;
};

struct Node* create_node(const char* content, const int ind) {
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if (!new_node) return NULL; // malloc failed
    strcpy(new_node->data, content);
    new_node->index = ind;
    return new_node;
}

void add_to_list(struct Node** list, const char* content) {
    struct Node* head = create_node(content, *list ? (*list)->index+1 : 1);
    if (!head) return; // create_node failed
    head->next = *list;
    *list = head;
}

void print_list(struct Node* head) {
    struct Node* current_node = head;
    while (current_node) {
        printf("%d %s", current_node->index, current_node->data);
        current_node = current_node->next;
    }
}

void delete_list(struct Node* node) {
    if (node == NULL) return; // reached end of list
    delete_list(node->next);
    free(node);
}

void exit_routine(struct Node* list) {
    perror("error");
    delete_list(list);
    exit(EXIT_FAILURE);
}

int main(void)
{

    close(2); // disables us from writing to STDERR
    dup(1); // duplicates STDOUT
    char buf[BUFFER_SIZE];
    char* argv[COMMAND_SIZE];
    struct Node* history_list = NULL; // head of linked list

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(buf, 0, BUFFER_SIZE);
        fgets(buf, BUFFER_SIZE, stdin);
        if(strncmp(buf, "exit", 4) == 0)
            break;

        add_to_list(&history_list, buf);

        if (strncmp(buf, "history", 7) == 0) {
            print_list(history_list);
            continue;
        }

        for (int i = 0; i < COMMAND_SIZE; i++) argv[i] = NULL;

        char *ptr = strtok(buf, " ");
        int index = 0;
        while (ptr) {
            argv[index] = ptr;
            index++;
            ptr = strtok(NULL, " ");
        }
        // get rid of '\n'
        for (int i = 0; argv[index-1][i] != '\0'; i++)
            if (argv[index-1][i] == '\n')
                argv[index-1][i] = '\0';

        pid_t pid = fork();
        if (pid < 0) {
            exit_routine(history_list);
        }
        else if (pid == 0) { // child
            if (strcmp(argv[index-1], "&") == 0)
                argv[index-1] = NULL;

            if (execvp(argv[0], argv) < 0)
                exit_routine(history_list);
        }
        else { // pid > 0, parent
            if (strcmp(argv[index - 1], "&") != 0)
                if (waitpid(pid, NULL, 0) == -1) // wait() is not sufficient b\c there may be background processes
                    exit_routine(history_list);
        }
    }

    delete_list(history_list); // if user ran the exit command
    return 0;
}
