#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "hashtable.h"

struct _HashTable
{
  HashBox *elems;
  unsigned numElems;
  unsigned size;
};

unsigned hash(Data *data, unsigned total)
{
  unsigned hash = 0;
  unsigned l = data->len;
  for (size_t i = 0; i < l; i++)
    hash += data->data[i] + 31 * hash;
  return hash % total;
}

HashTable hashtable_make(unsigned size)
{
  // Pedimos memoria para la estructura principal y las casillas.
  HashTable table = malloc(sizeof(struct _HashTable));
  assert(table != NULL);
  table->elems = malloc(sizeof(Queue) * size);
  assert(table->elems != NULL);
  table->numElems = 0;
  table->size = size;

  // Inicializamos las casillas con datos nulos.
  for (unsigned idx = 0; idx < size; ++idx)
  {
    table->elems[idx] = queue_create();
  }

  return table;
}

unsigned hashtable_nelems(HashTable table) { return table->numElems; }

HashBox hashtable_elem(HashTable table, unsigned n) { return table->elems[n]; }

unsigned hashtable_size(HashTable table) { return table->size; }

void hashtable_destroy(HashTable table)
{

  // Destruir cada uno de los datos.
  for (unsigned idx = 0; idx < table->size; ++idx)
    queue_destroy(table->elems[idx]);

  // Liberar el arreglo de casillas y la tabla.
  free(table->elems);
  free(table);
  return;
}

int hashtable_insert(HashTable table, QueueNode *qNode)
{
  unsigned idx = queue_data_ihash(queue_node_data(qNode, NULL), -1);

  // Si está en la tabla lo eliminamos
  int pos = hashtable_delete(table, queue_node_data(qNode, NULL));
  queue_push(table->elems[idx], qNode);
  
  if (pos == -1)
    table->numElems++;
  return pos;
}

int hashtable_search(HashTable table, QueueData *qData)
{
  return queue_search(table->elems[queue_data_ihash(qData, -1)], qData);
}

int hashtable_delete(HashTable table, QueueData *data)
{
  int idx = queue_data_ihash(data, -1);
  // Eliminar el elemento si se encuentra en la tabla.
  if (queue_is_empty(table->elems[idx]))
    return -1;

  return queue_delete(table->elems[idx], data, -1);
}

void hashtable_pop(HashTable t, int pos)
{
  queue_pop(t->elems[pos]);
}

