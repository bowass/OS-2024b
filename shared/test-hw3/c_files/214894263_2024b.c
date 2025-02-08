#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"
#include <pthread.h>

struct node {
  int value;
  node* next;
  pthread_mutex_t node_lock;
  // add more fields
};

struct list {
  // add fields
  node* head;
  pthread_mutex_t list_lock;
};

void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}

void delete_node(node* node_ptr)
{
    pthread_mutex_destroy(&(node_ptr->node_lock));
    free(node_ptr);
}

list* create_list()
{
  // add code here
  list* list_ptr = (list*) malloc(sizeof(list));
  if (list_ptr == NULL)
  {
      return NULL;
  }
  if (pthread_mutex_init(&(list_ptr->list_lock), NULL) != 0)
  {
      return NULL;
  }
  list_ptr->head = NULL;
  return list_ptr; // REPLACE WITH YOUR OWN CODE
}

void delete_list(list* list)
{
  // add code here
  node* curr;
  node* temp;
  if (list == NULL)
  {
      return;
  }
  pthread_mutex_lock(&(list->list_lock));
  curr = list->head;
  if (curr != NULL)
  {
      pthread_mutex_lock(&(curr->node_lock));
  }
  while(curr != NULL)
  {
      temp = curr->next;
      pthread_mutex_unlock(&(curr->node_lock));
      delete_node(curr);
      if (temp != NULL)
      {
          pthread_mutex_lock(&(temp->node_lock));
      }
      curr = temp;
  }
  pthread_mutex_unlock(&(list->list_lock));
  pthread_mutex_destroy(&(list->list_lock));
  free(list);
}

node* create_node(int value)
{
    node* new_node = (node*) malloc(sizeof(node));
    if (!new_node)
    {
        return NULL;
    }
    new_node->value = value;
    new_node->next = NULL;
    if (pthread_mutex_init(&(new_node->node_lock), NULL) != 0)
    {
        return NULL;
    }
    return new_node;
}

void insert_value(list* list, int value)
{
  // add code here
  node* curr;
  node* next;
  node* new_node;
  node* temp;
  if (list == NULL)
  {
      return;
  }
  new_node = create_node(value);
  if (new_node == NULL)
  {
      return;
  }
  pthread_mutex_lock(&(list->list_lock));
  curr = list->head;
  if (curr == NULL)
  {
      list->head = new_node;
      pthread_mutex_unlock(&(list->list_lock));
      return;
  }
  pthread_mutex_lock(&(curr->node_lock));
  if (new_node->value < curr->value)
  {
      list->head = new_node;
      new_node->next = curr;
      pthread_mutex_unlock(&(curr->node_lock));
      pthread_mutex_unlock(&(list->list_lock));
      return;
  }
  pthread_mutex_unlock(&(list->list_lock));
  next = curr->next;
  if (next != NULL)
  {
      pthread_mutex_lock(&(next->node_lock));
  }
  while(next != NULL)
  {
      if (curr->value <= new_node->value && next->value >= new_node->value)
      {
          curr->next = new_node;
          new_node->next = next;
          pthread_mutex_unlock(&(curr->node_lock));
          pthread_mutex_unlock(&(next->node_lock));
          return;
      }
      if (next->next != NULL)
      {
          pthread_mutex_lock(&(next->next->node_lock));
      }
      temp = curr;
      curr = curr->next;
      pthread_mutex_unlock(&(temp->node_lock));
      next = curr->next;
  }
  curr->next = new_node;
  pthread_mutex_unlock(&(curr->node_lock));
}

void remove_value(list* list, int value)
{
  // add code here
  node* curr;
  node* prev;
  node* temp;
  if (list == NULL)
  {
      return;
  }
  pthread_mutex_lock(&(list->list_lock));
  curr = list->head;
  if (curr == NULL)
  {
      pthread_mutex_unlock(&(list->list_lock));
      return;
  }
  pthread_mutex_lock(&(curr->node_lock));
  if (curr->value == value)
  {
      list->head = curr->next;
      pthread_mutex_unlock(&(curr->node_lock));
      delete_node(curr);
      pthread_mutex_unlock(&(list->list_lock));
      return;
  }
  pthread_mutex_unlock(&(list->list_lock));
  if (curr->next != NULL)
  {
      pthread_mutex_lock(&(curr->next->node_lock));
  }
  prev = curr;
  curr = curr->next;
  while (curr != NULL)
  {
      if (curr->value == value)
      {
          prev->next = curr->next;
          pthread_mutex_unlock(&(curr->node_lock));
          delete_node(curr);
          pthread_mutex_unlock(&(prev->node_lock));
          return;
      }
      if (curr->next != NULL)
      {
          pthread_mutex_lock(&(curr->next->node_lock));
      }
      temp = prev;
      prev = curr;
      curr = curr->next;
      pthread_mutex_unlock(&(temp->node_lock));
  }
  pthread_mutex_unlock(&(prev->node_lock));
}

void print_list(list* list)
{
  // add code here
  node* curr;
  node* temp;
  if (list == NULL)
  {
      return;
  }
  pthread_mutex_lock(&(list->list_lock));
  curr = list->head;
  if (curr !=  NULL)
  {
      pthread_mutex_lock(&(curr->node_lock));
  }
  pthread_mutex_unlock(&(list->list_lock));
  while (curr != NULL)
  {
      printf("%d ", curr->value);
      if (curr->next != NULL)
      {
          pthread_mutex_lock(&(curr->next->node_lock));
      }
      temp = curr;
      curr = curr->next;
      pthread_mutex_unlock(&(temp->node_lock));
  }
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  // add code here
  node* curr;
  node* temp;
  if (list == NULL)
  {
      return;
  }
  pthread_mutex_lock(&(list->list_lock));
  curr = list->head;
  if (curr !=  NULL)
  {
      pthread_mutex_lock(&(curr->node_lock));
  }
  pthread_mutex_unlock(&(list->list_lock));
  while (curr != NULL)
  {
      if (predicate(curr->value))
      {
          count++;
      }
      if (curr->next != NULL)
      {
          pthread_mutex_lock(&(curr->next->node_lock));
      }
      temp = curr;
      curr = curr->next;
      pthread_mutex_unlock(&(temp->node_lock));
  }
  printf("%d items were counted\n", count); // DO NOT DELETE
}
