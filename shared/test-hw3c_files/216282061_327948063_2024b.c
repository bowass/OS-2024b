#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"
/*#define get_head(list) do {if (list != NULL) \
		pthread_mutex_lock(&list->mutex); \
	else { \
		head = NULL; \
		break; \
	} \
	head = list->head; \
	pthread_mutex_unlock(&list->mutex); \
	} while(0) \
*/

struct node {
	int value;
	pthread_mutex_t mutex; // each node will have a lock for hand over hand coding paradigm
	struct node *next;
};

struct list {
	struct node *head;
	pthread_mutex_t mutex; 
};

void print_node(node *node)
{	
	// DO NOT DELETE
	if (node != NULL)
	{
		printf("%d ", node->value);
	}
}

/*node *get_head (list* list)
{
	if (list != NULL)
		pthread_mutex_lock(&list->mutex);
	else 
		return NULL;
	node *head = list->head;
	pthread_mutex_unlock(&list->mutex);
	return head;
}*/

list* create_list()
{
	list* res = (list*)malloc(sizeof(list));
	if (res != NULL) {
		res->head = NULL;
		pthread_mutex_init(&res->mutex, NULL);
	}
	else 
		fprintf(stdout, "error: malloc failed for create_list\n");
	return res;
}

void delete_list(list* list)
{
	if(list)
		pthread_mutex_lock(&list->mutex);
	node *tmp = NULL, *curr = list->head;
	if(list)
		pthread_mutex_unlock(&list->mutex);
	
	if (curr != NULL)
		pthread_mutex_lock(&curr->mutex); //lock the node for safe use
	while (curr != NULL) {
		tmp = curr;
		curr = curr->next;
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
		
		pthread_mutex_unlock(&tmp->mutex);
		pthread_mutex_destroy(&tmp->mutex);
		free(tmp);
	}
	
	pthread_mutex_unlock(&list->mutex);
	pthread_mutex_destroy(&list->mutex);
	free(list);
}

void insert_value(list* list, int value)
{
	if(list)
		pthread_mutex_lock(&list->mutex);
	node *tmp = NULL, *curr = list->head, *new_node;
	if(list)
		pthread_mutex_unlock(&list->mutex);
	new_node = (node*)malloc(sizeof(node));
	if (new_node == NULL) {
		fprintf(stdout, "error: malloc failed for value: %d\n", value);
		return;
	}
	new_node->value = value;
	new_node->next = NULL;
	pthread_mutex_init(&new_node->mutex, NULL);
	
	if (curr != NULL) {
		pthread_mutex_lock(&curr->mutex);
		//if (curr->value != 0) {
			//printf("error at %d\n", value);
		//}
	}
	else {
		if (list != NULL) {
			pthread_mutex_lock(&list->mutex);
			list->head = new_node;
			pthread_mutex_unlock(&list->mutex);
		}
		return;
	}

	while (curr != NULL) {
		if (curr->value >= value)
			break;

		if (tmp != NULL)
			pthread_mutex_unlock(&tmp->mutex);
		tmp = curr;
		curr = curr->next;
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
	}
	//we want to add the new node between tmp and curr
	
	if (curr != NULL && tmp!= NULL) { // in case in the middle of the list
		tmp->next = new_node;
		new_node->next = curr;
		
		pthread_mutex_unlock(&tmp->mutex);
		pthread_mutex_unlock(&curr->mutex);
	}
	else if (curr == NULL) { // in case in the end of the list
		tmp->next = new_node;
		pthread_mutex_unlock(&tmp->mutex);
	}
	else { // in case in the head of the list, tmp=NULL
		new_node->next = curr;
		
		if (list != NULL)
			pthread_mutex_lock(&list->mutex);
		else
			return;
		list->head = new_node;
		pthread_mutex_unlock(&list->mutex);
		pthread_mutex_unlock(&curr->mutex);
	}
}

void remove_value(list* list, int value)
{
	if(list)
		pthread_mutex_lock(&list->mutex);
	node *tmp = NULL, *curr = list->head, *new_node;
	if(list)
		pthread_mutex_unlock(&list->mutex);
	
	if (curr != NULL)
		pthread_mutex_lock(&curr->mutex);

	while (curr != NULL) {
		if (curr->value == value)
			break;

		if (tmp != NULL)
			pthread_mutex_unlock(&tmp->mutex);
		tmp = curr;
		curr = curr->next;
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
	}
	// we want to remove curr 
	if (tmp != NULL && curr != NULL) {
		tmp->next = curr->next;
		pthread_mutex_unlock(&tmp->mutex);
	}
	else if (tmp == NULL) { // in case we want to remove the first
		if (list != NULL)
			pthread_mutex_lock(&list->mutex);
		else
			return;
		list->head = curr->next;
		pthread_mutex_unlock(&list->mutex);
	}
	
	pthread_mutex_unlock(&curr->mutex);
	pthread_mutex_destroy(&curr->mutex);
	free(curr);
}

void print_list(list* list)
{
	if(list)
		pthread_mutex_lock(&list->mutex);
	node *tmp = NULL, *curr = list->head, *new_node;
	if(list)
		pthread_mutex_unlock(&list->mutex);
	
	if (curr != NULL)
		pthread_mutex_lock(&curr->mutex); //lock the node for safe use

	while (curr != NULL) {
		print_node(curr);
		if (tmp != NULL)
			pthread_mutex_unlock(&tmp->mutex);
		tmp = curr;
		curr = curr->next;
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
		
	}
	if (tmp != NULL)
		pthread_mutex_unlock(&tmp->mutex);
	
	printf("\n"); // DO NOT DELETE
	
	
	
	
	
	
	/*node *curr = get_head(list), *tmp;
		
	if (curr != NULL)
		pthread_mutex_lock(&curr->mutex); //lock the node for safe use

	while (curr != NULL) {
		print_node(curr);
		tmp = curr;
		curr = curr->next;
		pthread_mutex_unlock(&tmp->mutex); // unlock the node and lock the following lock
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
		
	}
	printf("\n"); // DO NOT DELETE*/
}

void count_list(list* list, int (*predicate)(int))
{
	int count = 0; // DO NOT DELETE
	if(list)
		pthread_mutex_lock(&list->mutex);
	node *tmp = NULL, *curr = list->head, *new_node;
	if(list)
		pthread_mutex_unlock(&list->mutex);
	
	if (curr != NULL)
		pthread_mutex_lock(&curr->mutex); //lock the node for safe use

	while (curr != NULL) {
		if (predicate(curr->value))
			count++;
		tmp = curr;
		curr = curr->next;
		if (tmp != NULL)
			pthread_mutex_unlock(&tmp->mutex); // unlock the node and lock the following lock
		if (curr != NULL)
			pthread_mutex_lock(&curr->mutex);
		
	}

	printf("%d items were counted\n", count); // DO NOT DELETE
}
