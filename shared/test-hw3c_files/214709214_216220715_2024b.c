#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <linux/kernel.h> 
#include "concurrent_list.h"

struct node {
	int value;
	node* next;
	node* prev;
	pthread_mutex_t m;
	// add more fields
};

struct list {
	node* head;
	// add fields
};

void print_node(node* node)
{
	// DO NOT DELETE
	if (node)
	{
		printf("%d ", node->value);
	}
}

list* create_list()
{
	// add code here
	list* myList = (list*)malloc(sizeof(list));
	myList->head = NULL;
	return myList; // REPLACE WITH YOUR OWN CODE
}

void delete_list(list* list)
{
	node* current = list->head;
	node* next=NULL;
	while (current != NULL)
	{
		next = current->next;
		pthread_mutex_destroy(&current->m);
		free(current);
		current = next;
	}
	free(list);
}

void insert_value(list* list, int value)
{
	node* newNode = (node*)calloc(1,sizeof(node));
	newNode->value = value;
	newNode->next = NULL;
	newNode->prev = NULL;
	pthread_mutex_init(&(newNode->m), NULL);
	node* tmp=NULL;
	//if we want to insert at the benninging
	if (list->head == NULL)
	{
		pthread_mutex_lock(&newNode->m);
		list->head = newNode;
		pthread_mutex_unlock(&newNode->m);
		return;
	}
	if (list->head->value > value)
	{
		pthread_mutex_lock(&list->head->m);
		pthread_mutex_lock(&newNode->m);
		newNode->next = list->head;
		newNode->prev = NULL;
		list->head->prev = newNode;
		list->head = newNode;
		pthread_mutex_unlock(&list->head->m);
		pthread_mutex_unlock(&newNode->next->m);
		return;
	}

	node* current = list->head;

	while (current->next != NULL && value > current->next->value)
	{
		current = current->next;
	}
	if (current->next == NULL)
	{
		pthread_mutex_lock(&newNode->m);
		pthread_mutex_lock(&current->m);
		current->next = newNode;
		newNode->prev = current;
		newNode->next = NULL;
		pthread_mutex_unlock(&newNode->m);
		pthread_mutex_unlock(&current->m);
		return;
	}
	else
	{
		pthread_mutex_lock(&newNode->m);
		pthread_mutex_lock(&current->m);
		pthread_mutex_lock(&current->next->m);
		tmp = current->next;
		current->next = newNode;
		newNode->next = tmp;
		newNode->prev = current;
		tmp->prev = newNode;
		pthread_mutex_unlock(&newNode->m);
		pthread_mutex_unlock(&current->m);
		pthread_mutex_unlock(&tmp->m);
	}
	return;
}

void remove_value(list* list, int value)
{
	node* current = list->head;
	node* tmp=NULL;
	pthread_mutex_t* mutex;
	
	if (list->head == NULL)
		return;
	
	while (current != NULL)
	{
		if (current->value == value)
		{
			if (current == list->head)//remove from the benninging
			{
				
				mutex = &current->m;
				pthread_mutex_lock(mutex);

				if (current->next != NULL) {
					pthread_mutex_lock(&current->next->m);
					list->head = current->next;
					current->next->prev = NULL;
					pthread_mutex_unlock(&current->next->m);
				}
				else
				{
					list->head = NULL;
				}				

				pthread_mutex_unlock(mutex);
				pthread_mutex_destroy(mutex);
				
				free(current);				
			}
			
			else if (current->next != NULL)
			{
				pthread_mutex_lock(&current->m);
				pthread_mutex_lock(&current->prev->m);
				pthread_mutex_lock(&current->next->m);

				current->next->prev = current->prev;
				current->prev->next = current->next;

				pthread_mutex_unlock(&current->m);
				pthread_mutex_destroy(&current->m);
				pthread_mutex_unlock(&current->prev->m);
				pthread_mutex_unlock(&current->next->m);
				free(current);
			}
			else if (current->prev != NULL)
			{
				pthread_mutex_lock(&current->m);
				pthread_mutex_lock(&current->prev->m);
				
				current->prev->next = NULL;

				pthread_mutex_unlock(&current->m);
				pthread_mutex_destroy(&current->m);
				pthread_mutex_unlock(&current->prev->m);
	
				free(current);
			}
			return;
		}
		current = current->next;
	}
	return;
}

void print_list(list* list)
{
	node* current = list->head;
	if (list->head != NULL)
	{ 
		pthread_mutex_lock(&current->m);
		while (current->next != NULL) {
			printf("%d ", current->value);
			pthread_mutex_lock(&current->next->m);
			pthread_mutex_unlock(&current->m);
			current = current->next;
		}
		printf("%d ", current->value);
		pthread_mutex_unlock(&current->m);
	}
	printf("\n"); // DO NOT DELETE
	return;
}

void count_list(list* list, int (*predicate)(int))
{
	int count = 0; // DO NOT DELETE
	node* current = list->head;
	if (list->head != NULL)
	{
		pthread_mutex_lock(&current->m);
		while (current->next != NULL) {
			count += predicate(current->value);
			pthread_mutex_lock(&current->next->m);
			pthread_mutex_unlock(&current->m);
			current = current->next;
		}
		count += predicate(current->value);
		pthread_mutex_unlock(&current->m);
	}
	printf("%d items were counted\n", count); // DO NOT DELETE
	return;
}
