#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define BUFFER_SIZE 100

// implement history using a stack
typedef struct history_node {
	struct history_node *next;
	char *command;
	size_t index;
} history_node;

void free_history(history_node *history_stack);
history_node* update_history(history_node *history_stack, char *command);
void exec_history(history_node *history_stack);

void get_info(char command[BUFFER_SIZE], int *argc, bool *background);
char** get_argv(char command[BUFFER_SIZE], int argc);
void run(char command[BUFFER_SIZE], char **argv, bool background, history_node *history_stack);

int main(void)
{	
    close(2);
    dup(1);
    char command[BUFFER_SIZE]; 

	history_node *history_stack = NULL, *tmp;

    while (1)
    {
		int argc;
		bool background;
		
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
		
		tmp = update_history(history_stack, command); 
		if(tmp == NULL){
			perror("error");
			continue;
		}
		history_stack = tmp;
		
		get_info(command, &argc, &background);
		char **argv = get_argv(command, argc);
		run(command, argv, background, history_stack);
		free(argv);
    }
	
	free_history(history_stack);
	
    return 0;
}

void free_history(history_node *history_stack)
{
	history_node *curr = history_stack, *next;
	while (curr) {
		next = curr->next;
		free(curr->command);
		free(curr);
		curr = next;
	}
}

history_node* update_history(history_node *history_stack, char *command)
{
	history_node *new_head = (history_node*)malloc(sizeof(history_node));
	if (new_head){
		new_head->command = (char*)malloc(strlen(command));
		if (!new_head->command)
			return 0;
		strncpy(new_head->command, command, BUFFER_SIZE);
		new_head->next = history_stack;
		if (history_stack)
			new_head->index = history_stack->index + 1;
		else
			new_head->index = 1;
	}
	return new_head;
}

void exec_history(history_node *history_stack)
{
	history_node *curr = history_stack;
	while (curr) {
		printf("%zu %s", curr->index, curr->command);
		curr = curr->next;
	}
}

void get_info(char command[BUFFER_SIZE], int *argc, bool *background)
{
	// original:   commmand  arg1  arg2 arg3 ... arg_argc  \n
	// new:command arg1 arg2 arg3 ... arg_argc
	// therfore: # whitespace sequences after command = argc + 1 (because of '\n' at the end) = len(argv) - 1 (becuase we add NULL to end of argv)
	// if arg_argc == '&', subtruct 1 from the above
	char new_command[BUFFER_SIZE];
	int new_i, i;
	*argc = 0;
	
	for (i = 0; isspace(command[i]); i++) // skip whitespace prefix
		;
		
	for (new_i = 0; command[i] != '\0'; i++) {
		if (!isspace(command[i]))
			new_command[new_i++] = command[i];
		else if (new_i == 0 || new_command[new_i - 1] != ' ') { // beginning of whitespace sequence 
			new_command[new_i++] = ' ';
			(*argc)++;
		}
	}
	(*argc)--;
	new_i--;
	
	if (*argc == -1) { // command is a single sequence of whitespaces
		*background = false;
		strcpy(command, "");
		return;
	}
	if (*argc == 0 && new_command[0] == '&') { // command is equeivelent to &
		*background = true;
		*argc = -1;
		strcpy(command, "");
		return;
	}
	
	// last char is always '\n' (which becomes ' ') so we can swap that, the char before that indicates backgroundness (english sucks, C is better)
	*background = new_command[new_i - 1] == '&' && new_command[new_i - 2] == ' ';
	if (*background) {
		new_command[new_i - 2] = '\0';
		(*argc)--;
	}
	else 
		new_command[new_i] = '\0';
	strcpy(command, new_command);
}

char** get_argv(char command[BUFFER_SIZE], int argc)
{	
	int index = 0;
	char **argv = (char**)malloc(sizeof(char*) * (argc + 2));
	char *curr = strtok(command, " ");
	argv[index++] = curr;
	
	while ((curr = strtok(NULL, " "))) {
		argv[index++] = curr;
	}
	argv[index] = NULL;

	return argv;
}

void run(char command[BUFFER_SIZE], char **argv, bool background, history_node *history_stack)
{
	if (strncmp(command, "history", 7) == 0) { // by defenition, history never runs in the background
		exec_history(history_stack);
		return;
	}
	
	int res = 0;
	pid_t p = fork(); // always fork to not stop execution of shell
	if (p < 0){
		perror("error");
		return;
	}
	if (p == 0) { // child process
		if (strcmp(command, "") != 0) {
			res = execvp(command, argv);
			perror("error");
			exit(1);
		}
		exit(0);
	}
	
	else if (!background) // parent
		waitpid(p, &res, 0);	
}
