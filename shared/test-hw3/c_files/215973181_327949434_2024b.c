#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
	int value;
	node* next;
	node* prev;
	pthread_mutex_t mutex;
};

struct list {
	node* head;
	pthread_mutex_t mutex;
};

inline void lock(node* N) {
	if (N) pthread_mutex_lock(&N->mutex);
}

inline void unlock(node* N) {
	if (N) pthread_mutex_unlock(&N->mutex);
}

node* create_node(int value) {
	node* new_node = (node*) malloc(sizeof(node));
	new_node->value = value;
	new_node->prev = NULL;
	new_node->next = NULL;
	pthread_mutex_init(&new_node->mutex, NULL);
	return new_node;
}

list* create_list()
{
	list* new_list = (list*) malloc(sizeof(list));
	new_list->head = NULL;
	pthread_mutex_init(&new_list->mutex, NULL);
	return new_list;
}

void insert_value(list* cont, int value)
{
	if (!cont) return;

	pthread_mutex_lock(&(cont->mutex));
	lock(cont->head);
	node* N = cont->head;

	if (!N) { // list is empty
		node* curr_node = create_node(value);
		// no check for !curr_node bc if malloc failed, nothing would change
		cont->head = curr_node;
		pthread_mutex_unlock(&(cont->mutex));

		return;
	}

	if (N->value > value) { // inserting min val
		node* curr_node = create_node(value);

		cont->head = curr_node;
		pthread_mutex_unlock(&(cont->mutex));

		N->prev = curr_node;
		curr_node->next = N;
		unlock(N);

		return;
	}


	node* prev_node = N;

	while (prev_node) {
		if (prev_node->next && prev_node->next->value < value) {
			lock(prev_node->next);
			node* temp = prev_node->next;
			if (prev_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
			unlock(prev_node);
			prev_node = temp;
		}
		else break;
	}

	lock(prev_node->next);
	node* next_node = prev_node->next;

	node* curr_node = create_node(value);

	pthread_mutex_lock(&curr_node->mutex);

	if (prev_node) prev_node->next = curr_node;
	curr_node->prev = prev_node;
	curr_node->next = next_node;
	if (next_node) next_node->prev = curr_node;

	unlock(prev_node);
	if (prev_node == cont->head) pthread_mutex_unlock(&(cont->mutex));

	pthread_mutex_unlock(&curr_node->mutex);
	
	unlock(next_node);
}

void remove_node(node* del_node)
{
	lock(del_node->next);

	node *prev_node = del_node->prev, *next_node = del_node->next;

	if (prev_node) prev_node->next = next_node;
	if (next_node) next_node->prev = prev_node;

	pthread_mutex_unlock(&del_node->mutex);
	pthread_mutex_destroy(&del_node->mutex);
	free(del_node);

	unlock(prev_node);
	unlock(next_node);
}

void delete_node(node* del_node)
{
	pthread_mutex_unlock(&del_node->mutex);
	pthread_mutex_destroy(&del_node->mutex);
	free(del_node);
}

void remove_value(list* cont, int value)
{
	if (!cont) return;

	pthread_mutex_lock(&(cont->mutex));
	lock(cont->head);
	node* N = cont->head;

	if (!N) {
		pthread_mutex_unlock(&(cont->mutex));

		return;
	}

	if (N->value == value) {
		lock(N->next);
		cont->head = N->next;
		if (cont->head) cont->head->prev = NULL;
		delete_node(N);
		pthread_mutex_unlock(&(cont->mutex));
		unlock(cont->head);

		return;
	}

    	node* prev_node = N;

	while (prev_node) {
		if (prev_node->next && prev_node->next->value < value) {
			lock(prev_node->next);
			node* temp = prev_node->next;
			if (prev_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
			unlock(prev_node);
			prev_node = temp;
		}
		else break;
	}

	lock(prev_node->next);

	if (!(prev_node->next) || prev_node->next->value != value) { // didn't find a node with the requested value
		if (prev_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
		unlock(prev_node);
		unlock(prev_node->next);
		return;
	}

	node* curr_node = prev_node->next;
	remove_node(curr_node);
	if (prev_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
}

void delete_list(list* cont)
{
	if (!cont) return;

	pthread_mutex_lock(&(cont->mutex));
	lock(cont->head);
	node* curr_node = cont->head;

	while (curr_node) {
		lock(curr_node->next);
		node* next_node = curr_node->next;

		delete_node(curr_node);
		curr_node = next_node;
	}

	cont->head = NULL;
	pthread_mutex_unlock(&(cont->mutex));
	pthread_mutex_destroy(&cont->mutex);
	free(cont);
}

void print_node(node* curr_node)
{
	if (curr_node)
		printf("%d ", curr_node->value);
}

void print_list(list* cont)
{
	if (!cont) {
		printf("\n");
		return;
	}

	pthread_mutex_lock(&(cont->mutex));
	lock(cont->head);
	node* curr_node = cont->head;

	if (!curr_node) {
		pthread_mutex_unlock(&(cont->mutex));
		printf("\n");

		return;
	}

	while (curr_node) {
		print_node(curr_node);

		lock(curr_node->next);
		node* next_node = curr_node->next;
		if (curr_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
		unlock(curr_node);
		curr_node = next_node;
	}

	unlock(curr_node);

	printf("\n");
}

void count_list(list* cont, int (*predicate)(int))
{
	int count = 0;

	if (!cont) {
		printf("%d items were counted\n", count);
		return;
	}

	pthread_mutex_lock(&(cont->mutex));
	lock(cont->head);
	node* curr_node = cont->head;

	if (!curr_node) {
		pthread_mutex_unlock(&(cont->mutex));
		printf("%d items were counted\n", count);

		return;
	}

	while (curr_node) {
		if (predicate(curr_node->value))
			++count;

		lock(curr_node->next);
		node* next_node = curr_node->next;
		if (curr_node == cont->head) pthread_mutex_unlock(&(cont->mutex));
		unlock(curr_node);
		curr_node = next_node;
	}

	unlock(curr_node);

	printf("%d items were counted\n", count);
}
