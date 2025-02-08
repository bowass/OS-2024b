#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "concurrent_list.h"
#define LOCK(p) pthread_mutex_lock(&((p)->mutex))
#define UNLOCK(p) pthread_mutex_unlock(&((p)->mutex))
#define DESTROY(p)                        \
  do                                      \
  {                                       \
    pthread_mutex_destroy(&((p)->mutex)); \
    free((p));                            \
  } while (0)

struct node
{
  int value;
  node *next;
  pthread_mutex_t mutex;
};

struct list
{
  node *head; // dummy node
};

static void print_node(node *const node)
{
  if (node)
  {
    printf("%d ", node->value);
  }
}

static node *create_node(const int value, node *const next)
{
  node *const new_node = (node *)malloc(sizeof(node));
  new_node->value = value;
  new_node->next = next;
  pthread_mutex_init(&(new_node->mutex), NULL);
  return new_node;
}

list *create_list()
{
  list *const new_list = (list *)malloc(sizeof(list));
  new_list->head = create_node(INT_MIN, NULL);
  return new_list;
}

void delete_list(list *const list)
{
  node *prev, *next;
  for (prev = list->head, LOCK(prev); NULL != (next = prev->next); prev = next)
  {
    LOCK(next);
    DESTROY(prev);
  }
  DESTROY(prev);
  free(list);
}

void insert_value(list *const list, const int value)
{
  node *prev, *next;
  for (prev = list->head, LOCK(prev); NULL != (next = prev->next); prev = next)
  {
    LOCK(next);
    if (next->value > value)
      break;
    UNLOCK(prev);
  }
  prev->next = create_node(value, next);
  UNLOCK(prev);
  if (next)
    UNLOCK(next);
}

void remove_value(list *const list, const int value)
{
  node *prev, *next;
  for (prev = list->head, LOCK(prev); NULL != (next = prev->next); prev = next)
  {
    LOCK(next);
    if (next->value == value)
    {
      prev->next = next->next;
      DESTROY(next);
      break;
    }
    UNLOCK(prev);
  }
  UNLOCK(prev);
}

void print_list(list *const list)
{
  node *prev, *next;
  for (prev = list->head, LOCK(prev); NULL != (next = prev->next); prev = next)
  {
    LOCK(next);
    UNLOCK(prev);
    print_node(next);
  }
  UNLOCK(prev);
  printf("\n");
}

void count_list(list *const list, int (*predicate)(int))
{
  int count = 0;
  node *prev, *next;
  for (prev = list->head, LOCK(prev); NULL != (next = prev->next); prev = next)
  {
    LOCK(next);
    UNLOCK(prev);
    count += predicate(next->value);
  }
  UNLOCK(prev);
  printf("%d items were counted\n", count);
}
#undef LOCK
#undef UNLOCK
#undef DESTROY
