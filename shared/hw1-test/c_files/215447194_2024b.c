
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100

typedef struct historyCmd{ //history stack struct
    char* str;
    struct historyCmd* pred;
    int cnt;
} historyCmd;

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];

    historyCmd* last = NULL; //init history

    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        //the input string has \n so I ignore it (had a few problems with the print)
        int len = strlen(command);
        if(len>0 && command[len-1]=='\n')
            command[len-1] = '\0';

        //command isn't exit, save history
        historyCmd* ptr = (historyCmd*)malloc(sizeof(historyCmd));
        ptr->str = (char*) malloc(BUFFER_SIZE * sizeof(char));
        strcpy(ptr->str,command);
        if(last==NULL) //history empty
            ptr->cnt = 1;
        else
            ptr->cnt = last->cnt+1;
        
        //push history
        ptr->pred = last;
        last = ptr;
        
        if(strncmp(command, "history",7)==0){ //execute command history
            historyCmd* ptr = last;
            while(ptr!=NULL){
                printf("%d %s\n",ptr->cnt, ptr->str);
                ptr = ptr->pred;
            }
        }
        else{ //execute command which isnt history
            char* argv[10];
            char* pch;
            pch = strtok(command, " ");
            char* cmd = pch;
            int i = 0;
            while(pch!=NULL){
                argv[i++]=pch;
                pch = strtok(NULL, " ");
            }
            int background; //flag: 0: foreground, 1: background
            if (strncmp(argv[i-1],"&",1)==0 && i>0){ //if the last char is '&' we want to ignore it as an argument
                argv[i-1] = NULL; //ignore it
                background = 1; //if the last char is '&' change to 1
            }
            else{
                argv[i] = NULL; //there isnt '&' so all the arguments we got are fine
                background = 0; //there isnt '&' it to 0
            }
            argv[i] = NULL;
            pid_t pid;
            if ((pid = fork()) < 0) {     
                perror("ERROR: forking child process failed\n");
            }
            else if(pid == 0){ //child
                if(execvp(cmd,argv)<0){
                    perror("ERROR: exec failed\n");
                }
            }
            else{ //parent
                if(background == 0){ //run foreground
                    if(waitpid(pid, NULL, 0) < 0){ //we want to wait for the specific last child which is running foreground
                        perror("ERROR: waitpid failed\n");
                    }
                }
            
            }
        }
            
    }
    //free history
    while(last!= NULL){
        historyCmd* ptr = last;
        last = last->pred;
        free(ptr->str);
        free(ptr);
    }
    return 0;
}
