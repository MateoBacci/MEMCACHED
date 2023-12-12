#ifndef __HASHCONC_H__
#define __HASHCONC_H__

#include <pthread.h>
#include "hashtable.h"
#include "conc_queue.h"

typedef struct _ConcHashTable *ConcHashTable;

int conc_hashtable_trylock(ConcHashTable t, unsigned pos);

HashTable conc_hashtable_table(ConcHashTable cht);

void conc_hashtable_lock(ConcHashTable cht, unsigned i);

void conc_hashtable_unlock(ConcHashTable cht, unsigned i);

/**
 * Crea una nueva tabla hash vacia, con la capacidad dada.
 */
ConcHashTable conc_hashtable_init(unsigned size);

/**
 * Retorna el numero de elementos de la tabla.
 */
int conc_hashtable_nelems(ConcHashTable table);

/**
 * Retorna la capacidad de la tabla.
 */
int conc_hashtable_size(ConcHashTable table);

/**
 * Retorna la casilla de la posicion n.
 */
Queue conc_hashtable_elem(ConcHashTable table, unsigned n);

/**
 * Destruye la tabla.
 */
void conc_hashtable_destroy(ConcHashTable table);

/**
 * Elimina el primer dato de la cola en la posición dada.
 */
void conc_hashtable_pop(ConcHashTable t, int pos);

#endif /* __HASHCONC_H__ */