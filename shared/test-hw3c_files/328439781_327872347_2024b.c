#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"


struct node {
  int value;
  struct node* next; // Pointer to the next node in the list
  pthread_mutex_t nodeLock; // Mutex lock for concurrent access to the node
};


struct list {
   struct node* head; // Pointer to the head node of the list
};



void print_node(node* node)
{
  if(node)
  {
    printf("%d ", node->value);
  }
}



list* create_list()
{
    struct list* newListPtr = (struct list*)malloc(sizeof(struct list)); // Allocating memory for the new struct.
    newListPtr->head = NULL;
    return newListPtr;
}



void delete_list(list* list)
{
    struct node* tmp;
    while (list->head != NULL) // Deleting every node
    {
        // Making sure no other thread will access the nodes affected by the delete
        pthread_mutex_lock(&list->head->nodeLock);
        if (list->head->next)
        {
            pthread_mutex_lock(&list->head->next->nodeLock);
        }

        tmp = list->head;
        list->head = list->head->next;
        if (list->head)
        {
            pthread_mutex_unlock(&list->head->nodeLock);
        }
        pthread_mutex_destroy(&tmp->nodeLock);
        free(tmp); // Freeing the node's memory
    }

    free(list); // Freeing the list's memory
}



void insert_value(list* list, int value) { 
    // Allocating memory for the new node
    struct node* newNodePtr = (struct node*)malloc(sizeof(struct node));

    // Initializing the new node's value, next pointer, and mutex
    newNodePtr->value = value;
    newNodePtr->next = NULL;
    pthread_mutex_init(&newNodePtr->nodeLock, NULL);

    // Handling empty list
    if (!list->head) {
        list->head = newNodePtr;
        return;
    }

    // Parameters for moving in the list
    struct node* current = list->head;
    struct node* nextCurrent = current->next;

    // Special handling for the first node
    pthread_mutex_lock(&current->nodeLock);
    if (current->value >= value) {
        // Inserting at the beginning
        list->head = newNodePtr;
        list->head->next = current;
        pthread_mutex_unlock(&current->nodeLock);
        return;
    }
    pthread_mutex_unlock(&current->nodeLock);

    // Finding the insertion position
    if (nextCurrent) {
        pthread_mutex_lock(&nextCurrent->nodeLock);
    }
    while (nextCurrent && nextCurrent->value < value) {
        pthread_mutex_unlock(&current->nodeLock);
        current = nextCurrent;
        nextCurrent = current->next;
        if (nextCurrent) {
            pthread_mutex_lock(&nextCurrent->nodeLock);
        }
    }

    // Inserting in the middle or at the end
    current->next = newNodePtr;
    newNodePtr->next = nextCurrent;

    // Releasing mutexes in reverse order
    if (nextCurrent) {
        pthread_mutex_unlock(&nextCurrent->nodeLock);
    }
    pthread_mutex_unlock(&current->nodeLock);

    // No need to free(newNodePtr) here,
    // since the memory for the new node is now part of the linked list
}





void remove_value(list* list, int value) {
    struct node* tmp;
    int done = 0;
    
    if (!list)
    {
        return;
    }

    if (!list->head) {
        return;
    }

    // Lock head
    pthread_mutex_lock(&list->head->nodeLock);
    if (list->head->next)
    {
        pthread_mutex_lock(&list->head->next->nodeLock);
    }

    // Check head node for removal
    if (list->head->value == value) {
        tmp = list->head;
        list->head = tmp->next;
        pthread_mutex_destroy(&tmp->nodeLock);
        free(tmp);
        done = 1;
    }
    if (list->head)
    {
        pthread_mutex_unlock(&list->head->nodeLock);
    }
    if (list->head->next && !done)
    {
        pthread_mutex_unlock(&list->head->next->nodeLock);
    }


    // If not done, search for the node to remove (considering the head case)
    if (!done && list->head && list->head->next) { // Check if head exists after potential removal
        struct node* lastOne = list->head;
        struct node* current = list->head->next;
        struct node* nextCurrent;

        while (current && !done) {
            // This case is when the head needs to be deleted.
            pthread_mutex_lock(&current->nodeLock);
            nextCurrent = current->next;
            if (nextCurrent)
            {
                pthread_mutex_lock(&nextCurrent->nodeLock);
            }

            if (current->value == value) {
                tmp = current;
                lastOne->next = nextCurrent;
                pthread_mutex_destroy(&tmp->nodeLock);
                free(tmp);
                done = 1;
            }


            if (!done)
            {
                // Locking the current node and it's successor while reading from it.
            // If it is the node to delete, it will affect the successor, thus the locking.
                pthread_mutex_unlock(&current->nodeLock);
                if (nextCurrent) {
                    pthread_mutex_unlock(&nextCurrent->nodeLock);
                }

                lastOne = current; // Updating lastOne without locking
                current = nextCurrent;
            }
            else
            {
                // We unlock only the node exists (is not a NULL)
                if (nextCurrent) {
                    pthread_mutex_unlock(&nextCurrent->nodeLock);
                }
            }
        }
    }
}




void print_list(list* list)
{
    if (!list) // Handling deleted / unintialized list
    {
        printf("\n");
        return;
    }

    if (!list->head) // Handling empty list
    {
        printf("\n");
        return;
    }

    struct node* current;
    struct node* tmp;
    current = list->head;

  while (current) // Going over every node
  {
      // Locking access to the current node while reading from it.
      pthread_mutex_lock(&current->nodeLock);
      print_node(current);
      tmp = current;
      current = current->next;

      pthread_mutex_unlock(&tmp->nodeLock);
  }

  printf("\n"); // DO NOT DELETE
}



void count_list(list* list, int (*predicate)(int))
{
    int count = 0;

    if (!list) // Handling deleted / unitialized list
    {
        printf("%d items were counted\n", count);
        return;
    }

    if (!list->head) // Handling empty list
    {
        printf("%d items were counted\n", count);
        return;
    }

    struct node* current;
    struct node* tmp;
    current = list->head;

    // Always locking the node while reading from it
    pthread_mutex_lock(&current->nodeLock);

    while (current)
    {
        // Cheking if the node meets the required condition
        if (predicate(current->value) == 1)
        {
            count++;
            print_node(current);
        }
        tmp = current;
        current = current->next;
        pthread_mutex_unlock(&tmp->nodeLock);

        if (current)
        {
            pthread_mutex_lock(&current->nodeLock);
        }
    }
    printf("\n");

  printf("%d items were counted\n", count); // DO NOT DELETE
}
