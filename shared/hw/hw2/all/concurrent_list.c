#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
  int value;

  // add more fields
};

struct list {
  // add fields
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
  // add code here
  return NULL; // REPLACE WITH YOUR OWN CODE
}

void delete_list(list* list)
{
  // add code here
}

void insert_value(list* list, int value)
{
  // add code here
}

void remove_value(list* list, int value)
{
  // add code here
}

void print_list(list* list)
{
  // add code here

  printf("\n"); // DO NOT DELETE
}

void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  // add code here

  printf("%d items were counted\n", count); // DO NOT DELETE
}
