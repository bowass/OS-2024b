#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "concurrent_list.h"

#define CMD_BUFFER_SIZE 100
#define MAX_THREAD_COUNT 252

char* delimiters = " \n\r\t";
char* string_delimiter = "\"";
char command[CMD_BUFFER_SIZE];
char parsed_command[CMD_BUFFER_SIZE];
pthread_t threads[MAX_THREAD_COUNT];
int thread_count = 0;
list* mylist = NULL;

/*********** Halt Test ***********/
pthread_cond_t cond_halt;
pthread_mutex_t mutex_halt;

pthread_cond_t cond_access;
pthread_mutex_t mutex_access;

int halted = 0;
int accessed = 0;
/*********************************/

void* delete_list_task(void* arg)
{
	delete_list(mylist);
	mylist = NULL;
	return 0;
}

void* print_list_task(void* arg)
{
	print_list(mylist);
	return 0;
}

void* insert_value_task(void* arg)
{
	int value = (int)arg;
	insert_value(mylist, value);
	return 0;
}

void* remove_value_task(void* arg)
{
	int value = (int)arg;
	remove_value(mylist, value);
	return 0;
}

void* count_greater_task(void* arg)
{
	int threshold = (int)arg;
	int pred(int value)
	{
		return value > threshold;
	}	
	count_list(mylist, pred);
	return 0;
}

void* halt_task(void* arg)
{
	int threshold = (int)arg;
	int pred(int value)
	{
		if(value == threshold)
		{
			pthread_mutex_lock(&mutex_halt);
			halted = 1;
			pthread_cond_signal(&cond_halt);
			while(halted)
			{
				pthread_cond_wait(&cond_halt, &mutex_halt);
			}
			
			pthread_mutex_unlock(&mutex_halt);
			return 1;
		}

		return 0;
	}	
	count_list(mylist, pred);
	return 0;
}

void* check_access(void* arg)
{
	int threshold = (int)arg;
	int pred(int value)
	{
		if(value == threshold)
		{
			pthread_mutex_lock(&mutex_access);
			accessed = 1;
			pthread_cond_signal(&cond_access);
			pthread_mutex_unlock(&mutex_access);
			return 1;
		}

		return 0;
	}	
	count_list(mylist, pred);
	return 0;
}

void parse_command(char* command, char* command_out, int* arg)
{
    int token_num = 0;
    char* token = strtok(command, delimiters);
    while (token != NULL)
    {
    	if(token_num == 0)
    	{
			strcpy(command_out, token);   		
    	}
    	else
    	{
			*arg = atoi(token);		
    	}

        token = strtok (NULL, delimiters);
        token_num++;
    }
}

int execute_command(char* command, int value)
{
	if(strcmp(command, "create_list") == 0)
	{
		mylist = create_list();
	}
	else if(strcmp(command, "delete_list") == 0)
	{
		pthread_create(&threads[thread_count], NULL, delete_list_task, NULL);
		thread_count++;
	}
	else if(strcmp(command, "print_list") == 0)
	{
		pthread_create(&threads[thread_count], NULL, print_list_task, NULL);
		thread_count++;
	}		
	else if(strcmp(command, "insert_value") == 0)
	{	
		pthread_create(&threads[thread_count], NULL, insert_value_task, (void*)value);
		thread_count++;
	}
	else if(strcmp(command, "remove_value") == 0)
	{
		pthread_create(&threads[thread_count], NULL, remove_value_task, (void*)value);
		thread_count++;
	}		
	else if(strcmp(command, "count_greater") == 0)
	{
		pthread_create(&threads[thread_count], NULL, count_greater_task, (void*)value);
		thread_count++;
	}
	else if(strcmp(command, "join") == 0)
	{
		for(int i = 0; i < thread_count; i++)
		{
			pthread_join(threads[i], NULL);
		}
		thread_count = 0;
	}
	/*********** Halt Test ***********/
	else if(strcmp(command, "halt") == 0)
	{
		pthread_create(&threads[thread_count], NULL, halt_task, (void*)value);
		thread_count++;
	}
	else if(strcmp(command, "cancel_halt") == 0)
	{
		pthread_mutex_lock(&mutex_halt);
		halted = 0;
		pthread_cond_signal(&cond_halt);
		pthread_mutex_unlock(&mutex_halt);
	}
	else if(strcmp(command, "ensure_halt") == 0)
	{
		pthread_mutex_lock(&mutex_halt);
		while(!halted)
		{
			pthread_cond_wait(&cond_halt, &mutex_halt);	
		}
		pthread_mutex_unlock(&mutex_halt);
	}
	/*********** Access Test ***********/
	else if(strcmp(command, "check_access") == 0)
	{
		pthread_create(&threads[thread_count], NULL, check_access, (void*)value);
		thread_count++;
	}
	else if(strcmp(command, "ensure_access") == 0)
	{
		pthread_mutex_lock(&mutex_access);
		while(!accessed)
		{
			pthread_cond_wait(&cond_access, &mutex_access);	
		}
		pthread_mutex_unlock(&mutex_access);
	}		
	/*********************************/	
	else
	{
		printf("unknown command\n");
	}				

	return 0;
}

int main(int argc, const char** argv)
{
	/*********** Halt Test ***********/
	pthread_cond_init(&cond_halt, NULL);
	pthread_mutex_init(&mutex_halt, NULL);
	/*********** Access Test ***********/
	pthread_cond_init(&cond_access, NULL);
	pthread_mutex_init(&mutex_access, NULL);	
	/*********************************/	

    while (1)
    {
        memset(command, 0, CMD_BUFFER_SIZE);
        memset(parsed_command, 0, CMD_BUFFER_SIZE);        
        fgets(command, CMD_BUFFER_SIZE, stdin);
        if(strncmp(command, "exit", 4) == 0)
        {
            break;
        }

        int value;
        parse_command(command, parsed_command, &value);
        execute_command(parsed_command, value);
    }

	/*********** Halt Test ***********/
	pthread_cond_destroy(&cond_halt);
	pthread_mutex_destroy(&mutex_halt);
	/*********** Access Test ***********/
	pthread_cond_destroy(&cond_access);
	pthread_mutex_destroy(&mutex_access);	
	/*********************************/	

    return 0;
}
