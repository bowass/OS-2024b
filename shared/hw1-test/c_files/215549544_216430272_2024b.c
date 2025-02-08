#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define ARGV_SIZE 101
#define WHITESPACE " "
typedef struct node *Nodeptr;

typedef struct node{
    char* data;
    Nodeptr next;
} Node;

int pushToHistory(Nodeptr *proot, const char* command)
{
    Nodeptr pNewNode = (Nodeptr)malloc(sizeof(Node));
    if (pNewNode == NULL)
    {
        perror("error");
        return -1;
    }
    if ((pNewNode->data = (char*)malloc(strlen(command) + 1)) == NULL)
    {
        free (pNewNode); /* to avoid memory leak*/
        perror("error");
        return -1;
    }
    strcpy(pNewNode->data, command);

    pNewNode->next = *proot;
    *proot = pNewNode;
    return 0;
}

void deleteHistory(Nodeptr pRoot)
{
    while (pRoot)
    {
        Nodeptr pTemp = pRoot->next;
        free(pRoot->data);
        free(pRoot);
        pRoot = pTemp;
    }
}

/* count is the number of commands in history*/
void printHistory(const Nodeptr pRoot, int count )
{
    Nodeptr p;
    for(p = pRoot; p != NULL; p = p->next)
    {
        printf("%d %s\n", count, p->data);
        count --;
    }
}
int isEmpty(const char* command)
{
    int i;
    for(i= 0; i < strlen(command); i++)
    {
        if(command[i] != ' ')
            return 0;
    }
    return 1;
}
/*return true if the function is history, else return false*/
/*if we got here we assume command is not a null, return 1 if its history, 0 otherwise, but -1 for error...*/
int updateHistory(const char* command)
{
    static Nodeptr historyRoot = NULL;
    static int historyCounter = 0;

    if (strncmp(command, "exit", 4) == 0)
    {
        deleteHistory(historyRoot);
        return 2; /*the value is not matter in this case*/
    }
    // Strip '\n', if any
    char *crlf = strchr(command, '\n');
    if (crlf != NULL) {
        *crlf = 0;
    }
    if (isEmpty(command) == 1)
        return 2;
    if (pushToHistory(&historyRoot, command) == -1)
        return -1;
    historyCounter ++;

    /*now check if this is a history command */
    if(strncmp(command, "history", 7) == 0)
    {
        printHistory(historyRoot, historyCounter);
        return 1;
    }
    return 0;
}

int parseCommand( char command[], char **argv, int* argc)
{
    int i = 0;
    char* pToken = strtok(command, " ");
    while(pToken != NULL)
    {
        argv[i] = (char*)malloc(strlen(pToken) +1);
        if (argv[i] == NULL)
        {
            perror("error");
            *argc = i;
            return -1;
        }
        strcpy(argv[i], pToken);
        i++;
        pToken = strtok(NULL, " ");
    }
    *argc = i;
    /*the last place in the argv is a null, because we inited all the places before
    so the last place after the last place is null... */
	return 0;
}

void initArgv(char **argv)
{
    int i;
    for(i = 0; i < ARGV_SIZE; i++)
    {
        argv[i] = NULL;
    }
}


int main(void)
{
    int i;
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char* argv[ARGV_SIZE];
    int argc = 0;
    int command_flag;

	initArgv(argv);
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

		// Memory cleanup before parsing a new command
		for(i = 0; i < argc; i++)
        {
            if (argv[i] != NULL)
            {
                free(argv[i]);
				argv[i] = NULL;
            }
        }
        if(strncmp(command, "exit", 4) == 0)
        {
            updateHistory("exit");
            break;
        }
        command_flag = updateHistory(command);
        if(command_flag == -1)
        {
            perror("error");
			continue;
        }
        if (parseCommand(command, argv, &argc) != 0)
        {
            perror("error");
			continue;
        }
        // command is "history", so it's already been treated.
        // or argv is empty, so there's nothing to do.
        if(command_flag == 1 || argc == 0)
        {
            continue;
        }
		int flagRunInBackground = 0;
		if(strncmp(argv[argc -1], "&", 1) == 0)
        {
			// remove the "&" from argv as it is not an argument, but background exec flag
			free (argv[argc - 1]);
            argv[argc - 1] = NULL;
			flagRunInBackground = 1; //in background
		}

		pid_t pid;
		if( (pid = fork()) < 0)
		{
			perror("error");
		}
		else if (pid == 0) /* the child process*/
		{
			if(execvp(argv[0], argv) <0) {
				perror("error");
				exit(1);
			}
			exit(0);
        }
        else { 			/* the parent process */
			if (flagRunInBackground == 0)
            {
				/*foreground- we wait until the child process ends*/
                waitpid(pid, NULL, 0);
            }
        }
    }
    return 0;
}
