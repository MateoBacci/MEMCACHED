#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include <string.h>

struct _queueData
{
  long ihash;
  Data *key, *value;
};

struct _node
{
  QueueData *data;
  struct _node *next;
};

struct _Queue
{
  QueueNode *first, *last;
};


void queue_node_init(QueueNode * qn) {
  qn->data = NULL;
  qn->next = NULL;
}

QueueNode *queue_first(Queue q, QueueNode *newFirst)
{
  if (newFirst != NULL)
    q->first = newFirst;
  return q->first;
}

QueueNode *queue_last(Queue q, QueueNode *newLast)
{
  if (newLast != NULL)
    q->last = newLast;
  return q->last;
}

QueueData *queue_node_data(QueueNode *q, QueueData *newData)
{
  if (newData != NULL)
    q->data = newData;
  return q->data;
}

QueueNode *queue_node_next(QueueNode *q, QueueNode *newNext)
{
  if (newNext != NULL)
    q->next = newNext;
  return q->next;
}

void queue_data_init(QueueData *qd) {
  qd->ihash = -1;
  qd->key = qd->value = NULL;
}

unsigned int queue_data_size()
{
  return sizeof(QueueData);
}

long queue_data_ihash(QueueData *qd, long newHash)
{
  if (newHash >= 0)
    qd->ihash = newHash;
  return qd->ihash;
}

Data *queue_data_key(QueueData *qd, Data *newKey)
{
  if (newKey != NULL)
    qd->key = newKey;
  return qd->key;
}

Data *queue_data_value(QueueData *qd, Data *newValue)
{
  if (newValue != NULL)
    qd->value = newValue;
  return qd->value;
}

unsigned int queue_node_size()
{
  return sizeof(QueueNode);
}

Queue queue_create()
{
  Queue q = malloc(sizeof(struct _Queue));
  q->first = q->last = NULL;
  return q;
}

void free_queue_node(QueueNode *qn)
{
  if (qn->data != NULL)
    free_queue_data(qn->data);
  free(qn);
}

void free_queue_data(QueueData *d)
{

  if (d->key != NULL)
  {
    free_data(d->key);
  }
  if (d->value != NULL)
  {
    free_data(d->value);
  }
  free(d);
}

int queue_is_empty(Queue queue) { return queue->first == NULL; }

void queue_destroy(Queue queue)
{
  QueueNode *nodeToDestroy;
  while (!queue_is_empty(queue))
  {
    nodeToDestroy = queue->first;
    queue->first = nodeToDestroy->next;
    free_queue_node(nodeToDestroy);
  }
  free(queue);
}

void queue_push(Queue queue, QueueNode *nodo)
{
  // first es el primero agregado a la cola.
  // last es el último por lo tanto apunta a NULL.
  // Es decir que en realidad estamos agregando al final.
  // Esto simplifica al momento de eliminar.

  if (queue_is_empty(queue))
    queue->first = nodo;
  if (queue->last != NULL)
    queue->last->next = nodo;
  queue->last = nodo;
}

unsigned queue_pop(Queue qu)
{
  unsigned a = qu->first->data->ihash;
  QueueNode *aux = qu->first;
  qu->first = qu->first->next;
  if(qu->first == NULL)
    qu->last = NULL;
  free_queue_node(aux);
  return a;
}


/*
  La posición se usa cuando eliminamos un nodo de la tabla hash. Como todos los nodos
  de la misma casilla tienen el mismo índice, y el orden en el que aparecen en la casilla
  es el mismo que el de la cola de desalojo (a pesar de los nodos que tienen otro índice),
  cuando queremos eliminar el nodo de la cola que corresponde con el nodo eliminado, debemos
  buscar el que esté en la misma posición que el nodo de la tabla contando únicamente los nodos
  con su mismo índice.

  Por ejemplo: Eliminamos el 5to nodo del ínidce 10.
  Cuando busquemos en la cola, debemos ignorar los primeros 4 nodos que tengan índice 10, y recién
  el 5to será el nodo de la cola que corresponde al eliminado en la tabla.
*/
int queue_delete_by_pos(Queue q, QueueData *data, int pos)
{
  QueueNode *nodeToDelete = q->first;

  for (int i = 0; i < pos; nodeToDelete = nodeToDelete->next) {
    if (nodeToDelete->data->ihash == data->ihash)
      i++;
  }
  
  if (q->first == nodeToDelete && nodeToDelete->data->ihash == data->ihash)
  {
    q->first = nodeToDelete->next;
    if (q->last == nodeToDelete)
      q->last = NULL;
    free_queue_node(nodeToDelete);
    return 0;
  }
  /*
      Como ya chequeamos el caso en el que haya un solo elemento,
      sabemos que hay al menos dos.
  */
  QueueNode *prev = nodeToDelete;
  nodeToDelete = nodeToDelete->next;
  int position = 1;
  for (; nodeToDelete != q->last; prev = nodeToDelete, nodeToDelete = nodeToDelete->next, position++)
  {
    if (nodeToDelete->data->ihash == data->ihash)
    {
      prev->next = nodeToDelete->next;
      free_queue_node(nodeToDelete);
      return position;
    }
  }
  if (nodeToDelete->data->ihash == data->ihash)
  {
    q->last = prev;
    prev->next = NULL;
    free_queue_node(nodeToDelete);
    return position;
  }
  return -1;
}

int queue_delete(Queue queue, QueueData *data, int pos)
{
  if (queue_is_empty(queue))
    return -1;

  QueueNode *nodeToDelete = queue->first;
  if (pos >= 0)
    return queue_delete_by_pos(queue, data, pos);
  
  // Nos fijamos si es el primero
  if (memcmp(nodeToDelete->data->key->data, data->key->data, data->key->len) == 0) {
    
    if (queue->first == queue->last) {
      queue->first = NULL;
      queue->last = NULL;
    }
    else 
      queue->first = nodeToDelete->next;
    nodeToDelete->next = NULL;
    free_queue_node(nodeToDelete);
    return 0;
  }


  // Si el primero no era el que se buscaba y es el único de la cola
  // retornamos -1 porque significa que no existe.
  if (queue->first == queue->last)
    return -1;

  QueueNode *prev = nodeToDelete;
  nodeToDelete = queue->first->next;
  int position = 1;
  for (; nodeToDelete != queue->last; prev = nodeToDelete, nodeToDelete = nodeToDelete->next, position++)
  {
    if (memcmp(nodeToDelete->data->key->data, data->key->data, data->key->len) == 0) {
      prev->next = nodeToDelete->next;
      free_queue_node(nodeToDelete);
      return position;
    }
  }

  /* Chequeo el ultimo nodo. nodeToDelete = queue->last */
  if (memcmp(nodeToDelete->data->key->data, data->key->data, data->key->len) == 0)
  {
    prev->next = NULL;
    queue->last = prev;
    free_queue_node(nodeToDelete);
    return position;
  }
  return -1;
}

int queue_search(Queue q, QueueData *qd)
{
  QueueNode *aux = q->first;
  for (; aux != NULL; aux = aux->next)
  {
    if (memcmp(aux->data->key->data, qd->key->data, qd->key->len) == 0)
    {
      qd->value = malloc(sizeof(Data));
      qd->value->data = malloc(aux->data->value->len + 1);
      if (qd->value != NULL && qd->value->data != NULL) {
        memcpy(qd->value->data, aux->data->value->data, aux->data->value->len);
        qd->value->data[aux->data->value->len] = 0;
        qd->value->len = aux->data->value->len;
      }
      return aux->data->value->len;
    }
  }
  return -1;
}
