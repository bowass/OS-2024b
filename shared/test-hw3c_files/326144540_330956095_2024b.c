#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  struct node* next;
  pthread_mutex_t node_mutex; // Mutex to protect this node

};

struct list {
    struct node* head;
    pthread_mutex_t list_mutex; // Mutex to protect the list structure
};

void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}

list* create_list()
{
  // add code here
  struct list* new_list = (struct list*)malloc(sizeof(struct list));
  if (!new_list) {
    perror("Memory allocation failed");
    exit(EXIT_FAILURE);
   }
  new_list->head = NULL;
  pthread_mutex_init(&new_list->list_mutex, NULL);
  return new_list;
}


void delete_list(list* list)
{
    if (list == NULL)
        return;
    pthread_mutex_lock(&list->list_mutex);
    struct node* current = list->head;
    struct node* nextNode;
    if(current!=NULL)
        pthread_mutex_lock(&current->node_mutex);
    while (current != NULL)
    {
        nextNode = current->next;
        if(nextNode)
            pthread_mutex_lock(&nextNode->node_mutex);
        pthread_mutex_unlock(&current->node_mutex);
        pthread_mutex_destroy(&current->node_mutex);
        free(current);
        current =nextNode;
    }
    pthread_mutex_unlock(&list->list_mutex);
    pthread_mutex_destroy(&list->list_mutex);
    free(list);
}



void insert_value(list* list, int value)
{
    if (list == NULL)
        return;
    struct node* current;
    struct node* prev;
    struct node* new_node = (struct node*)malloc(sizeof(struct node));;
    if (!new_node) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    new_node->value = value;
    new_node->next = NULL;
    pthread_mutex_init(&new_node->node_mutex, NULL);
    pthread_mutex_lock(&list->list_mutex);
    if (list->head == NULL||list->head->value > value )
    {
        new_node->next =list->head;
        list->head = new_node;
        pthread_mutex_unlock(&list->list_mutex);
        return;
    }
    current = list->head;
    pthread_mutex_lock(&current->node_mutex);
    pthread_mutex_unlock(&list->list_mutex);
    
    while (current->next&&current->next->value<value)
    {
        prev = current;
        current = current->next;
        pthread_mutex_lock(&current->node_mutex); 
        pthread_mutex_unlock(&prev->node_mutex);
    }

    new_node->next = current->next;
    current->next = new_node;
    pthread_mutex_unlock(&current->node_mutex);

}

void remove_value(list* list, int value)
{
    if (list == NULL)
        return;
    pthread_mutex_lock(&list->list_mutex);
    if (list->head == NULL)
    {
        pthread_mutex_unlock(&list->list_mutex);
        return;
    }
    struct node* current = list->head;
    struct node* prev;

    pthread_mutex_lock(&current->node_mutex);
  
    if (current->value == value)
    {
        list->head =current->next;

        pthread_mutex_unlock(&list->list_mutex);
        pthread_mutex_destroy(&current->node_mutex);
        free(current);
      
        return;
    }
    pthread_mutex_unlock(&list->list_mutex);
    while (current&&current->next&&current->value<=value)
    {
        prev = current;
        current = current->next;
        pthread_mutex_lock(&current->node_mutex);
        if (current->value == value)
        {
            prev->next =current->next;
            pthread_mutex_unlock(&current->node_mutex);
            pthread_mutex_destroy(&current->node_mutex);
            free(current);
            pthread_mutex_unlock(&prev->node_mutex);
            return;
        }
        pthread_mutex_unlock(&prev->node_mutex);
        
    }
    pthread_mutex_unlock(&current->node_mutex);
}

void print_list(list* list)
{
   if (list == NULL)
        return;
  pthread_mutex_lock(&list->list_mutex);
  struct node* current = list->head;
  struct node* prev;
  if (current)
    pthread_mutex_lock(&current->node_mutex);
  while(current)
  {

      print_node(current);
      prev = current;
      current = current->next;
      pthread_mutex_unlock(&prev->node_mutex);

      
      if(current!=NULL)
        pthread_mutex_lock(&current->node_mutex);
  }
  printf("\n"); // DO NOT DELETE
  pthread_mutex_unlock(&list->list_mutex);
 
}



void count_list(list* list, int (*predicate)(int))
{
    if (list == NULL)
        return;
    int count = 0; // DO NOT DELETE
    pthread_mutex_lock(&list->list_mutex);
    struct node* current = list->head;
    struct node* prev;
    if (current)
        pthread_mutex_lock(&current->node_mutex);
    while (current)
    {

        if (predicate(current->value))
            count++;
        prev = current;
        current = current->next;
        pthread_mutex_unlock(&prev->node_mutex);


        if (current != NULL)
            pthread_mutex_lock(&current->node_mutex);
    }
    pthread_mutex_unlock(&list->list_mutex);
    printf("%d items were counted\n", count); // DO NOT DELETE

}
