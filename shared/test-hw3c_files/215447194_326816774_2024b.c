#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  node* next;
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

list* create_list()
{
  list* ls;
  if((ls = (list*) malloc(sizeof(list)))==NULL){
    printf("error malloc\n");
    exit(1);
  }
  if(pthread_mutex_init(&(ls->lock), NULL)!=0){
    printf("mutex init error");
  }
  ls->head = NULL;
  return ls; // REPLACE WITH YOUR OWN CODE
}

void delete_list(list* list)
{
  // add code here
  pthread_mutex_lock(&(list->lock));

  while(list->head){
    node* tmp=list->head;
    if(pthread_mutex_destroy(&(tmp->lock))!=0)
      printf("mutex destroy error");
    list->head = list->head->next;
    free(tmp);
  }
  pthread_mutex_unlock(&(list->lock));
  if(pthread_mutex_destroy(&(list->lock))!=0)
      printf("mutex destroy error");
  free(list);
}

void insert_value(list* list, int value)
{
  // add code here
  if(!list){
    printf("no list error\n");
    exit(1);
  }
  node* nptr = (node*) malloc(sizeof(node));
  if(pthread_mutex_init(&(nptr->lock), NULL)!=0){
    printf("mutex init error");
  }
  pthread_mutex_lock(&(nptr->lock));
  nptr->value = value;

  pthread_mutex_lock(&(list->lock));
  if(!list->head || list->head->value > value){ //new value is the new head
    nptr->next = list->head;
    list->head = nptr;
    pthread_mutex_unlock(&(list->lock));
    pthread_mutex_unlock(&(nptr->lock));

  }else{

    node* pos = list->head;
    pthread_mutex_lock(&(pos->lock));
    pthread_mutex_unlock(&(list->lock));

    if(pos->next)
      pthread_mutex_lock(&(pos->next->lock));

    while(pos->next && pos->next->value <= value){
      pthread_mutex_unlock(&(pos->lock)); //old pos (we wont use him again)
      pos = pos->next;

      if(pos->next)
        pthread_mutex_lock(&(pos->next->lock));
    }
    nptr->next = pos->next;
    pos->next = nptr;
    if(nptr->next)
      pthread_mutex_unlock(&(nptr->next->lock));

    pthread_mutex_unlock(&(pos->lock));
    pthread_mutex_unlock(&(nptr->lock));
  }
}

void remove_value(list* list, int value)
{
  if(!list){
    printf("no list error\n");
    exit(1);
  }
  pthread_mutex_lock(&(list->lock));
  node* pos = list->head;
  if(pos)
    pthread_mutex_lock(&(pos->lock));
  else
    return; //if link is empty do nothing

  pthread_mutex_unlock(&(list->lock));

  if(pos->value == value){ //if we need to remove first value
    pthread_mutex_lock(&(list->lock)); //we can still use list while we check if (this is why we lock twice)
    list->head = pos->next;
    pthread_mutex_unlock(&(list->lock));
    pthread_mutex_unlock(&(pos->lock));
    if(pthread_mutex_destroy(&(pos->lock))!=0)
      printf("mutex destroy error");
    free(pos);
  }else{
    if (pos->next)
      pthread_mutex_lock(&(pos->next->lock));
    while(pos->next && pos->next->value !=value){
      pthread_mutex_unlock(&(pos->lock)); //old pos (we wont use him again)
      pos = pos->next;

      if(pos->next)
        pthread_mutex_lock(&(pos->next->lock));
    }
    node* tmp = pos->next;
    pos->next = pos->next->next;
    pthread_mutex_unlock(&(pos->lock));
    pthread_mutex_unlock(&(tmp->lock));
    if(pthread_mutex_destroy(&(tmp->lock))!=0)
      printf("mutex destroy error");
    free(tmp);
  }
  
}

void print_list(list* list)
{
  if(!list){
    printf("no list error\n");
    exit(1);
  }
  pthread_mutex_lock(&(list->lock));
  node* pos = list->head;
  if(pos)
    pthread_mutex_lock(&(pos->lock));
  pthread_mutex_unlock(&(list->lock));
  while(pos){
    print_node(pos);
    pthread_mutex_unlock(&(pos->lock));
    if(pos->next)
      pthread_mutex_lock(&(pos->next->lock));
    pos=pos->next;
  }
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  if(!list){
    printf("no list error\n");
    exit(1);
  }
  pthread_mutex_lock(&(list->lock));
  node* pos = list->head;
  pthread_mutex_lock(&(pos->lock));
  pthread_mutex_unlock(&(list->lock));

  while(pos){
    if(predicate(pos->value)==1)
      count++;
    pthread_mutex_unlock(&(pos->lock));
    if(pos->next)
      pthread_mutex_lock(&(pos->next->lock));
    pos=pos->next;
  }
  printf("%d items were counted\n", count); // DO NOT DELETE
}
