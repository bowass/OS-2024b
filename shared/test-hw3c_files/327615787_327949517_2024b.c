#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
    int value;
    node* next;
    pthread_mutex_t m;
};

struct list {
    node* head;
};

//pthread_mutex_t m;
// pthread_mutex_init?

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
    list* newList = (list*)malloc(sizeof(list));
    if(newList == NULL){
        //error
    }
    newList->head = NULL;
    return newList;
}

void delete_list(list* list)
{
    // can destroy list because we delete anyway

    if (list == NULL) {
        return;
    }

    if (list->head == NULL) {
        free(list);
        return;
    }
    pthread_mutex_lock(&(list->head->m));
    node* temp = list->head;


    while(list->head != NULL){
        
        

        temp = list->head;
        if (list->head->next != NULL) {
            pthread_mutex_lock(&(list->head->next->m));
        }
        list->head = list->head->next;
        pthread_mutex_unlock(&(temp->m));
        pthread_mutex_destroy(&(temp->m));
        free(temp);


    }

    free(list);

}

void insert_value(list* list, int value)
{



    node* newNode = (node*)malloc(sizeof(node));
    if (newNode == NULL) {
        //error
    }
    newNode->value = value;
    

    pthread_mutex_init(&(newNode->m), NULL);
    newNode->next = NULL; // for now


    if (list->head == NULL) {
        list->head = newNode;
        return;
    }


    pthread_mutex_lock(&(list->head->m));
    node* temp = list->head;



    if (list->head->value > value) {

        newNode->next = list->head;
        list->head = newNode;
        pthread_mutex_unlock(&(list->head->next->m));

        return;
    }


    int inserted = 0;
    pthread_mutex_t* temp_m;

    while (inserted == 0)
    {
        if (temp->next != NULL) {
            pthread_mutex_lock(&(temp->next->m));
        }
        if (temp->next == NULL) {
            newNode->next = temp->next;
            temp->next = newNode;
            inserted = 1;
            pthread_mutex_unlock(&(temp->m));
        }
        else if (value < temp->next->value) {
            newNode->next = temp->next;
            temp->next = newNode;
            inserted = 1;
            pthread_mutex_unlock(&(temp->m));
            pthread_mutex_unlock(&(temp->next->next->m)); 
        }
        else {
            temp_m = &(temp->m);
            temp = temp->next;
            pthread_mutex_unlock(temp_m);

        }
    }

}

void remove_value(list* list, int value)
{
    
    if (list->head == NULL) {
        return;
    }
    pthread_mutex_lock(&(list->head->m));

    node* temp = list->head;

    if (temp->value == value) {
        list->head = list->head->next;
        free(temp);
        return;
    }


    pthread_mutex_t* temp_m;
    node* removed_node;

    // looking to remove temp->next
    while (temp->next != NULL) {
        pthread_mutex_lock(&(temp->next->m));

        if (temp->next->value == value) {
            removed_node = temp->next;
            temp->next = temp->next->next;
            pthread_mutex_unlock(&(removed_node->m));
            pthread_mutex_destroy(&(removed_node->m));
            free(removed_node);

            pthread_mutex_unlock(&(temp->m));

            return;
        }
        else {
            temp_m = &(temp->m);
            temp = temp->next;
            pthread_mutex_unlock(temp_m);
        }

    }
}

void print_list(list* list)
{

    if (list == NULL) {
        // do nothing
        // need to get to the printf("\n")
    }
    else if (list->head == NULL) {
        // do nothing
        // need to get to the printf("\n")
    }
    else {
        pthread_mutex_lock(&(list->head->m));
        node* temp = list->head;
        pthread_mutex_t* temp_m; // SHOULD BE POINTER
        while (temp != NULL) {

            print_node(temp);
            temp_m = &(temp->m);
            temp = temp->next;
            if (temp != NULL) {
                pthread_mutex_lock(&(temp->m));
            }
            pthread_mutex_unlock(temp_m);

        }
    }


    printf("\n"); // DO NOT DELETE



}

void count_list(list* list, int (*predicate)(int))
{

    int count = 0; // DO NOT DELETE

    if (list == NULL) {
        return;
    }
    node* temp = list->head;
    pthread_mutex_t* temp_m;
    if (temp == NULL) {
        return;
    }
    pthread_mutex_lock(&(temp->m));
    while (temp != NULL) {

        if (predicate(temp->value) == 1) {
            count++;
        }
        temp_m = &(temp->m);
        temp = temp->next;
        if (temp != NULL) {
            pthread_mutex_lock(&(temp->m));
        }
        pthread_mutex_unlock(temp_m); // this is the next!!!

    }

    printf("%d items were counted\n", count); // DO NOT DELETE


}
