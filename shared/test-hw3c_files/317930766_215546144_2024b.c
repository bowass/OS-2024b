#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  pthread_mutex_t m;
  int value;
  node * next;
};

struct list {
  node * head;
  pthread_mutex_t m;
};

node * give_head(list * List)
{
  pthread_mutex_lock(&(List->m));
  node * list_head = List->head;
  pthread_mutex_lock(&(list_head->m));
  pthread_mutex_unlock(&(List->m));

  return list_head;
}

void print_node(node* node)
{
  // DO NOT DELETE
  pthread_mutex_trylock(&(node->m));
  if(node)
  {
    printf("%d ", node->value);
  }
   pthread_mutex_unlock(&(node->m));
}

list* create_list()
{
  list * List = (list *)malloc(sizeof(list));
  List->head = NULL;
  if (pthread_mutex_init(&(List->m), NULL) != 0)
  {
    printf("\n mutex init has failed\n");
    free(List);
    return NULL;
  }
  return List;
}

void delete_list(list* list)
{
  if (list == NULL) return;
  pthread_mutex_lock(&(list->m));
  if (list->head != NULL) {
    pthread_mutex_lock(&(list->head->m));
  }
  while (list->head != NULL)
  {
    if (list->head->next != NULL) {
      pthread_mutex_lock(&(list->head->next->m));
    }
    node * next_node = list->head->next;
    pthread_mutex_unlock(&(list->head->m));
    pthread_mutex_destroy(&(list->head->m));
    
    free(list->head);
    list->head = next_node;
  }
  pthread_mutex_unlock(&(list->m));
  pthread_mutex_destroy(&(list->m));
  free(list);
}

void insert_value(list* list, int value)
{
  // init new node
  if (list == NULL) return;
  node * new_node = (node *)malloc(sizeof(node));
  new_node->value = value;


  if (pthread_mutex_init(&(new_node->m), NULL) != 0)
  {
    printf("\n mutex init has failed\n");
    return;
  }

  // lock list
  pthread_mutex_lock(&(list->m));

  if (list->head == NULL || list->head->value > value) {
    new_node->next = list->head;
    list->head = new_node;
    // free list
    pthread_mutex_unlock(&(list->m));
    return;
  }

  
  pthread_mutex_lock(&(list->head->m)); // lock head
  node * current_node = list->head; // current node needs to find the node with the value
  
  

  node* next_node = current_node->next;
  // free list
  pthread_mutex_unlock(&(list->m));

  while (next_node != NULL) {
    // lock next_node
    pthread_mutex_lock(&(next_node->m));
    if (value <= next_node->value) {
      // free next_node
      break;
    }
    // free current_node
    pthread_mutex_unlock(&(current_node->m));

    current_node = next_node;
    next_node = next_node->next;
  }

  new_node->next = next_node;
  // unlock next_node
  if (next_node != NULL) {
    pthread_mutex_unlock(&(next_node->m));
  }
  

  current_node->next = new_node;
  // unlock current_node
  pthread_mutex_unlock(&(current_node->m));

}

void remove_value(list* list, int value)
{
  // Empty List

  if (list->head == NULL) return;

  // lock list
  pthread_mutex_lock(&(list->m));
  pthread_mutex_lock(&(list->head->m));
  node * current_node = list->head;
  // lock current_node


  // Remove head
  if (current_node->value == value) {
    node * new_head = current_node->next;
    // destroy current_node
    pthread_mutex_unlock(&(current_node->m));
    pthread_mutex_destroy(&(current_node->m));
    free(current_node);
    list->head = new_head;
    // free list
    pthread_mutex_unlock(&(list->m));

    return;
  }

  // free list
  pthread_mutex_unlock(&(list->m));
  
  while (current_node->next != NULL)
  {
    pthread_mutex_lock(&(current_node->next->m));
    node * node_to_delete = current_node->next;
    // lock node_to_delete
    if (node_to_delete->value >= value) {
      break;
    }
    else {
      // free current_node
      pthread_mutex_unlock(&(current_node->m));
      current_node = node_to_delete;
    }
  }
  
  node * node_to_delete = current_node->next;
  if (current_node->next == NULL ) {
    pthread_mutex_unlock(&(current_node->m));
    return;
  }

  if (node_to_delete->value != value) {
    pthread_mutex_unlock(&(node_to_delete->m));

  } else {
    current_node->next = node_to_delete->next;
    // destroy node_to_delete
    pthread_mutex_unlock(&(node_to_delete->m));
    pthread_mutex_destroy(&(node_to_delete->m));
    free(node_to_delete);
  }
  // free current_node
  pthread_mutex_unlock(&(current_node->m));
}

void print_list(list* list)
{
  if (list != NULL && list->head != NULL) {
    node * current_node = give_head(list);
    while(current_node != NULL)
    {
      printf("%d ",current_node->value);
      node * next_node = current_node->next;
      if ( next_node != NULL) {

        // lock next_node
        pthread_mutex_lock(&(next_node->m));
      }
      //free current_node
      pthread_mutex_unlock(&(current_node->m));

      current_node = next_node;
    }  
  }
  
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE
  //lock list & current_node
  if (list == NULL || list->head == NULL) return;
  pthread_mutex_lock(&(list->m));
  pthread_mutex_lock(&(list->head->m));
  node * current_node = list->head;

  

  //free list
  pthread_mutex_unlock(&(list->m));

  while (current_node != NULL)
  {
    if (predicate(current_node->value)) count++;
    if (current_node->next == NULL) break;
    pthread_mutex_lock(&(current_node->next->m));
    node * pre_node = current_node;
    current_node = pre_node->next;
    pthread_mutex_unlock(&(pre_node->m));

  }
  pthread_mutex_unlock(&(current_node->m));
  printf("%d items were counted\n", count); // DO NOT DELETE
}
