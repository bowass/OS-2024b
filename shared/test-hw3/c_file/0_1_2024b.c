#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  struct node* next;
  pthread_mutex_t mutex;
};

struct list {
  struct node* first;
  int isEmpty;
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
  struct list* l = malloc(sizeof(list));
  l->first= malloc(sizeof(node));
  pthread_mutex_init(&(l->first->mutex), NULL);
  l->isEmpty=1;
  return l; // REPLACE WITH YOUR OWN CODE
}

void delete_list(list* list)
{
  struct node* temp1;
  struct node* temp2;
  temp1=list->first;
  while(temp1) {
    temp2=temp1;
    temp1=temp1->next;
    pthread_mutex_destroy(&(temp2->mutex));
    free(temp2);
  }
  free(list);
}

void insert_value(list* list, int value)
{
  while(pthread_mutex_trylock(&(list->first->mutex))!=0);
  if(list->isEmpty) {
    list->isEmpty=0;
    list->first->value=value;
    pthread_mutex_unlock(&(list->first->mutex));
    return;
  }
  struct node* temp=list->first;
  if(temp->next==NULL) {
    temp->next=malloc(sizeof(struct node));
    if(temp->value<value)
        temp->next->value=value;
    else {
        temp->next->value=temp->value;
        temp->value =value;
    }
    pthread_mutex_init(&(temp->next->mutex), NULL);
    pthread_mutex_unlock(&(list->first->mutex));
    return;
  }
  struct node* current= malloc(sizeof(struct node));
  pthread_mutex_init(&(current->mutex), NULL);
  current->value=value;
  if(value<=list->first->value) {
    pthread_mutex_lock(&(current->mutex));
    current->next=list->first;
    list->first=current;
    pthread_mutex_unlock(&(list->first->mutex));
    pthread_mutex_unlock(&(list->first->next->mutex));
    return;
  }
  struct node* temp2;
  pthread_mutex_lock(&(temp->next->mutex));
  while(temp->next->next&&temp->next->value<value) {
    pthread_mutex_lock(&(temp->next->next->mutex));
    temp2=temp;
    temp=temp->next;
    pthread_mutex_unlock(&(temp2->mutex));
  }
  if(temp->next->next==NULL&&temp->next->value<value) {
    temp2=temp;
    temp=temp->next;
    pthread_mutex_unlock(&(temp2->mutex));
    temp->next=current;
    pthread_mutex_unlock(&(temp->mutex));
  }
  else {
    current->next=temp->next;
    temp->next=current;
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_unlock(&(current->next->mutex));
  }
}

void remove_value(list* list, int value)
{
  while(pthread_mutex_trylock(&(list->first->mutex))!=0);
  if(list->isEmpty) {
    pthread_mutex_unlock(&(list->first->mutex));
    return;
  }
  if(list->first->value==value&&list->first->next==NULL) {
    list->isEmpty=1;
    pthread_mutex_unlock(&(list->first->mutex));
    return;
  }
  pthread_mutex_lock(&(list->first->next->mutex));
  struct node* temp=list->first;
  struct node* temp2;
  if(list->first->value==value) {
    list->first=temp->next;
    temp->next=NULL;
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_destroy(&(temp->mutex));
    free(temp);
    pthread_mutex_unlock(&(list->first->mutex));
    return;
  }
  while(temp->next->next&&temp->next->value!=value) {
    pthread_mutex_lock(&(temp->next->next->mutex));
    temp2=temp;
    temp=temp->next;
    pthread_mutex_unlock(&(temp2->mutex));
  }
  if(temp->next->next==NULL&&temp->next->value!=value) {
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_unlock(&(temp->next->mutex));
  }
  else if(temp->next->next) {
    pthread_mutex_lock(&(temp->next->next->mutex));
    temp2=temp->next;
    temp->next=temp2->next;
    temp2->next=NULL;
    pthread_mutex_unlock(&(temp2->mutex));
    pthread_mutex_destroy(&(temp2->mutex));
    free(temp2);
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_unlock(&(temp->next->mutex));
  }
  else {
    temp2=temp->next;
    temp->next=NULL;
    pthread_mutex_unlock(&(temp2->mutex));
    pthread_mutex_destroy(&(temp2->mutex));
    free(temp2);
    pthread_mutex_unlock(&(temp->mutex));
  }
}

void print_list(list* list)
{
  pthread_mutex_lock(&(list->first->mutex));
  if(!(list->isEmpty)&&(list->first->next)) {
    struct node* temp=list->first;
    struct node* temp2;
    pthread_mutex_lock(&(temp->next->mutex));
    while(temp->next->next) {
        print_node(temp);
        pthread_mutex_lock(&(temp->next->next->mutex));
        temp2=temp;
        temp=temp->next;
        pthread_mutex_unlock(&(temp2->mutex));
    }
    print_node(temp);
    print_node(temp->next);
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_unlock(&(temp->next->mutex));
  }
  else if(list->first->next) {
    pthread_mutex_lock(&(list->first->next->mutex));
    print_node(list->first);
    print_node(list->first->next);
    pthread_mutex_unlock(&(list->first->mutex));
    pthread_mutex_unlock(&(list->first->next->mutex));
  }
  else if(!(list->isEmpty)) {
    print_node(list->first);
    pthread_mutex_unlock(&(list->first->mutex));
  }
  else
    pthread_mutex_unlock(&(list->first->mutex));
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  pthread_mutex_lock(&(list->first->mutex));
  if(!(list->isEmpty)&&(list->first->next)) {
    struct node* temp=list->first;
    struct node* temp2;
    pthread_mutex_lock(&(temp->next->mutex));
    while(temp->next->next) {
        pthread_mutex_lock(&(temp->next->next->mutex));
        temp2=temp;
        temp=temp->next;
        pthread_mutex_unlock(&(temp2->mutex));
        count++;
    }
    pthread_mutex_unlock(&(temp->mutex));
    pthread_mutex_unlock(&(temp->next->mutex));
    count+=2;
  }
  else if(list->first->next) {
    pthread_mutex_unlock(&(list->first->mutex));
    count+=2;
  }
  else if(!(list->isEmpty)) {
    count++;
    pthread_mutex_unlock(&(list->first->mutex));
  }
  else
    pthread_mutex_unlock(&(list->first->mutex));
  printf("%d items were counted\n", count); // DO NOT DELETE
}
