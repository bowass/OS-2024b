#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"


struct node {
    int value;
    struct node* next;
    struct node* prev;
    pthread_mutex_t lock;
};

struct list {
    node* head;
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
    list* new_list = (list*)malloc(sizeof(list));
    new_list->head = NULL;
    return new_list;
}

void delete_list(list* list)
{
    node* head = NULL;
    node* curr = NULL;
    if(list != NULL){
        head = list->head;
        while(head != NULL){
            curr = head;
            head = head->next;
            pthread_mutex_destroy(&(curr->lock));
            free(curr);
        }
        free(list);
    }
}

void insert_value(list* list, int value)
{
    if(list != NULL){
        node* newNode = (node*)malloc(sizeof(node));
        newNode->value = value;
        pthread_mutex_init(&(newNode->lock), NULL);
        if(list->head != NULL){
            pthread_mutex_lock(&(list->head->lock));
        }
        if(list->head == NULL || list->head->value > value){
            if(list->head != NULL){
                list->head->prev = newNode;
            }
            newNode->next = list->head;
            newNode->prev = NULL;
            list->head = newNode;
            if(newNode->next != NULL){
                pthread_mutex_unlock(&(newNode->next->lock));
            }
        }
        else{
            node* curr = list->head;
            node* prev = NULL;
            while(curr != NULL && curr->value < value){
                if(curr->next != NULL){
                    pthread_mutex_lock(&(curr->next->lock));
                }
                prev = curr;
                curr = curr->next;
                if(curr != NULL && curr->value < value){
                    pthread_mutex_unlock(&(prev->lock));
                }
            }
            if(curr == NULL){
                newNode->next = NULL;
                newNode->prev = prev;
                prev->next = newNode;
                pthread_mutex_unlock(&(prev->lock));
            }
            else{
                newNode->next = curr;
                newNode->prev = prev;
                prev->next = newNode;
                curr->prev = newNode;
                pthread_mutex_unlock(&(prev->lock));
                pthread_mutex_unlock(&(curr->lock));
            }
        }
    }
}

    
void remove_value(list* list, int value)
{
    if(list != NULL){
        node* curr = list->head;
        node* prev = NULL;
        if(curr!= NULL){
            pthread_mutex_lock(&(curr->lock));
        }
        while(curr != NULL && curr->value != value){
            if(curr->next != NULL){
                pthread_mutex_lock(&(curr->next->lock));
            }
            prev = curr;
            curr = curr->next;
            pthread_mutex_unlock(&(prev->lock));
        }
        if(curr != NULL){
            if(curr->prev == NULL){
                if(curr->next != NULL){
                    pthread_mutex_lock(&(curr->next->lock));
                    list->head = curr->next;
                    list->head->prev = NULL;
                    pthread_mutex_unlock(&(curr->next->lock));
                }
                else{
                    list->head = NULL;
                    list->head = NULL;
                }
            }
            else if(curr->next == NULL){
                pthread_mutex_lock(&(curr->prev->lock));
                curr->prev->next = NULL;
                pthread_mutex_unlock(&(curr->prev->lock));
            }
            else{
                pthread_mutex_lock(&(curr->prev->lock));
                pthread_mutex_lock(&(curr->next->lock));
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
                pthread_mutex_unlock(&(curr->prev->lock));
                pthread_mutex_unlock(&(curr->next->lock));
            }
            pthread_mutex_unlock(&(curr->lock));
            free(curr);
        }
    }
}

void print_list(list* list)
{
    if(list != NULL){
        node* curr = list->head;
        node* prev = NULL;
        if(curr!= NULL){
            pthread_mutex_lock(&(curr->lock));
        }
        while(curr != NULL){
            if(curr->next != NULL){
                pthread_mutex_lock(&(curr->next->lock));
            }
            print_node(curr);
            prev = curr;
            curr = curr->next;
            pthread_mutex_unlock(&(prev->lock));
        }
    }
    printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
    int count = 0; // DO NOT DELETE
    if(list != NULL){
        node* curr = list->head;
        node* prev = NULL;
        if(curr!= NULL){
            pthread_mutex_lock(&(curr->lock));
        }
        while(curr != NULL){
            if(curr->next != NULL){
                pthread_mutex_lock(&(curr->next->lock));
            }
            count += predicate(curr->value);
            prev = curr;
            curr = curr->next;
            pthread_mutex_unlock(&(prev->lock));
        }
    }
    printf("%d items were counted\n", count); // DO NOT DELETE
}
