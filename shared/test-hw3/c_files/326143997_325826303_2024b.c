#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

void createNode(node* prev, node* next, int value, list* list);
void print_node(node* node);

/*Done*/
struct node {
  int value;
  node* next;
  node* prev;
  pthread_mutex_t lock;
};

/*Done*/
struct list {
    node* beg;
    pthread_mutex_t lock;
};

/*Done*/
list* create_list()
{
    list* temp = (list*)malloc(sizeof(list));
    temp->beg = NULL;
    pthread_mutex_init(&temp->lock, NULL);
    return temp;
}

/*Done*/
void delete_list(list* list)
{
    pthread_mutex_lock(&list->lock);
    if (list->beg != NULL)
        pthread_mutex_lock(&list->beg->lock);
    node* n = list->beg;
    while (n != NULL) {
        node* temp = n;
        n = n->next;
        if (n != NULL)
            pthread_mutex_lock(&n->lock);
        pthread_mutex_unlock(&temp->lock);
        pthread_mutex_destroy(&temp->lock);
        free(temp);
    }
    list->beg = NULL;
    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
    free(list);
}

/*Done*/
void insert_value(list* list, int value)
{
    pthread_mutex_lock(&list->lock);
    node* n = list->beg;
    node* temp = n;
    if (n == NULL) { /*Empty list*/
        createNode(NULL, NULL, value, list); /*insert node*/
        pthread_mutex_unlock(&(list->lock));
        return;
    }
    else { 
        pthread_mutex_lock(&list->beg->lock);
        while (n != NULL) {
            if (n->next != NULL) /*Lock next*/
                pthread_mutex_lock(&n->next->lock);
            if (n->value >= value || n->next == NULL) /*insert Node*/
            {
                if (n->value >= value) {
                    createNode(n->prev, n, value, list);
                    /*Free locks*/
                    pthread_mutex_unlock(&n->lock);
                    if (n->next != NULL)
                        pthread_mutex_unlock(&n->next->lock);
                    if (n->prev->prev != NULL) /*n->prev is the newly inserted node*/
                        pthread_mutex_unlock(&n->prev->prev->lock);
                    else {
                        pthread_mutex_unlock(&list->lock);
                    }
                }
                else
                {
                    createNode(n, n->next, value, list);
                    pthread_mutex_unlock(&n->lock);
                    if (n->prev != NULL)
                        pthread_mutex_unlock(&n->prev->lock);
                    else
                        pthread_mutex_unlock(&list->lock);
                }
                return;
            }
            else {
                if (temp == n) /*only true for first iterration - unlock list's lock*/
                    pthread_mutex_unlock(&list->lock);
                temp = n;
                n = n->next;
                if(temp->prev != NULL)
                    pthread_mutex_unlock(&temp->prev->lock);
            }
        }
    }
}

/*Done*/
void remove_value(list* list, int value)
{
    pthread_mutex_lock(&list->lock);
    if (list->beg != NULL) {
        pthread_mutex_lock(&list->beg->lock);
        node* n = list->beg;
        node* temp = n;
        while (n != NULL) {
            if (n->next != NULL) {
                pthread_mutex_lock(&n->next->lock);
            }
            if (n->value == value) { /*Update list if found*/
                node* prev = n->prev;
                node* next = n->next;
                if (next != NULL) next->prev = prev;
                if (prev != NULL) prev->next = next;
                else { /*Update list->beg*/
                    list->beg = next;
                    pthread_mutex_unlock(&list->lock);
                }
                /*Unlock mutexes*/
                pthread_mutex_unlock(&n->lock);
                if (next) pthread_mutex_unlock(&next->lock);
                if (prev) pthread_mutex_unlock(&prev->lock);
                /*Destroy lock*/
                pthread_mutex_destroy(&n->lock);
                /*Free Dynamiclly allocated data*/
                free(n);
                return;
            } /*Else - don't delete*/
            if (temp == n) /*Only true for first iterataion*/
                pthread_mutex_unlock(&list->lock);
            temp = n;
            n = n->next;
            if (temp->prev != NULL)
                pthread_mutex_unlock(&temp->prev->lock);
        }
        pthread_mutex_unlock(&temp->lock);
    }
    else
        pthread_mutex_unlock(&list->lock);
} 

/*Done*/
void print_list(list* list)
{
    pthread_mutex_lock(&list->lock);
    if (list->beg != NULL) {
        pthread_mutex_lock(&list->beg->lock);
        node* n = list->beg;
        node* temp = n;
        while (n != NULL) {
            print_node(n);
            if (n->next != NULL) {
                pthread_mutex_lock(&n->next->lock);
            }
            if (temp == n) /*Only true for first iterataion*/
                pthread_mutex_unlock(&list->lock);
            temp = n;
            n = n->next;
            pthread_mutex_unlock(&temp->lock);
        }
    }
    else
        pthread_mutex_unlock(&list->lock);
    printf("\n"); /*DO NOT DELETE*/
}

/*Done*/
void count_list(list* list, int (*predicate)(int))
{
    int count = 0; /*DO NOT DELETE*/
    pthread_mutex_lock(&list->lock);
    if (list->beg != NULL) {
        pthread_mutex_lock(&list->beg->lock);
        node* n = list->beg;
        node* temp = n;
        while (n != NULL) {
            count += predicate(n->value);
            if (n->next != NULL) {
                pthread_mutex_lock(&n->next->lock);
            }
            if (temp == n) /*Only true for first iterataion*/
                pthread_mutex_unlock(&list->lock);
            temp = n;
            n = n->next;
            pthread_mutex_unlock(&temp->lock);
        }
    }
    else
        pthread_mutex_unlock(&list->lock);
    printf("%d items were counted\n", count); /*DO NOT DELETE*/
}

/*Done*/
void createNode(node* prev, node* next, int value, list* list) /*Allocate memory and initillize*/
{
    node* n = (node*)malloc(sizeof(node));
    n->value = value;
    n->next = next;
    n->prev = prev; 
    if (prev != NULL) prev->next = n; /*Update*/
    else list->beg = n; /*new beg of list*/
    if (next != NULL) next->prev = n; /*Update*/
    pthread_mutex_init(&n->lock, NULL);
    return;
}

/*Done*/
void print_node(node* node)
{
    if (node)
    {
        printf("%d ", node->value);
    }
    return;
}

