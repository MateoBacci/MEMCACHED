#ifndef __CONC_QUEUE_H__
#define __CONC_QUEUE_H__
#include "queue.h"
#include <pthread.h>

/**
 * Estructura principal que representa la tabla hash.
 */
typedef struct _ConcQueue *ConcQueue;

Queue conc_queue_queue(ConcQueue cq);

void conc_queue_lock(ConcQueue cq);

void conc_queue_unlock(ConcQueue cq);

/**
 * Devuelve una cola vacía.
 */
ConcQueue conc_queue_create();

/**
 * Destruccion de la cola.
 */
void conc_queue_destroy(ConcQueue queue);


/**
 * Elimina un elemento puntual de la cola.
 */
int conc_queue_delete(ConcQueue queue, QueueData *data, int pos);

#endif /* __CONC_QUEUE_H__ */