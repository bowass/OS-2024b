#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  // add more fields
  struct node* next;
  pthread_mutex_t lock;
};

struct list {
  // add fields
  node *head;
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
    list* new_list;
    new_list=(list*)malloc(sizeof(list));
    new_list->head=NULL;
    return new_list;
}

void delete_list(list* list)
{
    node *tmp;
    node *p;
    if(list!=NULL)
    {
        p=list->head;
        while(p!=NULL)
        {
            tmp=p;
            p=p->next;
            pthread_mutex_destroy(&tmp->lock);
            free(tmp);
        }
        free(list);
    }
}
//till here perfect
void insert_value(list* list, int value)//i think its ok
{
    if(list==NULL)return;
    node* new_node=(node*)malloc(sizeof(node));
    new_node->value=value;
    new_node->next=NULL;
    pthread_mutex_init(&(new_node->lock), NULL);
    pthread_mutex_lock(&new_node->lock);

    node* current=list->head;
    if(current==NULL){//if empty
        list->head=new_node;
        pthread_mutex_unlock(&new_node->lock);
        return;
    }
    pthread_mutex_lock(&current->lock);

    if(current->next==NULL)//if there's 1 node in the list
    {
        if(value<(current->value))
        {
            new_node->next=current;
            list->head=new_node;
        }
        else
        {
            current->next=new_node;
        }
        pthread_mutex_unlock(&current->lock);
        pthread_mutex_unlock(&new_node->lock);
        return;
    }
    if(value<current->value)//insert first
    {
        new_node->next=current;
        list->head=new_node;
        pthread_mutex_unlock(&current->lock);
        pthread_mutex_unlock(&new_node->lock);
        return;
    }
    node* l=current->next;
    pthread_mutex_lock(&l->lock);//current and l are locked
    while(l!=NULL && value>l->value)//middle or end
    {
        pthread_mutex_unlock(&current->lock);
        current=l;
        l=l->next;
        if(l!=NULL){
            pthread_mutex_lock(&l->lock);
        }
    }//at the end p& l are locked
    current->next=new_node;
    if(l!=NULL)//node at end
    {
        new_node->next=l;
        pthread_mutex_unlock(&l->lock);
    }
    pthread_mutex_unlock(&current->lock);
    pthread_mutex_unlock(&new_node->lock);
}

//till here its okay;
void remove_value(list* list, int value)
{
    node *p;
    node *l;
    if(list!=NULL){
    p=list->head;
    if(p!=NULL){
        pthread_mutex_lock(&p->lock);
        if(p->value==value)//if in first node
        {
            list->head=p->next;
            pthread_mutex_unlock(&p->lock);
            pthread_mutex_destroy(&p->lock);
            free(p);
            return;
        }

    l=p->next;
    while(l!=NULL && l->value!=value)//in case the value is in the middle or end
    {
        pthread_mutex_unlock(&p->lock);
        p=l;
        l=l->next;//the one we're deleting
        if(l!=NULL)
            pthread_mutex_lock(&l->lock);
    }
    if(l!=NULL)// l->value==value
    {
        p->next=l->next;
        pthread_mutex_unlock(&l->lock);
        pthread_mutex_destroy(&l->lock);
        free(l);
    }
    pthread_mutex_unlock(&p->lock);
    }
   }
}

void print_list(list* list)//i think its good
{
    if(list!=NULL){
        node* current=list->head;
        if(current!=NULL){
        pthread_mutex_lock(&current->lock);
        }
        while(current!=NULL)
        {
            node* tmp=current;
            print_node(current);
            current=current->next;
            if(current!=NULL){
                pthread_mutex_lock(&current->lock);
            }
            pthread_mutex_unlock(&tmp->lock);
        }
    }
  printf("\n"); // DO NOT DELETE
}

//should be correct
void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE
  if(list!=NULL){
  node* p=list->head;
  if(p!=NULL)
  {
      pthread_mutex_lock(&p->lock);
  }

  while(p!=NULL)
  {
      node* l=p;
      if(predicate(p->value)){
        count++;
      }
      p=p->next;
      if(p!=NULL)
      {
          pthread_mutex_lock(&p->lock);
      }
      pthread_mutex_unlock(&l->lock);
  }
  }
  printf("%d items were counted\n", count); // DO NOT DELETE
}


