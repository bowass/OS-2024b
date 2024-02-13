#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

// history as a linked list
int command_count = 0;
typedef struct node {
	char* s;
	struct node* next;
} History;

void add_to_history(History** h, char* s) {
	// adds a new node at the beginning of History
	History* newNode = (History*) malloc(sizeof(History));
	newNode->s = (char*) malloc(sizeof(char)*strlen(s));
	strcpy(newNode->s, s);
	newNode->next = *h;
	*h = newNode;
	command_count++;
}

void print_history(History* h) {
	// prints in LIFO order
	History* tmp = h;
	int i = command_count;
	while (tmp != NULL) {
		fprintf(stdout, "%d %s\n", i--, tmp->s);
		tmp = tmp->next;
	}
}

void free_history(History** h) {
	// frees entier list
	if (*h == NULL) return;
	free_history(&(*h)->next);
	free((*h)->s);
	free(*h);
}

// get #tokens
// returns in front has no &
int getNumArgs(char* s, int* front) {
	int spaces = 0;
	char* tmp = s;
	*front = 1;
	// get #spaces + 1
	while ((tmp = strchr(tmp, ' ')) != NULL) {
		spaces++; tmp++;
		// catch background: & as last argumnet
		if (strchr(tmp, ' ') == NULL && 
			strcmp(tmp, "&") == 0)
			*front = 0;
	}
	// if in front #tokens = #spaces + 1
	// else #actual cmd tokens = #spaces (including " &" space)
	return spaces + *front;
}

int main(void) {
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char *ptr;
    char** argv;
    pid_t pid;
	int front, numArgs, i;
	History* history = NULL;

    while (1) {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        
        if(strncmp(command, "exit", 4) == 0)
		break;
        
	// remove trailing '\n'
	command[strcspn(command, "\n")] = '\0';
		
	// get #arguments and front/background
	numArgs = getNumArgs(command, &front);

        // save command & push to history
        argv = (char**) malloc(sizeof(char*)*(numArgs+1));
        add_to_history(&history, command);
        
        // parse command & update argv
	ptr = strtok(command, " ");
        if (ptr != NULL) strcpy(command, ptr);
		
        for (int i = 0; i < numArgs; i++) {
        	argv[i] = ptr;
        	ptr = strtok(NULL, " ");
	}
        argv[numArgs] = NULL;

    	// history command
	if (strncmp(argv[0], "history", 7) == 0)
	        print_history(history);
	    // non-history -> fork
        else if ((pid = fork()) < 0) {
		perror("error");
		free(argv);
		free_history(&history);
		exit(1);
	}
        // child process
        else if (pid == 0) {
	       	argv[numArgs-1][strcspn(argv[numArgs-1], "&")] = 0;
		if (execvp(command, argv) < 0) {
			perror("error");
			free(argv);
			free_history(&history);
			exit(1);
		}
		break;
        }
        // parent process
        else if (front) waitpid(pid, NULL, 0);
        free(argv);
    }
    
    free(argv);
    free_history(&history);

    return 0;
}

