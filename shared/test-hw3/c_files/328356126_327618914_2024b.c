#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;
  node* next;
  pthread_mutex_t node_lock;
};

struct list {
  node* first_node;
  pthread_mutex_t list_lock;
} ;

//a function that printing the given node
void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}

//creating a new list
list* create_list()
{
  list* new_list= (list*)calloc(1,sizeof(list));
  pthread_mutex_init(&new_list->list_lock, NULL);
  pthread_mutex_lock(&new_list->list_lock);
  new_list->first_node=NULL;
  pthread_mutex_unlock(&new_list->list_lock);
  return new_list;
}

//deleting the list and it's nodes
void delete_list(list* list)
{
  pthread_mutex_lock(&list->list_lock);//locked because the first node is deleted, so its imposible to get to the list
  node* current=list->first_node;
  if(!current){
      pthread_mutex_unlock(&list->list_lock);
      free(list);
      list=NULL;
      return;
  }
  pthread_mutex_lock(&current->node_lock);
  pthread_mutex_unlock(&list->list_lock);
  node* temp;
  while(current)
  {
    temp=current->next;
    if(temp){
        pthread_mutex_lock(&temp->node_lock);
    }
    pthread_mutex_unlock(&current->node_lock);
    free(current);
    current=temp;
       
    }
  free(list);
  list=NULL;  
}



//insert a new node with the given value
void insert_value(list* list, int value)
{
  node* new_node= (node*)calloc(1,sizeof(node));
  pthread_mutex_init(&new_node->node_lock, NULL);
  pthread_mutex_lock(&new_node->node_lock);//locked always
  new_node->value=value;
  pthread_mutex_lock(&list->list_lock);
  if (!list->first_node)//case zero, the list is empty
  {
    new_node->next=NULL;
    list->first_node=new_node;
    pthread_mutex_unlock(&list->list_lock);
    pthread_mutex_unlock(&new_node->node_lock);
    return;
  }
  if (list->first_node->value>=value)//case one, the value is smaller then the first node's value
  {
    new_node->next=list->first_node;
    list->first_node=new_node;
    pthread_mutex_unlock(&list->list_lock);
    pthread_mutex_unlock(&new_node->node_lock);
    return;
  }

  else{
    node* current=list->first_node;//maybe pointer
    pthread_mutex_lock(&current->node_lock);
    pthread_mutex_unlock(&list->list_lock);  
    int flag = 0;//1 if middle case is handled 
    node* temp;  
    while (current->next && flag==0)
    {
        pthread_mutex_lock(&current->next->node_lock);
        if (value <= current->next->value) {//case middle
            temp = current->next;
            current->next = new_node;
            new_node->next = temp;
            pthread_mutex_unlock(&current->node_lock);
            pthread_mutex_unlock(&current->next->next->node_lock);
            flag = 1;
            pthread_mutex_unlock(&new_node->node_lock);
            return;
        }
        temp = current;
        current = current->next;
        pthread_mutex_unlock(&temp->node_lock);
    }
    if (flag == 0) {//case last
        current->next = new_node;
        new_node->next = NULL;
        pthread_mutex_unlock(&current->node_lock);
    }
  }
  pthread_mutex_unlock(&new_node->node_lock);
}

void remove_value(list* list, int value)
{
  pthread_mutex_lock(&list->list_lock);
  if (value==list->first_node->value)//case first, the first node' value==value
  {
    node* temp = list->first_node;
    list->first_node= list->first_node->next;
    pthread_mutex_unlock(&list->list_lock);
    free(temp);
    return;
  }
  else {
      node* current = list->first_node;//maybe pointer
      node* temp;
      pthread_mutex_lock(&current->node_lock);
      pthread_mutex_unlock(&list->list_lock);//מה אם בדיוק בניהם
      while (current->next)//case middle+ last
      {
          pthread_mutex_lock(&current->next->node_lock);//צריך מנעול בכלל?מצד אחד אסור 
          if (value ==current->next->value) {//האם צריך לוק לנקס נקס כי אנחנו משנות את נקסט ויכולים למחוק את נקס נקס?
              temp =current->next;
              current->next = temp->next;
              pthread_mutex_unlock(&temp->node_lock);
              free(temp);
              pthread_mutex_unlock(&current->node_lock);
              return;
          }
          temp=current;
          current = current->next;//אם יש שגיאה כי אודליה אמרה שמותר למרות שאסור
          pthread_mutex_unlock(&temp->node_lock);
      }
      pthread_mutex_unlock(&current->node_lock);
  }
}

void print_list(list* list){
  if(!list){
      printf("\n"); // DO NOT DELETE
      return;
    }
  pthread_mutex_lock(&list->list_lock);
  node* current = list->first_node;
  if(!current){
      printf("\n"); // DO NOT DELETE
      return;
  }
  pthread_mutex_lock(&current->node_lock);
  pthread_mutex_unlock(&list->list_lock);
  node* temp = current;
  while (current)
  {
      print_node(current);
      temp = current;
      current = current->next;
      if(current){
        pthread_mutex_lock(&current->node_lock);
      }
      pthread_mutex_unlock(&temp->node_lock);
  }
  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE
  if(!list){
     printf("%d items were counted\n", count); // DO NOT DELETE
      return;
    }
  pthread_mutex_lock(&list->list_lock);
  node* current = list->first_node;
  if(!current){
      printf("%d items were counted\n", count); // DO NOT DELETE
      return;
  }
  pthread_mutex_lock(&current->node_lock);
  pthread_mutex_unlock(&list->list_lock);
  node* temp = current;
  while (current)
  {
      count+=predicate(current->value);
      temp = current;
      current = current->next;
      if(current){
        pthread_mutex_lock(&current->node_lock);
      }
      pthread_mutex_unlock(&temp->node_lock);
  }
  printf("%d items were counted\n", count); // DO NOT DELETE
}


