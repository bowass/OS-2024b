#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

char *strdup(const char *c);

typedef char* T;
typedef struct Stack {
    T *array;
    unsigned size;
    unsigned allocation;
} Stack, *pStack;

//this are the only functions we need
bool init(pStack stack);
void destroy(pStack stack);
bool push(pStack stack, T data);
T* top(Stack stack);
bool _Set_size(pStack stack, unsigned size);//impl detail

int wordcount (char *string);

//for debugging if needed
void DBG_print(char * format, ...);

#define BUFFER_SIZE 100

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    bool needToFree = false;//for the child only
    int argc = 0; //number of elements
    char** argv; //if we'll use static memory it will be bounded by char*[51] and I don't want it.
    int i = 0; //general porpuse temporary
    char * p; 
    Stack s;//ths stack for the history
    init(&s);
    while (1)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        command[strcspn(command,"\n")]='\0';//fgets put \n in the end of the input
        push(&s,strdup(command));
        
        int words = wordcount(command);
        argv = (char**)malloc(sizeof(char*)*(words+1));
        if(!argv)
            exit(1);

        
        //parsing the line around spaces
        i = 0;
        p = strtok(command," ");
        while (p != NULL)
        {
            argv[i++]=p;
            p = strtok (NULL, " ");
        }
        argv[i]=NULL;
        argc = i;
        if(argc==0) continue; // if the line is empty
        
        //here there can be a very interesting data race
        //because argv points to command if there is & in the command
        //and the main thread keep running and gets new user input 
        //before this thread calls the execvp than the command change and so
        //argv will become trash(the new user input) so we need to copy command
        //for the argv for that case
        //soulution 1: each command no matter what we'll make that backup but it's expensive for the reguler commands and they are the most entered commands(ui shell).
        //soulution 2: we'll copy the argv only if needed so most of the times(in our case of the shell) it will be only one not taken branch.
        if(*argv[argc-1]=='&' && *(argv[argc-1]+1)=='\0')
        {
            int diff = argv[0] - command;
            char* buffer = (char*)malloc(BUFFER_SIZE-diff); //we malloc one buffer for the whole parameters but don't need the spaces before the first argument
            memcpy(buffer,command,BUFFER_SIZE-diff);
            needToFree = true;
            for(i = 0; i < argc - 1 ; i++)
            {
                argv[i] = argv[i] - command + buffer - diff; // we move the argv to the new buffer by taking their offset from command
            }
            argc--;
            argv[argc] = NULL;
            }

        pid_t childPid;
        switch( childPid = fork() ) {
        case 0://child
            

            if(strncmp(argv[0], "history", 7) == 0)
            {

                i = top(s) - s.array + 1;
                for(char** pp = top(s); pp>=s.array ; pp--) // the repeatung function is horrible but it can be chached easily by the compiler.
                {
                    printf("%d %s\n",i--,*pp);
                }
            }
            else {
                if (execvp(argv[0],argv) == -1)
                {
                    perror("error");
                }
            }


            if(needToFree)
            {
                free(argv[0]);
            }
            free(argv);

            return 0;
            break;
        case -1:
            perror("error");
            break;
        default:
            if(!needToFree)
            {
                int returnStatus;    
                waitpid(childPid, &returnStatus, 0);
                if (returnStatus == 1)      
                {
                    perror("error");//this is only for the cases where the execvp'ed process faild
                }
                
            }
            free(argv);
        }
    }
    destroy(&s);
    return 0;
}

#define MIN_ALLOC 1

bool _Set_size(pStack stack, unsigned size) {
    stack->size = size;

    while(size > stack->allocation)
        stack->allocation *= 2;
    while(size <= stack->allocation/4 && stack->allocation/2 >= MIN_ALLOC)
        stack->allocation /= 2;

    T* temp = realloc(stack->array, stack->allocation * sizeof(T));
    if(!temp) return false;
    stack->array = temp;
    return true;
}

bool init(pStack stack) {
    stack->allocation = MIN_ALLOC;
    stack->array = malloc(stack->allocation * sizeof(T));
    stack->size = 0;
    return stack->array!=NULL;
}

void destroy(pStack stack) {
    free(stack->array);
    stack->array = NULL;
    stack->allocation = 0;
    stack->size = 0;
}

bool push(pStack stack, T data) {
    if(!_Set_size(stack, stack->size+1)) return false;
    stack->array[stack->size-1] = data;
    return true;
}

T* top(Stack stack){
    return stack.array + stack.size - 1;
}

int wordcount (char *string){

    int n = 0; 

    char *p = string ;
    int flag = 0 ;

    while(isspace(*p)) p++;


    while(*p){
        if(!isspace(*p)){
            if(flag == 0){
                flag = 1 ;
                n++;
            }
        }
        else flag = 0;
        p++;
    }

    return n ;
}

void DBG_print(char * format, ...)
{
#ifdef MDEBUG
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    printf("%s", buffer);
    va_end(args);
#endif
}

