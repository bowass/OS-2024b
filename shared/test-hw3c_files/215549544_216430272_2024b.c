#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"
typedef struct node *Nodeptr;
typedef struct list *Listptr;

struct node {
  int value;
  struct node* next;
  pthread_mutex_t mutex;
}Node;

struct list {
    struct node* head;
    pthread_mutex_t mutex;
}List;
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
    Listptr newList = (Listptr)malloc(sizeof(List));
    pthread_mutex_init(&newList->mutex, NULL);
    newList->head = NULL;
    return newList;
}
void delete_list(list* list)
{
    if (list == NULL)
        return;
    Nodeptr p = list->head;
    while(p != NULL)
    {
        Nodeptr temp = p->next;
        //free the node
        pthread_mutex_destroy(&p->mutex);
        free(p);
        p = temp;
    }
    pthread_mutex_destroy(&list->mutex);
    free(list);
}

void insert_value(list* list, int value)
{
    if(list == NULL)
        return;
    //create new node
    Nodeptr pNew = (Nodeptr)malloc(sizeof(Node));
    pthread_mutex_init(&pNew->mutex, NULL);
    pNew->value = value;

    pthread_mutex_lock(&list->mutex);
    if (list->head == NULL || list->head->value > value)//אם קטן ממש: הכנס כראשון
    {
        pNew->next = list->head;
        list->head = pNew;
        pthread_mutex_unlock(&list->mutex);
        return;
    }
    Nodeptr pPrev = list->head;
    pthread_mutex_lock(&pPrev->mutex);
    pthread_mutex_unlock(&list->mutex);
    Nodeptr pCurrent = pPrev->next;
    while(pCurrent != NULL)
    {
        if (pPrev->value <= value && value <= pCurrent->value)
        {
            pNew->next = pCurrent;
            pPrev->next = pNew;
            pthread_mutex_unlock(&pPrev->mutex);
            return;
        }
        pthread_mutex_lock(&pCurrent->mutex);
        pthread_mutex_unlock(&pPrev->mutex);
        pPrev = pCurrent;
        pCurrent = pCurrent->next;
    }
    //add to the end of the list
    pNew->next = pCurrent;//it's actually NULL
    pPrev->next = pNew;
    pthread_mutex_unlock(&pPrev->mutex);
}

void remove_value(list* list, int value)
{
    if (list == NULL)
        return;
    pthread_mutex_lock(&list->mutex);
    if(list->head == NULL || list->head->value > value)//the value is not in the list
    {
        pthread_mutex_unlock(&list->mutex);
        return;
    }
    if(list->head->value ==  value)
    {
        //remove the first node
        Nodeptr p = list->head;
        pthread_mutex_lock(&p->mutex);
        list->head = p->next;
        pthread_mutex_unlock(&p->mutex);
        pthread_mutex_destroy(&p->mutex);
        free(p);
        pthread_mutex_unlock(&list->mutex);
        return;
    }
    Nodeptr pPrev = list->head;
    pthread_mutex_lock(&pPrev->mutex);
    pthread_mutex_unlock(&list->mutex);
    Nodeptr pCurrent = pPrev->next;
    while(pCurrent != NULL)
    {
        if (pCurrent->value == value)
        {
            //lock it to remove it
            pthread_mutex_lock(&pCurrent->mutex);
            pPrev->next = pCurrent->next;
            pthread_mutex_unlock(&pCurrent->mutex);
            pthread_mutex_destroy(&pCurrent->mutex);
            free(pCurrent);
            pthread_mutex_unlock(&pPrev->mutex);
            return;
        }
        pthread_mutex_lock(&pCurrent->mutex);
        pthread_mutex_unlock(&pPrev->mutex);
        pPrev = pCurrent;
        pCurrent = pCurrent->next;
    }
    //not in list
    pthread_mutex_unlock(&pPrev->mutex);
}

void print_list(list* list)
{
    if(list == NULL)
    {
        printf("\n");
        return;
    }
    pthread_mutex_lock(&list->mutex);
    if (list->head == NULL)
        pthread_mutex_unlock(&list->mutex);
    else
    {
        //if there are values in the list
        printf("%d ", list->head->value);
        Nodeptr pPrev = list->head;
        pthread_mutex_lock(&pPrev->mutex);
        pthread_mutex_unlock(&list->mutex);
        Nodeptr pCurrent = pPrev->next;
        while(pCurrent != NULL)
        {
            printf("%d ", pCurrent->value);
            pthread_mutex_lock(&pCurrent->mutex);
            pthread_mutex_unlock(&pPrev->mutex);
            pPrev = pCurrent;
            pCurrent = pCurrent->next;
        }
        pthread_mutex_unlock(&pPrev->mutex);
    }
    printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0; // DO NOT DELETE
    if (list == NULL)
    {
        printf("%d items were counted\n", count);
        return;
    }

    pthread_mutex_lock(&list->mutex);
    if(list->head == NULL)
    {
        pthread_mutex_unlock(&list->mutex);
    }
    else
    {
        if (predicate(list->head->value) == 1)
            count++;
        Nodeptr pPrev = list->head;
        pthread_mutex_lock(&pPrev->mutex);
        pthread_mutex_unlock(&list->mutex);
        Nodeptr pCurrent = pPrev->next;
        while(pCurrent != NULL)
        {
            if(predicate(pCurrent->value) == 1)
                count++;
            pthread_mutex_lock(&pCurrent->mutex);
            pthread_mutex_unlock(&pPrev->mutex);
            pPrev = pCurrent;
            pCurrent = pCurrent->next;
        }
        pthread_mutex_unlock(&pPrev->mutex);
    }
    printf("%d items were counted\n", count); // DO NOT DELETE
}

