#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  struct node* next;
  pthread_mutex_t lock;
 
};

struct list {
    node* head;
    pthread_mutex_t lock;
};

void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}


list* create_list(){
    list* new_list = (list*)malloc(sizeof(list));
    pthread_mutex_init(&(new_list->lock), NULL);
    new_list->head = NULL;


    return new_list;

}

void delete_list(list* list)
{
    if (list == NULL)
        return;
    node* next_node;

    pthread_mutex_lock(&list->lock);
    node* current = list->head;

    while (current != NULL) {

        pthread_mutex_lock(&current->lock);
        next_node = current->next;
        if (next_node != NULL) {
            pthread_mutex_lock(&next_node->lock);
        }
        pthread_mutex_unlock(&current->lock);
        pthread_mutex_destroy(&current->lock);
        free(current);

        current = next_node;
    }

    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
    free(list);
}

void insert_value(list* list, int value)
{
    if (list == NULL)
        return;
    node* new_node = (node*)malloc(sizeof(node));
    new_node->value = value;
    new_node->next = NULL;
    pthread_mutex_init(&(new_node->lock), NULL);
    node* current, *next;

    pthread_mutex_lock(&list->lock);
    current = list->head;

    if (!current) {
        list->head = new_node;
        pthread_mutex_unlock(&list->lock);
        return;
    }

    if (current->value > value) {
        new_node->next = current;
        list->head = new_node;
        pthread_mutex_unlock(&list->lock);
        return;
    }

    next = list->head->next;

    pthread_mutex_lock(&current->lock);
    pthread_mutex_unlock(&list->lock);
    if (next) pthread_mutex_lock(&next->lock);

    while (next) {
        if (next->value > value) {
            break;
        }
        node* old_current = current;
        current = next;
        next = next->next;
        pthread_mutex_unlock(&old_current->lock);
        if (next) pthread_mutex_lock(&next->lock);
    }

    new_node->next = next;
    current->next = new_node;
    pthread_mutex_unlock(&current->lock);
    if (next) pthread_mutex_unlock(&next->lock);
}

void remove_value(list* list, int value)
{
    if (list == NULL)
        return;
    node* next_node;
    pthread_mutex_lock(&list->lock);
    node* current = list->head;
    if (!current) {
        pthread_mutex_unlock(&list->lock);
        return;
    }

    if (current->value == value){
    
        list->head = current->next;
        free(current);
        pthread_mutex_unlock(&list->lock);
        return;
    }

    pthread_mutex_lock(&current->lock);
    pthread_mutex_unlock(&list->lock);

    while (current->next != NULL) {
        
        next_node = current->next;
        pthread_mutex_lock(&(next_node->lock));

        if (next_node->value == value) {
            current->next = next_node->next;
            pthread_mutex_unlock(&next_node->lock);
            pthread_mutex_destroy(&next_node->lock);
            free(next_node);
            pthread_mutex_unlock(&current->lock);
            return;
        }
        pthread_mutex_unlock(&current->lock);
        current = next_node;
       
    }

    pthread_mutex_unlock(&current->lock);
}

void print_list(list* list)
{
    if (list == NULL)
        return;
    pthread_mutex_lock(&list->lock);
    node* current = list->head;
    pthread_mutex_unlock(&list->lock);
    while (current != NULL) {
        pthread_mutex_lock(&current->lock);
        print_node(current);
        node* next = current->next;
        pthread_mutex_unlock(&current->lock);
        current = next;
    }

  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0;

    if (list == NULL)
        return;

    pthread_mutex_lock(&list->lock);
    node* current = list->head;
    pthread_mutex_unlock(&list->lock);

    while (current != NULL) {
        pthread_mutex_lock(&current->lock);
        if(predicate(current->value))
            count++;
        node* next = current->next;
        pthread_mutex_unlock(&current->lock);
        current = next;
    }

  printf("%d items were counted\n", count); // DO NOT DELETE
}
