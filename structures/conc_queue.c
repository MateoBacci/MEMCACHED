#include "conc_queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

struct _ConcQueue
{
  Queue qu;
  pthread_mutex_t mut;
};

Queue conc_queue_queue(ConcQueue cq)
{
  return cq->qu;
}

void conc_queue_lock(ConcQueue cq)
{
  pthread_mutex_lock(&cq->mut);
}

void conc_queue_unlock(ConcQueue cq)
{
  pthread_mutex_unlock(&cq->mut);
}

ConcQueue conc_queue_create()
{
  ConcQueue queue = malloc(sizeof(struct _ConcQueue));
  pthread_mutex_init(&queue->mut, NULL);
  queue->qu = queue_create();
  return queue;
}

void conc_queue_destroy(ConcQueue queue)
{
  pthread_mutex_destroy(&queue->mut);
  queue_destroy(queue->qu);
  free(queue);
}

int conc_queue_is_empty(ConcQueue queue)
{
  pthread_mutex_lock(&queue->mut);
  int isEmpty = queue_is_empty(queue->qu);
  pthread_mutex_unlock(&queue->mut);
  return isEmpty;
}

int conc_queue_delete(ConcQueue queue, QueueData *data, int pos)
{
  pthread_mutex_lock(&queue->mut);
  int a = queue_delete(queue->qu, data, pos);
  pthread_mutex_unlock(&queue->mut);
  return a;
}
