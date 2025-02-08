#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFFER_SIZE 100
#define MAX_LINE                80 
/* 80 chars per line, per command, should be enough. */
#define MAX_COMMANDS    100 /* size of history */

char history[MAX_COMMANDS][MAX_LINE];
char display_history [MAX_COMMANDS][MAX_LINE];

int command_count = 0;
char* delimiters = " \n\r\t";
char** parse_command(char* command, int* args_count);
void execute_command(char** args, int concurrent);
void free_parsed_command(char** args, int args_count);

void addtohistory(char inputBuffer[]) {
   int i = 0;
// add the command to history
   strcpy(history[command_count % MAX_COMMANDS], inputBuffer);
   while (inputBuffer[i] != '\n' && inputBuffer[i] != '\0') {
      display_history[command_count % MAX_COMMANDS][i] = inputBuffer[i];
      //printf("Haneie %c\n",inputBuffer[i]);
      i++;
   }
   display_history[command_count % MAX_COMMANDS][i] = '\0';
   ++command_count;
   return;
}


char** parse_command(char* command, int* args_count_out)
{
    char **args = NULL;
    int args_count = 0;
    

    char* token = strtok(command, delimiters);
    while (token != NULL)
    {
        args_count++;
        args = (char**)realloc(args, args_count * sizeof(char*));
        args[args_count - 1] = malloc(sizeof(char) * strlen(token));
        strcpy(args[args_count - 1], token);
        token = strtok (NULL, delimiters);
    }

    args = (char**)realloc(args, (args_count + 1) * sizeof(char*));
    args[args_count] = NULL;

    *args_count_out = args_count;
    return args;
}

void execute_command(char** args, int concurrent)
{
    pid_t pid, wpid;
    int status;

    pid = fork();

    if(pid == 0)
    { /* child process */
        if(execvp(args[0], args) == -1) 
        {
            perror("error");
            exit(EXIT_FAILURE);
        }
    }
    else if(pid > 0)
    { /* parent process */
        if(!concurrent)
        {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1)
            {
                perror("error");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void free_parsed_command(char** args, int args_count)
{
    for(int i = 0; i < args_count; i++)
    {
        if(args[i] != NULL)
        {
            free(args[i]);
        }
    }

    if(args != NULL)
    {
        free(args);
    }
}

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char copy_of_command[BUFFER_SIZE];
    int should_run = 1, command_number;
    
    while (1)
    {   
        fprintf(stdout, "my-shell> ");
        //fflush(stdout);
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

	    strcpy(copy_of_command, command);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
	    if (command[0] == '!'){

		    if (command[1] == '!'){
                if (command_count == 0) {
                    printf("No History\n");
                    continue;
                }
			    strcpy(command, history[(command_count - 1) % MAX_COMMANDS]);
			    strcpy(copy_of_command, history[(command_count - 1) % MAX_COMMANDS]);
		    }
		    else{ 
                if(isdigit(command[1])){ 
                    
                    command_number = atoi(&command[1]);

                    if (command_number > command_count)
                    {
                        printf("No History\n");
                        continue;
                    }
                    strcpy(command,history[command_number - 1]);
			        strcpy(copy_of_command,history[command_number - 1]); 
                }
            }
        }
        if (strncmp(command, "history", 7) == 0)
	    {
            addtohistory(command);
		    int i, upper = MAX_COMMANDS;
		    if (command_count < MAX_COMMANDS)
			    upper = command_count;
		    for (i = upper; i > 0; i--) {
			    printf("%d \t %s\n", i, display_history[i-1]);
		    }
		    continue;
	    }
        
        else {
    
            char** args;
            int args_count;
            args = parse_command(command, &args_count);

            int concurrent = 0;
            if (args_count > 1 && strcmp(args[args_count - 1], "&") == 0)
            {
                concurrent = 1;
                args[args_count - 1] = NULL;
            }

            // printf("args count: %d\n", args_count);
            // printf("concurrent: %d\n", concurrent);
            // for(int i = 0; i < args_count; i++)
            // {
            //     if(args[i] != NULL)
            //     {
            //         fprintf(stdout, "arg #%d: %s\n", i, args[i]);
            //     }
            // }

            if (*args != 0) {
                addtohistory(copy_of_command);
                execute_command(args, concurrent);
                free_parsed_command(args, args_count);
            }
        }
    }

    return 0;
}
