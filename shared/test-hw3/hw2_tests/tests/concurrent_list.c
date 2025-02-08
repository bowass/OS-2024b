#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  pthread_mutex_t lock;
  node* next;  
  int value;
};

struct list {
  //pthread_mutex_t lock;
  node* dummy; 
};

int valid_list(list* list)
{
  if(list)
  {
    return (int)(list->dummy != NULL);
  }

  return 0;
}

node* create_node(int value)
{
  node* node = (struct node*)malloc(sizeof(struct node));
  pthread_mutex_init(&node->lock, NULL);
  node->next = NULL;
  node->value = value;
  return node;
}

void delete_node(node* node)
{
  pthread_mutex_destroy(&node->lock);
  free(node);
}

void insert_after(node* insert_after_node, int value)
{
  node* new_node = create_node(value);
  new_node->next = insert_after_node->next;
  insert_after_node->next = new_node;  
}

int try_insert_value(node** previous_node_ptr, node** current_node_ptr, int param)
{
  node* previous_node = *previous_node_ptr;
  node* current_node = *current_node_ptr;

  if(previous_node)
  {
    if(current_node)
    {
      if(param <= current_node->value && param > previous_node->value)
      {
        insert_after(previous_node, param);
        pthread_mutex_unlock(&previous_node->lock);
        pthread_mutex_unlock(&current_node->lock);
        return 1;
      }
    }
    else
    {
      insert_after(previous_node, param);
      pthread_mutex_unlock(&previous_node->lock);
      return 1;
    }
  }

  pthread_mutex_unlock(&previous_node->lock);
  *previous_node_ptr = *current_node_ptr;

  return 0;
}

int try_remove_value(node** previous_node_ptr, node** current_node_ptr, int param)
{
  node* previous_node = *previous_node_ptr;
  node* current_node = *current_node_ptr;

  if(current_node)
  {
    if(param == current_node->value)
    {
      node* removed_node = current_node;
      previous_node->next = current_node->next;
      pthread_mutex_unlock(&previous_node->lock);
      pthread_mutex_unlock(&current_node->lock);
      delete_node(removed_node);
      return 1;
    }
  }

  pthread_mutex_unlock(&previous_node->lock);
  *previous_node_ptr = *current_node_ptr;

  return 0;
}

int print_node(node** previous_node_ptr, node** current_node_ptr, int param)
{
  node* previous_node = *previous_node_ptr;
  node* current_node = *current_node_ptr;

  if(current_node)
  {
    printf("%d ", current_node->value);
  }

  pthread_mutex_unlock(&previous_node->lock);
  *previous_node_ptr = *current_node_ptr;

  return 0;
}

int remove_node(node** previous_node_ptr, node** current_node_ptr, int param)
{
  node* previous_node = *previous_node_ptr;
  node* current_node = *current_node_ptr;

  if(current_node)
  {
    node* removed_node = current_node;
    previous_node->next = current_node->next;
    pthread_mutex_unlock(&current_node->lock);
    delete_node(removed_node);
  }

  return 0;
}

void safely_iterate_list(list* list, int (*process_node)(node**, node**, int), int param)
{
  if(valid_list(list))
  {
    node* previous_node = list->dummy;
    node* current_node = NULL;
    pthread_mutex_lock(&previous_node->lock);
    while(current_node = previous_node->next)
    {
      pthread_mutex_lock(&current_node->lock);
      if(process_node(&previous_node, &current_node, param))
      {
        return;
      }
    }

    process_node(&previous_node, &current_node, param);
  }
}

list* create_list()
{
  list* list = (struct list*)malloc(sizeof(struct list));
  //pthread_mutex_init(&list->lock, NULL);
  list->dummy = create_node(INT_MIN);
  return list;
}

void delete_list(list* list)
{
  if(list)
  {
    safely_iterate_list(list, remove_node, 0);   

    if(list->dummy)
    {
      delete_node(list->dummy);
    }

    free(list);
  }
}

void insert_value(list* list, int value)
{
  safely_iterate_list(list, try_insert_value, value);
}

void remove_value(list* list, int value)
{
  safely_iterate_list(list, try_remove_value, value);
}

void print_list(list* list)
{
  safely_iterate_list(list, print_node, 0);
  printf("\n");
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0;
  int try_predicate(node** previous_node_ptr, node** current_node_ptr, int param)
  {
    node* previous_node = *previous_node_ptr;
    node* current_node = *current_node_ptr;
    if(current_node)
    {
      count += predicate(current_node->value);
    }

    pthread_mutex_unlock(&previous_node->lock);
    *previous_node_ptr = *current_node_ptr;

    return 0;
  }

  safely_iterate_list(list, try_predicate, 0);
  printf("%d items were counted\n", count);
}
