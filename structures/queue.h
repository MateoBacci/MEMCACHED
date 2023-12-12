#ifndef __QUEUE_H__
#define __QUEUE_H__
#include "../server/data.h"


typedef struct _queueData QueueData;

typedef struct _node QueueNode;

typedef struct _Queue *Queue;

/*
 * Funciones para darle una interfaz a queue.
 * El segundo argumento se utiliza para asignar un valor nuevo.
 * En caso de no querer modificarlo, debe ir NULL o 0 según corresponda.
*/ 
void queue_node_init(QueueNode * qn);

QueueNode *queue_first(Queue q, QueueNode *newFirst);

QueueNode *queue_last(Queue q, QueueNode *newLast);

QueueData *queue_node_data(QueueNode *q, QueueData *newData);

QueueNode *queue_node_next(QueueNode *q, QueueNode *newNext);

void queue_data_init(QueueData *qd);

unsigned int queue_data_size();

long queue_data_ihash(QueueData *qd, long newHash);

Data *queue_data_key(QueueData *qd, Data *newKey);

Data *queue_data_value(QueueData *qd, Data *newValue);

unsigned int queue_node_size();

/**
 * Devuelve una cola vacía.
 */
Queue queue_create();

/**
 * Destruccion de la cola.
 */
void queue_destroy (Queue queue);

/**
 * Elimina un QueueNode
*/
void free_queue_node(QueueNode *qn);

/**
 * Elimina un QueueData
*/
void free_queue_data(QueueData *qd);

/**
 * Determina si la cola es vacía.
 */
int queue_is_empty (Queue queue);

/**
 * Agrega un elemento al final de la cola.
*/
void queue_push (Queue queue, QueueNode *n);

/**
 * Elimina el primer elemento de la cola.
*/
unsigned queue_pop(Queue queue);

/**
 * Compara dos nodos.
*/
int queue_cmpdata (QueueData data1, QueueData data2);

/**
 * Elimina un elemento puntual de la cola.
 */
int queue_delete(Queue queue, QueueData *data, int pos);

/**
 * Busca un nodo en una cola. Retorna el largo del valor si lo encuentra, -1 si no
 */
int queue_search(Queue q, QueueData *qd);

#endif /* __QUEUE_H__ */