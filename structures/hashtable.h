#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__
#include "queue.h"

/**
 * Casillas en la que almacenaremos los datos de la tabla hash.
 */
typedef Queue HashBox;

/**
 * Funcion de hash para strings propuesta por Kernighan & Ritchie en "The C
 * Programming Language (Second Ed.)".
 */
unsigned hash(Data *d, unsigned total);

typedef struct _HashTable *HashTable;

/**
 * Crea una nueva tabla hash vacia, con la capacidad dada.
 */
HashTable hashtable_make(unsigned size);

/**
 * Retorna el numero de elementos de la tabla.
 */
unsigned hashtable_nelems(HashTable table);

/**
 * Retorna la casilla de la posición n.
 */
HashBox hashtable_elem(HashTable table, unsigned n);

/**
 * Retorna la capacidad de la tabla.
 */
unsigned hashtable_size(HashTable table);

/**
 * Destruye la tabla.
 */
void hashtable_destroy(HashTable table);

/**
 * Inserta un dato en la tabla, o lo reemplaza si ya se encontraba.
 */
int hashtable_insert(HashTable table, QueueNode *qData);

/**
 * Guarda en qdata->value el valor asociado si la clave está en la tabla, o NULL en caso contrario.
 */
int hashtable_search(HashTable table, QueueData *qdata);

/**
 * Elimina el dato de la tabla que coincida con el dato dado.
 */
int hashtable_delete(HashTable table, QueueData *data);

/**
 * Elimina el primer dato de la cola en la posición dada.
*/
void hashtable_pop(HashTable t, int pos);

#endif /* __HASHTABLE_H__ */