#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "conc_hashtable.h"

#define CANT_LOCKS 100

struct _ConcHashTable
{
  HashTable tabla;
  pthread_mutex_t locks[CANT_LOCKS];
};

HashTable conc_hashtable_table(ConcHashTable cht)
{
  return cht->tabla;
}

int conc_hashtable_trylock(ConcHashTable t, unsigned pos) {
  return pthread_mutex_trylock(&t->locks[pos % CANT_LOCKS]);
}

void conc_hashtable_lock(ConcHashTable cht, unsigned i)
{
  pthread_mutex_lock(&cht->locks[i % CANT_LOCKS]);
}

void conc_hashtable_unlock(ConcHashTable cht, unsigned i)
{
  pthread_mutex_unlock(&cht->locks[i % CANT_LOCKS]);
}

ConcHashTable conc_hashtable_init(unsigned size)
{
  ConcHashTable table = malloc(sizeof(struct _ConcHashTable));
  table->tabla = hashtable_make(size);
  for (int i = 0; i < CANT_LOCKS; i++)
  {
    pthread_mutex_init(&table->locks[i], NULL);
  }
  return table;
}

int conc_hashtable_size(ConcHashTable table)
{
  return hashtable_size(table->tabla);
}

Queue conc_hashtable_elem(ConcHashTable table, unsigned n)
{
  return hashtable_elem(table->tabla, n);
}

void conc_hashtable_destroy(ConcHashTable table)
{
  for (int i = 0; i < CANT_LOCKS; i++)
    pthread_mutex_destroy(&table->locks[i]);
  hashtable_destroy(table->tabla);
  free(table);
}

void conc_hashtable_pop(ConcHashTable t, int pos)
{
  pthread_mutex_lock(&t->locks[pos]);
  hashtable_pop(t->tabla, pos);
  pthread_mutex_unlock(&t->locks[pos]);
}